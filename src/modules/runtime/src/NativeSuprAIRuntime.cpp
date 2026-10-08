#include "NativeSuprAIRuntime.h"

#include "AgentEngine.h"
#include "RuntimeOrchestrator.h"

#include <suprai/persistence/PersistencePort.h>

#include <utility>

namespace suprai::runtime::internal {

NativeSuprAIRuntime::NativeSuprAIRuntime(
    suprai::runtime::AgentRuntimeConfig config,
    suprai::providers::Provider *provider,
    suprai::persistence::PersistencePort *persistence,
    QObject *parent)
    : AgentRuntime(parent)
    , m_engine(new AgentEngine(provider, this))
{
    if (persistence && !persistence->parent()) {
        persistence->setParent(this);
    }

    m_orchestrator = new RuntimeOrchestrator(
        std::move(config),
        m_engine,
        persistence,
        this);

    connect(m_orchestrator, &RuntimeOrchestrator::eventOccurred,
            this, &AgentRuntime::eventOccurred);
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
