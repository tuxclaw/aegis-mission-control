#include "SystemStats.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSysInfo>
#include <QVariantMap>

#include <algorithm>
#include <unistd.h>

namespace {

QString readAll(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(f.readAll());
}

QString readTrimmed(const QString &path)
{
    return readAll(path).trimmed();
}

} // namespace

SystemStats::SystemStats(QObject *parent)
    : QObject(parent)
{
    m_hostname = QSysInfo::machineHostName();
    m_kernel = QSysInfo::kernelType() + QStringLiteral(" ") + QSysInfo::kernelVersion();
    m_hz = sysconf(_SC_CLK_TCK);
    m_pageKb = sysconf(_SC_PAGESIZE) / 1024;

    discoverSensors();

    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &SystemStats::sample);
    m_timer.start();
    sample();   // prime deltas so the second tick shows real rates
}

void SystemStats::setIntervalMs(int ms)
{
    ms = qBound(250, ms, 60000);
    if (ms == m_timer.interval())
        return;
    m_timer.setInterval(ms);
    emit intervalMsChanged();
}

void SystemStats::setPaused(bool p)
{
    if (p == m_paused)
        return;
    m_paused = p;
    if (p)
        m_timer.stop();
    else
        m_timer.start();
    emit pausedChanged();
}

void SystemStats::discoverSensors()
{
    // hwmon: CPU (k10temp on AMD, coretemp on Intel) and GPU (amdgpu edge temp).
    const QDir hwmonRoot(QStringLiteral("/sys/class/hwmon"));
    const auto entries = hwmonRoot.entryList({QStringLiteral("hwmon*")}, QDir::Dirs);
    for (const QString &entry : entries) {
        const QString base = hwmonRoot.filePath(entry);
        const QString name = readTrimmed(base + QStringLiteral("/name"));
        if ((name == QLatin1String("k10temp") || name == QLatin1String("coretemp"))
            && m_cpuTempPath.isEmpty())
            m_cpuTempPath = base + QStringLiteral("/temp1_input");
        else if (name == QLatin1String("amdgpu") && m_gpuTempPath.isEmpty())
            m_gpuTempPath = base + QStringLiteral("/temp1_input");
    }

    // amdgpu utilization.
    const QDir drmRoot(QStringLiteral("/sys/class/drm"));
    const auto cards = drmRoot.entryList({QStringLiteral("card?")}, QDir::Dirs);
    for (const QString &card : cards) {
        const QString path = drmRoot.filePath(card) + QStringLiteral("/device/gpu_busy_percent");
        if (QFile::exists(path)) {
            m_gpuBusyPath = path;
            break;
        }
    }
}

void SystemStats::sample()
{
    sampleCpu();
    sampleTemps();
    sampleGpu();
    sampleMemory();
    sampleNetwork();
    sampleProcesses();
    sampleUptime();
    emit sampled();
}

void SystemStats::sampleCpu()
{
    const QString stat = readAll(QStringLiteral("/proc/stat"));
    if (stat.isEmpty())
        return;

    std::vector<std::pair<qulonglong, qulonglong>> now;   // (busy, total)
    QVariantList coreLoads;
    double aggregate = 0;

    const auto lines = stat.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (!line.startsWith(QLatin1String("cpu")))
            break;
        const auto parts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (parts.size() < 8)
            continue;

        qulonglong total = 0;
        for (int i = 1; i < parts.size(); ++i)
            total += parts.at(i).toULongLong();
        const qulonglong idle = parts.at(4).toULongLong()      // idle
                              + parts.at(5).toULongLong();     // iowait
        const qulonglong busy = total - idle;

        const size_t idx = now.size();
        double load = 0;
        if (idx < m_prevCpu.size()) {
            const qulonglong dBusy = busy - m_prevCpu[idx].first;
            const qulonglong dTotal = total - m_prevCpu[idx].second;
            if (dTotal > 0)
                load = double(dBusy) / double(dTotal);
        }
        now.emplace_back(busy, total);

        if (idx == 0)
            aggregate = load;                                  // "cpu" line
        else
            coreLoads.append(load);                            // "cpuN" lines
    }

    m_prevCpu = std::move(now);
    m_cpuUsage = aggregate;
    m_perCore = coreLoads;
}

void SystemStats::sampleTemps()
{
    if (!m_cpuTempPath.isEmpty())
        m_cpuTemp = readTrimmed(m_cpuTempPath).toDouble() / 1000.0;
    if (!m_gpuTempPath.isEmpty())
        m_gpuTemp = readTrimmed(m_gpuTempPath).toDouble() / 1000.0;
}

void SystemStats::sampleGpu()
{
    const QString busy = readTrimmed(m_gpuBusyPath);
    if (busy.isEmpty()) {
        m_gpuBusy = qQNaN();
        return;
    }

    bool ok = false;
    const double percent = busy.toDouble(&ok);
    m_gpuBusy = ok ? percent / 100.0 : qQNaN();
}

void SystemStats::sampleMemory()
{
    const QString meminfo = readAll(QStringLiteral("/proc/meminfo"));
    if (meminfo.isEmpty())
        return;

    qulonglong totalKb = 0;
    qulonglong availKb = 0;
    const auto lines = meminfo.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (line.startsWith(QLatin1String("MemTotal:")))
            totalKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toULongLong();
        else if (line.startsWith(QLatin1String("MemAvailable:")))
            availKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toULongLong();
        if (totalKb && availKb)
            break;
    }

    m_memTotalGiB = double(totalKb) / (1024.0 * 1024.0);
    m_memUsedGiB = double(totalKb - availKb) / (1024.0 * 1024.0);
}

void SystemStats::sampleNetwork()
{
    const QString dev = readAll(QStringLiteral("/proc/net/dev"));
    if (dev.isEmpty())
        return;

    qulonglong rx = 0;
    qulonglong tx = 0;
    const auto lines = dev.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0)
            continue;
        const QString iface = line.left(colon).trimmed();
        if (iface == QLatin1String("lo"))
            continue;
        const auto fields = line.mid(colon + 1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (fields.size() < 10)
            continue;
        rx += fields.at(0).toULongLong();
        tx += fields.at(8).toULongLong();
    }

    const qint64 stamp = QDateTime::currentMSecsSinceEpoch();
    if (!m_netPrimed) {
        m_netPrimed = true;
    } else if (stamp > m_prevNetStamp) {
        const double dt = double(stamp - m_prevNetStamp) / 1000.0;
        m_netRx = double(rx - m_prevRxBytes) / dt;
        m_netTx = double(tx - m_prevTxBytes) / dt;
    }
    m_prevRxBytes = rx;
    m_prevTxBytes = tx;
    m_prevNetStamp = stamp;
}

void SystemStats::sampleProcesses()
{
    const QDir procRoot(QStringLiteral("/proc"));
    const auto entries = procRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (entries.isEmpty())
        return;

    const qint64 stamp = QDateTime::currentMSecsSinceEpoch();
    const double dt = m_prevProcStamp > 0 ? double(stamp - m_prevProcStamp) / 1000.0 : 0;

    struct Row { int pid; QString name; double cpu; double memMiB; };
    std::vector<Row> rows;
    rows.reserve(512);
    QHash<int, qulonglong> nowTicks;
    int count = 0;

    for (const QString &entry : entries) {
        bool numeric = false;
        const int pid = entry.toInt(&numeric);
        if (!numeric)
            continue;
        ++count;

        const QString base = QStringLiteral("/proc/") + entry;
        const QString statLine = readAll(base + QStringLiteral("/stat"));
        if (statLine.isEmpty())
            continue;
        // comm can contain spaces/parens — fields resume after the last ')'.
        const int close = statLine.lastIndexOf(QLatin1Char(')'));
        if (close < 0)
            continue;
        const auto rest = statLine.mid(close + 2).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (rest.size() < 13)
            continue;
        const qulonglong ticks = rest.at(11).toULongLong()     // utime
                               + rest.at(12).toULongLong();    // stime
        nowTicks.insert(pid, ticks);

        double cpu = 0;
        const auto it = m_prevProcTicks.constFind(pid);
        if (it != m_prevProcTicks.cend() && dt > 0 && m_hz > 0)
            cpu = double(ticks - it.value()) / (dt * double(m_hz));

        double memMiB = 0;
        const auto statm = readAll(base + QStringLiteral("/statm"))
                               .split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (statm.size() >= 2)
            memMiB = statm.at(1).toDouble() * double(m_pageKb) / 1024.0;

        QString name = readTrimmed(base + QStringLiteral("/comm"));
        if (name.isEmpty()) {
            const int open = statLine.indexOf(QLatin1Char('('));
            name = statLine.mid(open + 1, close - open - 1);
        }

        rows.push_back({pid, name, cpu, memMiB});
    }

    std::sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) {
        if (a.cpu != b.cpu)
            return a.cpu > b.cpu;
        return a.memMiB > b.memMiB;
    });

    QVariantList top;
    const size_t n = std::min<size_t>(rows.size(), 12);
    for (size_t i = 0; i < n; ++i) {
        QVariantMap m;
        m.insert(QStringLiteral("pid"), rows[i].pid);
        m.insert(QStringLiteral("name"), rows[i].name);
        m.insert(QStringLiteral("cpu"), rows[i].cpu);
        m.insert(QStringLiteral("memMiB"), rows[i].memMiB);
        top.append(m);
    }

    m_topProcesses = top;
    m_processCount = count;
    m_prevProcTicks = std::move(nowTicks);
    m_prevProcStamp = stamp;
}

void SystemStats::sampleUptime()
{
    const double seconds = readAll(QStringLiteral("/proc/uptime"))
                               .section(QLatin1Char(' '), 0, 0)
                               .toDouble();
    if (seconds <= 0)
        return;

    const int days = int(seconds / 86400);
    const int hours = int(seconds / 3600) % 24;
    const int mins = int(seconds / 60) % 60;
    if (days > 0)
        m_uptime = QStringLiteral("up %1d %2h %3m").arg(days).arg(hours).arg(mins);
    else if (hours > 0)
        m_uptime = QStringLiteral("up %1h %2m").arg(hours).arg(mins);
    else
        m_uptime = QStringLiteral("up %1m").arg(mins);
}
