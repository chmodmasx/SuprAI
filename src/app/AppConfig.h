#pragma once

#include <suprai/runtime/RuntimeConfig.h>

#include <QString>

namespace suprai::app {

struct AppConfig {
    QString runtimeMode = QStringLiteral("native");
    QString baseUrl = QStringLiteral("http://127.0.0.1:8090/v1");
    QString model;
    QString apiKey;
    QString systemPrompt = QStringLiteral(
        "You are SuprAI, a concise Linux-native AI assistant. "
        "Answer in the user's language unless asked otherwise.");

    suprai::runtime::AgentRuntimeConfig agentConfig() const
    {
        return {
            .model = model,
            .systemPrompt = systemPrompt,
        };
    }
};

} // namespace suprai::app
