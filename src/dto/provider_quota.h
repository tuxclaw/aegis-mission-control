#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

struct ProviderWindow {
    QString   label;             // "5h", "weekly", "monthly", "plan"
    qint64    tokensUsed  = 0;
    qint64    tokensLimit = 0;   // 0 = unknown
    double    creditsUsed  = 0.0;
    double    creditsLimit = 0.0;
    double    usedFraction = 0.0;  // 0.0-1.0
    QDateTime resetsAt;
};

struct ProviderBalance {
    double  cash     = 0.0;
    double  gift     = 0.0;
    double  total    = 0.0;
    QString currency = QStringLiteral("USD");
};

struct ProviderQuota {
    QString  id;           // "openai" | "anthropic" | "xai" | "xiaomi" | "gemini" | "minimax" | "zai"
    QString  displayName;
    bool     enabled  = true;
    bool     fetched  = false;
    bool     hasError = false;
    QString  errorMsg;

    QList<ProviderWindow> windows;
    ProviderBalance       balance;
    QString               planName;
    QDateTime             fetchedAt;

    double primaryFraction() const {
        return windows.isEmpty() ? -1.0 : windows.first().usedFraction;
    }
};
