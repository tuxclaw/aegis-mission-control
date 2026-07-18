#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <vector>

/*
    ContainerList — polls the container runtime for `ps -a` as JSON.

    Tries these commands in order until one succeeds, then sticks with it:
        podman
        flatpak-spawn --host podman      (from a Flatpak sandbox)
        distrobox-host-exec podman       (from inside a distrobox)
        docker

    Exposes `items` as a list of {name, image, state, status}.
    `available` is false while no runtime is reachable.
*/
class ContainerList : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY updated)
    Q_PROPERTY(bool available READ available NOTIFY updated)
    Q_PROPERTY(QString runtime READ runtime NOTIFY updated)
    Q_PROPERTY(int intervalMs READ intervalMs WRITE setIntervalMs NOTIFY intervalMsChanged)

public:
    explicit ContainerList(QObject *parent = nullptr);

    QVariantList items() const { return m_items; }
    bool available() const { return m_available; }
    QString runtime() const;
    int intervalMs() const { return m_timer.interval(); }
    void setIntervalMs(int ms);

    Q_INVOKABLE void refresh();

signals:
    void updated();
    void intervalMsChanged();

private:
    void succeed(const QByteArray &output);
    void fail();

    std::vector<QStringList> m_candidates;
    size_t m_index = 0;
    bool m_locked = false;          // stop rotating once a runtime answered
    bool m_attemptSettled = true;   // guards double fail (errorOccurred + finished)

    bool m_available = false;
    QVariantList m_items;
    QTimer m_timer;
    QProcess m_proc;
};
