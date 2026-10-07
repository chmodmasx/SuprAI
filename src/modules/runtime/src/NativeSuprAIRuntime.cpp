#include "NativeSuprAIRuntime.h"

#include "AgentEngine.h"
#include "RuntimeOrchestrator.h"

#include <utility>

namespace suprai::runtime::internal {

NativeSuprAIRuntime::NativeSuprAIRuntime(
    suprai::runtime::AgentRuntimeConfig config,
    suprai::providers::Provider *provider,
    QObject *parent)
    : AgentRuntime(parent)
    , m_engine(new AgentEngine(provider, this))
    , m_orchestrator(new RuntimeOrchestrator(std::move(config), m_engine, this))
{
    connect(m_orchestrator, &RuntimeOrchestrator::stateChanged,
            this, &AgentRuntime::stateChanged);
    connect(m_orchestrator, &RuntimeOrchestrator::userMessageAccepted,
            this, &AgentRuntime::userMessageAccepted);
    connect(m_orchestrator, &RuntimeOrchestrator::assistantMessageStarted,
            this, &AgentRuntime::assistantMessageStarted);
    connect(m_orchestrator, &RuntimeOrchestrator::assistantTextDelta,
            this, &AgentRuntime::assistantTextDelta);
    connect(m_orchestrator, &RuntimeOrchestrator::assistantMessageCompleted,
            this, &AgentRuntime::assistantMessageCompleted);
    connect(m_orchestrator, &RuntimeOrchestrator::reasoningActiveChanged,
            this, &AgentRuntime::reasoningActiveChanged);
    connect(m_orchestrator, &RuntimeOrchestrator::conversationReset,
            this, &AgentRuntime::conversationReset);
    connect(m_orchestrator, &RuntimeOrchestrator::errorOccurred,
            this, &AgentRuntime::errorOccurred);
    connect(m_orchestrator, &RuntimeOrchestrator::stopped,
            this, &AgentRuntime::stopped);
}

void NativeSuprAIRuntime::start()
{
    m_orchestrator->start();
}

void NativeSuprAIRuntime::shutdown()
{
    m_orchestrator->shutdown();
}

void NativeSuprAIRuntime::submitPrompt(const QString &prompt)
{
    m_orchestrator->submitPrompt(prompt);
}

void NativeSuprAIRuntime::cancelTurn()
{
    m_orchestrator->cancelTurn();
}

void NativeSuprAIRuntime::resetSession()
{
    m_orchestrator->resetSession();
}

} // namespace suprai::runtime::internal
