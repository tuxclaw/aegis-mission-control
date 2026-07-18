#include "services/providers/grok_fetcher.h"

#include "services/monitor_config.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

GrokFetcher::GrokFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                         QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void GrokFetcher::fetch() {
    m_cliPath = m_config->providerCredential(
        QStringLiteral("xai"), QStringLiteral("cliPath"));
    if (m_cliPath.isEmpty())
        m_cliPath = QStringLiteral("/usr/local/bin/grok");

    // Verify the CLI exists; fall back to PATH lookup
    if (!QFileInfo::exists(m_cliPath)) {
        const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("grok"));
        if (fromPath.isEmpty()) {
            emit failed(id(),
                QStringLiteral("grok CLI not found at %1").arg(m_cliPath));
            return;
        }
        m_cliPath = fromPath;
    }

    auto* process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, &QProcess::finished, this,
        [this, process](int exitCode, QProcess::ExitStatus) {
            onProcessFinished(exitCode, process);
        });
    connect(process, &QProcess::errorOccurred, this,
        [this, process](QProcess::ProcessError err) {
            if (err == QProcess::FailedToStart) {
                emit failed(id(),
                    QStringLiteral("Cannot start grok CLI: %1")
                        .arg(process->errorString()));
                process->deleteLater();
            }
        });

    // 10-second timeout
    QTimer::singleShot(10000, process, [this, process]() {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            emit failed(id(), QStringLiteral("grok CLI timed out"));
        }
    });

    QStringList args;
    args << QStringLiteral("usage") << QStringLiteral("--json");
    process->start(m_cliPath, args);
}

void GrokFetcher::onProcessFinished(int exitCode, QProcess* process) {
    process->deleteLater();

    if (exitCode != 0) {
        const QString stderr = QString::fromUtf8(process->readAllStandardError());
        emit failed(id(),
            QStringLiteral("grok exited %1: %2").arg(exitCode).arg(stderr.trimmed()));
        return;
    }

    const QByteArray raw = process->readAllStandardOutput();
    parseOutput(QString::fromUtf8(raw));
}

void GrokFetcher::parseOutput(const QString& output) {
    const QString trimmed = output.trimmed();

    // Try JSON first
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmed.toUtf8(), &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        parseJsonOutput(trimmed.toUtf8());
        return;
    }

    // Fall back to text pattern matching
    parseTextOutput(trimmed);
}

void GrokFetcher::parseJsonOutput(const QByteArray& raw) {
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    const QJsonObject root = doc.object();

    ProviderQuota quota;
    quota.id = id();
    quota.displayName = displayName();

    const QString plan = root.value(QStringLiteral("plan")).toString();
    if (!plan.isEmpty())
        quota.planName = plan;

    const QJsonObject usage = root.value(QStringLiteral("usage")).toObject();
    const qint64 tokens = static_cast<qint64>(usage.value(QStringLiteral("tokens")).toDouble());
    const qint64 limit  = static_cast<qint64>(usage.value(QStringLiteral("limit")).toDouble());

    ProviderWindow w;
    w.label = QStringLiteral("plan");
    w.tokensUsed = tokens;
    w.tokensLimit = limit;
    if (limit > 0)
        w.usedFraction =
            qBound(0.0, static_cast<double>(tokens) /
                            static_cast<double>(limit),
                   1.0);

    const QString resetStr = usage.value(QStringLiteral("reset_at")).toString();
    if (!resetStr.isEmpty())
        w.resetsAt = QDateTime::fromString(resetStr, Qt::ISODate);

    quota.windows.append(w);
    emit done(quota);
}

void GrokFetcher::parseTextOutput(const QString& text) {
    // Try patterns like "tokens: 150000", "usage: 150000/500000", "150k/500k"
    static QRegularExpression reTokens(
        QStringLiteral(R"(tokens?\s*[:=]\s*(\d+))"),
        QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression reUsage(
        QStringLiteral(R"(usage\s*[:=]\s*(\d+)\s*/\s*(\d+))"),
        QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression reKFormat(
        QStringLiteral(R"((\d+(?:\.\d+)?)\s*[kK]\s*/\s*(\d+(?:\.\d+)?)\s*[kK])"));

    ProviderQuota quota;
    quota.id = id();
    quota.displayName = displayName();
    quota.planName = QStringLiteral("CLI");

    qint64 used = 0, limit = 0;

    auto mUsage = reUsage.match(text);
    if (mUsage.hasMatch()) {
        used  = mUsage.captured(1).toLongLong();
        limit = mUsage.captured(2).toLongLong();
    } else {
        auto mK = reKFormat.match(text);
        if (mK.hasMatch()) {
            used  = static_cast<qint64>(mK.captured(1).toDouble() * 1000);
            limit = static_cast<qint64>(mK.captured(2).toDouble() * 1000);
        } else {
            auto mTokens = reTokens.match(text);
            if (mTokens.hasMatch())
                used = mTokens.captured(1).toLongLong();
        }
    }

    if (used == 0 && limit == 0) {
        emit failed(id(), QStringLiteral("Could not parse grok output"));
        return;
    }

    ProviderWindow w;
    w.label = QStringLiteral("plan");
    w.tokensUsed = used;
    w.tokensLimit = limit;
    if (limit > 0)
        w.usedFraction =
            qBound(0.0, static_cast<double>(used) /
                            static_cast<double>(limit),
                   1.0);

    quota.windows.append(w);
    emit done(quota);
}
