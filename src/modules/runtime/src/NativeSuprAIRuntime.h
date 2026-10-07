#pragma once

#include <suprai/runtime/AgentRuntime.h>
#include <suprai/runtime/RuntimeConfig.h>

namespace suprai::providers {
class Provider;
}

namespace suprai::runtime::internal {

class AgentEngine;
class RuntimeOrchestrator;

class NativeSuprAIRuntime final : public suprai::runtime::AgentRuntime
{
    Q_OBJECT

public:
    NativeSuprAIRuntime(
        suprai::runtime::AgentRuntimeConfig config,
        suprai::providers::Provider *provider,
        QObject *parent = nullptr);

public slots:
    void start() override;
    void shutdown() override;
    void submitPrompt(const QString &prompt) override;
    void cancelTurn() override;
    void resetSession() override;

private:
    AgentEngine *m_engine = nullptr;
    RuntimeOrchestrator *m_orchestrator = nullptr;
};

} // namespace suprai::runtime::internal
