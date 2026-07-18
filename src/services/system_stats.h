#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <vector>

/*
    SystemStats — Linux system sampling for the dashboard.

    Sources:
      CPU load       /proc/stat (aggregate + per-core, delta-based)
      CPU temp       hwmon "k10temp" (AMD) or "coretemp" (Intel), temp1_input
      GPU busy       /sys/class/drm/card*/device/gpu_busy_percent (amdgpu)
      GPU temp       hwmon "amdgpu", temp1_input (edge)
      Memory         /proc/meminfo (MemTotal − MemAvailable)
      Network        /proc/net/dev (bytes/s across non-loopback interfaces)
      Processes      /proc/[pid]/stat + statm + comm (top 12 by CPU, delta-based)
      Uptime         /proc/uptime

    All values refresh together; bind to the properties and listen to sampled().
    On non-Linux systems (or sandboxes without these files) values stay at 0.
*/
class SystemStats : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY sampled)        // 0..1
    Q_PROPERTY(QVariantList perCore READ perCore NOTIFY sampled)    // list of 0..1
    Q_PROPERTY(double cpuTemp READ cpuTemp NOTIFY sampled)          // °C
    Q_PROPERTY(double gpuBusy READ gpuBusy NOTIFY sampled)          // 0..1
    Q_PROPERTY(double gpuTemp READ gpuTemp NOTIFY sampled)          // °C
    Q_PROPERTY(double memUsedGiB READ memUsedGiB NOTIFY sampled)
    Q_PROPERTY(double memTotalGiB READ memTotalGiB NOTIFY sampled)
    Q_PROPERTY(double netRx READ netRx NOTIFY sampled)              // bytes/s
    Q_PROPERTY(double netTx READ netTx NOTIFY sampled)              // bytes/s
    // Top processes by CPU: list of {pid, name, cpu (fraction of one core), memMiB}
    Q_PROPERTY(QVariantList topProcesses READ topProcesses NOTIFY sampled)
    Q_PROPERTY(int processCount READ processCount NOTIFY sampled)
    Q_PROPERTY(QString uptime READ uptime NOTIFY sampled)
    Q_PROPERTY(QString hostname READ hostname CONSTANT)
    Q_PROPERTY(QString kernel READ kernel CONSTANT)
    Q_PROPERTY(int intervalMs READ intervalMs WRITE setIntervalMs NOTIFY intervalMsChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)

public:
    explicit SystemStats(QObject *parent = nullptr);

    double cpuUsage() const { return m_cpuUsage; }
    QVariantList perCore() const { return m_perCore; }
    double cpuTemp() const { return m_cpuTemp; }
    double gpuBusy() const { return m_gpuBusy; }
    double gpuTemp() const { return m_gpuTemp; }
    double memUsedGiB() const { return m_memUsedGiB; }
    double memTotalGiB() const { return m_memTotalGiB; }
    double netRx() const { return m_netRx; }
    double netTx() const { return m_netTx; }
    QVariantList topProcesses() const { return m_topProcesses; }
    int processCount() const { return m_processCount; }
    QString uptime() const { return m_uptime; }
    QString hostname() const { return m_hostname; }
    QString kernel() const { return m_kernel; }

    int intervalMs() const { return m_timer.interval(); }
    void setIntervalMs(int ms);

    bool paused() const { return m_paused; }
    void setPaused(bool p);

signals:
    void sampled();
    void intervalMsChanged();
    void pausedChanged();

private:
    void discoverSensors();
    void sample();
    void sampleCpu();
    void sampleTemps();
    void sampleGpu();
    void sampleMemory();
    void sampleNetwork();
    void sampleProcesses();
    void sampleUptime();

    QTimer m_timer;
    bool m_paused = false;

    // CPU deltas: (busy, total) per line; index 0 = aggregate.
    std::vector<std::pair<qulonglong, qulonglong>> m_prevCpu;
    qulonglong m_prevRxBytes = 0;
    qulonglong m_prevTxBytes = 0;
    qint64 m_prevNetStamp = 0;
    QHash<int, qulonglong> m_prevProcTicks;
    qint64 m_prevProcStamp = 0;
    long m_hz = 100;
    long m_pageKb = 4;

    QString m_cpuTempPath;
    QString m_gpuTempPath;
    QString m_gpuBusyPath;

    double m_cpuUsage = 0;
    QVariantList m_perCore;
    double m_cpuTemp = 0;
    double m_gpuBusy = 0;
    double m_gpuTemp = 0;
    double m_memUsedGiB = 0;
    double m_memTotalGiB = 0;
    double m_netRx = 0;
    double m_netTx = 0;
    QVariantList m_topProcesses;
    int m_processCount = 0;
    QString m_uptime;
    QString m_hostname;
    QString m_kernel;
};
