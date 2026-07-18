#include "AgentRoster.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

AgentRoster::AgentRoster(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(3000);
    connect(&m_timer, &QTimer::timeout, this, &AgentRoster::refresh);
    m_timer.start();
}

void AgentRoster::setSource(const QString &src)
{
    if (src == m_source)
        return;
    m_source = src;
    emit sourceChanged();
    refresh();
}

void AgentRoster::setIntervalMs(int ms)
{
    ms = qBound(500, ms, 300000);
    if (ms == m_timer.interval())
        return;
    m_timer.setInterval(ms);
    emit intervalMsChanged();
}

void AgentRoster::refresh()
{
    if (m_source.isEmpty()) {
        fail(QStringLiteral("no source configured"));
        return;
    }

    if (m_source.startsWith(QLatin1String("http://"))
        || m_source.startsWith(QLatin1String("https://"))) {
        QNetworkReply *reply = m_nam.get(QNetworkRequest(QUrl(m_source)));
        connect(reply, &QNetworkReply::finished, this, [this, reply] {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                fail(reply->errorString());
                return;
            }
            parse(reply->readAll());
        });
        return;
    }

    QString path = m_source;
    if (path.startsWith(QLatin1String("~/")))
        path = QDir::homePath() + path.mid(1);

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        fail(QStringLiteral("cannot read %1").arg(path));
        return;
    }
    parse(f.readAll());
}

void AgentRoster::parse(const QByteArray &json)
{
    const QJsonDocument doc = QJsonDocument::fromJson(json);

    QJsonArray arr;
    if (doc.isArray())
        arr = doc.array();
    else if (doc.isObject() && doc.object().value(QStringLiteral("agents")).isArray())
        arr = doc.object().value(QStringLiteral("agents")).toArray();
    else {
        fail(QStringLiteral("expected a JSON array or {\"agents\": [...]}"));
        return;
    }

    QVariantList items;
    for (const auto &v : std::as_const(arr))
        if (v.isObject())
            items.append(v.toObject().toVariantMap());

    m_items = items;
    m_available = true;
    m_error.clear();
    emit updated();
}

void AgentRoster::fail(const QString &why)
{
    m_available = false;
    m_error = why;
    m_items.clear();
    emit updated();
}
