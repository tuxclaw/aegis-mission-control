#include "ContainerList.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

ContainerList::ContainerList(QObject *parent)
    : QObject(parent)
{
    m_candidates = {
        {QStringLiteral("podman")},
        {QStringLiteral("flatpak-spawn"), QStringLiteral("--host"), QStringLiteral("podman")},
        {QStringLiteral("distrobox-host-exec"), QStringLiteral("podman")},
        {QStringLiteral("docker")},
    };

    connect(&m_proc, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus st) {
                if (m_attemptSettled)
                    return;
                if (st == QProcess::NormalExit && exitCode == 0)
                    succeed(m_proc.readAllStandardOutput());
                else
                    fail();
            });
    connect(&m_proc, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError err) {
                if (err == QProcess::FailedToStart && !m_attemptSettled)
                    fail();
            });

    m_timer.setInterval(5000);
    connect(&m_timer, &QTimer::timeout, this, &ContainerList::refresh);
    m_timer.start();
    refresh();
}

QString ContainerList::runtime() const
{
    if (m_index >= m_candidates.size())
        return {};
    return m_candidates[m_index].join(QLatin1Char(' '));
}

void ContainerList::setIntervalMs(int ms)
{
    ms = qBound(1000, ms, 300000);
    if (ms == m_timer.interval())
        return;
    m_timer.setInterval(ms);
    emit intervalMsChanged();
}

void ContainerList::refresh()
{
    if (m_proc.state() != QProcess::NotRunning)
        return;

    QStringList args = m_candidates[m_index];
    const QString program = args.takeFirst();
    args << QStringLiteral("ps") << QStringLiteral("-a")
         << QStringLiteral("--format") << QStringLiteral("json");

    m_attemptSettled = false;
    m_proc.start(program, args);
}

void ContainerList::succeed(const QByteArray &output)
{
    m_attemptSettled = true;
    m_locked = true;

    // podman emits a JSON array; docker emits JSON-lines. Handle both.
    QList<QJsonObject> objects;
    const QJsonDocument doc = QJsonDocument::fromJson(output);
    if (doc.isArray()) {
        const auto arr = doc.array();
        for (const auto &v : arr)
            if (v.isObject())
                objects.append(v.toObject());
    } else {
        const auto lines = output.split('\n');
        for (const QByteArray &line : lines) {
            const QJsonDocument lineDoc = QJsonDocument::fromJson(line.trimmed());
            if (lineDoc.isObject())
                objects.append(lineDoc.object());
        }
    }

    QVariantList items;
    for (const QJsonObject &obj : objects) {
        QString name;
        const QJsonValue names = obj.value(QStringLiteral("Names"));
        if (names.isArray() && !names.toArray().isEmpty())
            name = names.toArray().first().toString();
        else
            name = names.toString();

        QVariantMap m;
        m.insert(QStringLiteral("name"), name);
        m.insert(QStringLiteral("image"), obj.value(QStringLiteral("Image")).toString());
        m.insert(QStringLiteral("state"), obj.value(QStringLiteral("State")).toString());
        m.insert(QStringLiteral("status"), obj.value(QStringLiteral("Status")).toString());
        items.append(m);
    }

    m_items = items;
    m_available = true;
    emit updated();
}

void ContainerList::fail()
{
    m_attemptSettled = true;
    m_available = false;
    m_items.clear();
    if (!m_locked)
        m_index = (m_index + 1) % m_candidates.size();
    emit updated();
}
