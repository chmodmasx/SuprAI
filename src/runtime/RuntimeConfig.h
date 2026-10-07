#pragma once

#include <QString>

namespace suprai::runtime {

// Runtime-owned configuration contains only agent/runtime semantics.
// Provider transport configuration stays in the application composition layer.
struct AgentRuntimeConfig {
    QString model = QStringLiteral("bonsai2-27b");
    QString systemPrompt = QStringLiteral(
        "You are SuprAI, a concise Linux-native AI assistant. "
        "Answer in the user's language unless asked otherwise.");
};

// Bootstrap/configuration input. NativeSuprAIRuntime never reads baseUrl/apiKey;
// ApplicationBootstrap consumes those fields to construct a provider adapter.
struct RuntimeConfig {
    QString mode = QStringLiteral("native");
    QString baseUrl = QStringLiteral("http://127.0.0.1:8090/v1");
    QString model = QStringLiteral("bonsai2-27b");
    QString apiKey = QStringLiteral("no-key");
    QString systemPrompt = QStringLiteral(
        "You are SuprAI, a concise Linux-native AI assistant. "
        "Answer in the user's language unless asked otherwise.");

    AgentRuntimeConfig agentConfig() const
    {
        return {
            .model = model,
            .systemPrompt = systemPrompt,
        };
    }
};

} // namespace suprai::runtime
