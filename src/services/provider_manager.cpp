#include "services/provider_manager.h"

#include "services/monitor_config.h"
#include "services/i_provider_fetcher.h"
#include "services/providers/zai_fetcher.h"
#include "services/providers/xiaomi_fetcher.h"
#include "services/providers/minimax_fetcher.h"
#include "services/providers/codexbar_fetcher.h"

#include <QDateTime>
#include <QNetworkAccessManager>

namespace {

constexpr qint64 kCacheSeconds = 5 * 60;

}  // namespace

ProviderManager::ProviderManager(const MonitorConfig* config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_nam(new QNetworkAccessManager(this))
{
    // Register provider fetchers based on config
    if (config->providerEnabled(QStringLiteral("zai"))) {
        auto* zai = new ZAIFetcher(config, m_nam, this);
        m_fetchers.append(zai);
        connect(zai, &IProviderFetcher::done, this, &ProviderManager::onFetcherDone);
        connect(zai, &IProviderFetcher::failed, this, &ProviderManager::onFetcherFailed);
    }
    if (config->providerEnabled(QStringLiteral("xiaomi"))) {
        auto* xiaomi = new XiaomiFetcher(config, m_nam, this);
        m_fetchers.append(xiaomi);
        connect(xiaomi, &IProviderFetcher::done, this, &ProviderManager::onFetcherDone);
        connect(xiaomi, &IProviderFetcher::failed, this, &ProviderManager::onFetcherFailed);
    }
    if (config->providerEnabled(QStringLiteral("openai"))) {
        auto* openai = new CodexBarFetcher(config, QStringLiteral("openai"),
                                           QStringLiteral("Codex"), m_nam, this);
        m_fetchers.append(openai);
        connect(openai, &IProviderFetcher::done, this, &ProviderManager::onFetcherDone);
        connect(openai, &IProviderFetcher::failed, this, &ProviderManager::onFetcherFailed);
    }
    if (config->providerEnabled(QStringLiteral("anthropic"))) {
        auto* anthropic = new CodexBarFetcher(config, QStringLiteral("anthropic"),
                                              QStringLiteral("Claude"), m_nam, this);
        m_fetchers.append(anthropic);
        connect(anthropic, &IProviderFetcher::done, this, &ProviderManager::onFetcherDone);
        connect(anthropic, &IProviderFetcher::failed, this, &ProviderManager::onFetcherFailed);
    }
    if (config->providerEnabled(QStringLiteral("minimax"))) {
        auto* minimax = new MiniMaxFetcher(config, m_nam, this);
        m_fetchers.append(minimax);
        connect(minimax, &IProviderFetcher::done, this, &ProviderManager::onFetcherDone);
        connect(minimax, &IProviderFetcher::failed, this, &ProviderManager::onFetcherFailed);
    }
    if (config->providerEnabled(QStringLiteral("xai"))) {
        auto* xai = new CodexBarFetcher(config, QStringLiteral("xai"),
                                        QStringLiteral("Grok"), m_nam, this);
        m_fetchers.append(xai);
        connect(xai, &IProviderFetcher::done, this, &ProviderManager::onFetcherDone);
        connect(xai, &IProviderFetcher::failed, this, &ProviderManager::onFetcherFailed);
    }
    if (config->providerEnabled(QStringLiteral("gemini"))) {
        auto* gemini = new CodexBarFetcher(config, QStringLiteral("gemini"),
                                           QStringLiteral("Gemini"), m_nam, this);
        m_fetchers.append(gemini);
        connect(gemini, &IProviderFetcher::done, this, &ProviderManager::onFetcherDone);
        connect(gemini, &IProviderFetcher::failed, this, &ProviderManager::onFetcherFailed);
    }
}

ProviderManager::~ProviderManager() = default;

void ProviderManager::refresh() {
    m_pending.clear();

    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (IProviderFetcher* fetcher : m_fetchers) {
        const QString providerId = fetcher->id();
        const QDateTime fetchedAt = m_fetchedAt.value(providerId);
        if (m_cache.contains(providerId) && fetchedAt.isValid() &&
            fetchedAt.secsTo(now) < kCacheSeconds) {
            continue;
        }
        m_pending.insert(providerId);
    }

    if (m_pending.isEmpty()) {
        emit quotasReady(currentQuotas());
        return;
    }

    for (IProviderFetcher* fetcher : m_fetchers) {
        if (m_pending.contains(fetcher->id()))
            fetcher->fetch();
    }
}

QList<ProviderQuota> ProviderManager::currentQuotas() const {
    QList<ProviderQuota> quotas;
    QSet<QString> seen;

    for (const IProviderFetcher* fetcher : m_fetchers) {
        const QString providerId = fetcher->id();
        if (m_cache.contains(providerId)) {
            quotas.append(m_cache.value(providerId));
            seen.insert(providerId);
        }
    }

    for (auto it = m_cache.constBegin(); it != m_cache.constEnd(); ++it) {
        if (!seen.contains(it.key()))
            quotas.append(it.value());
    }

    return quotas;
}

void ProviderManager::onFetcherDone(ProviderQuota quota) {
    const QDateTime now = QDateTime::currentDateTimeUtc();
    quota.fetched = true;
    quota.fetchedAt = now;
    m_cache[quota.id] = quota;
    m_fetchedAt[quota.id] = now;
    m_pending.remove(quota.id);

    if (m_pending.isEmpty())
        emit quotasReady(currentQuotas());
}

void ProviderManager::onFetcherFailed(QString providerId, QString reason) {
    ProviderQuota quota;
    quota.id = providerId;
    quota.displayName = providerId;
    quota.fetched = true;
    quota.hasError = true;
    quota.errorMsg = reason;
    quota.fetchedAt = QDateTime::currentDateTimeUtc();

    m_cache[providerId] = quota;
    m_fetchedAt[providerId] = quota.fetchedAt;
    m_pending.remove(providerId);

    if (m_pending.isEmpty())
        emit quotasReady(currentQuotas());
}
