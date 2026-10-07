#pragma once

#include <QString>

namespace suprai::runtime {

struct RuntimeConfig {
    QString mode = QStringLiteral("native");
    QString baseUrl = QStringLiteral("http://127.0.0.1:8090/v1");
    QString model = QStringLiteral("bonsai2-27b");
    QString apiKey = QStringLiteral("no-key");
    QString systemPrompt = QStringLiteral(
        "You are SuprAI, a concise Linux-native AI assistant. "
        "Answer in the user's language unless asked otherwise.");
};

} // namespace suprai::runtime
