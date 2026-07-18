#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>

/*
    AgentRoster — polls a JSON source describing your agents and exposes it
    to QML. Source can be a local file path (~ is expanded) or an http(s) URL,
    so it works with anything: an OpenClaw endpoint, or a status file your
    heartbeat/cron runs write out.

    Accepted shapes:
        [ {...}, {...} ]                       a plain array
        { "agents": [ {...}, {...} ] }         or wrapped in an object

    Suggested entry fields (passed through as-is to QML):
        { "name": "Hermes", "model": "mimo-v2.5-pro",
          "state": "idle", "detail": "heartbeat 30s" }
*/
class AgentRoster : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QVariantList items READ items NOTIFY updated)
    Q_PROPERTY(bool available READ available NOTIFY updated)
    Q_PROPERTY(QString error READ error NOTIFY updated)
    Q_PROPERTY(int intervalMs READ intervalMs WRITE setIntervalMs NOTIFY intervalMsChanged)

public:
    explicit AgentRoster(QObject *parent = nullptr);

    QString source() const { return m_source; }
    void setSource(const QString &src);

    QVariantList items() const { return m_items; }
    bool available() const { return m_available; }
    QString error() const { return m_error; }

    int intervalMs() const { return m_timer.interval(); }
    void setIntervalMs(int ms);

    Q_INVOKABLE void refresh();

signals:
    void sourceChanged();
    void updated();
    void intervalMsChanged();

private:
    void parse(const QByteArray &json);
    void fail(const QString &why);

    QString m_source;
    QVariantList m_items;
    bool m_available = false;
    QString m_error;

    QTimer m_timer;
    QNetworkAccessManager m_nam;
};
