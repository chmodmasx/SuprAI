#include "NativeSuprAIRuntime.h"

#include "AgentEngine.h"
#include "RuntimeOrchestrator.h"

#include <suprai/domain/ConversationItem.h>
#include <suprai/runtime/ApplicationEvent.h>

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
    const auto currentTurnId = [this]() -> QString {
        return m_orchestrator->turns().isEmpty()
            ? QString{}
            : m_orchestrator->turns().constLast().id;
    };

    connect(m_orchestrator, &RuntimeOrchestrator::stateChanged,
            this, [this](RuntimeState state) {
                emit eventEmitted({
                    .payload = RuntimeStateChangedEvent{.state = state},
                });
            });

    connect(m_orchestrator, &RuntimeOrchestrator::userMessageAccepted,
            this, [this, currentTurnId](const QString &itemId, const QString &text) {
                emit eventEmitted({
                    .payload = ConversationItemStartedEvent{
                        .item = suprai::domain::makeMessageItem(
                            suprai::domain::ConversationRole::User,
                            text,
                            suprai::domain::ConversationItemState::Completed,
                            itemId,
                            currentTurnId()),
                    },
                });
            });

    connect(m_orchestrator, &RuntimeOrchestrator::assistantMessageStarted,
            this, [this, currentTurnId](const QString &itemId) {
                emit eventEmitted({
                    .payload = ConversationItemStartedEvent{
                        .item = suprai::domain::makeMessageItem(
                            suprai::domain::ConversationRole::Assistant,
                            {},
                            suprai::domain::ConversationItemState::Streaming,
                            itemId,
                            currentTurnId()),
                    },
                });
            });

    connect(m_orchestrator, &RuntimeOrchestrator::assistantTextDelta,
            this, [this](const QString &itemId, const QString &delta) {
                emit eventEmitted({
                    .payload = ConversationTextDeltaEvent{
                        .itemId = itemId,
                        .delta = delta,
                    },
                });
            });

    connect(m_orchestrator, &RuntimeOrchestrator::assistantMessageCompleted,
            this, [this](const QString &itemId, const QString &) {
                emit eventEmitted({
                    .payload = ConversationItemCompletedEvent{
                        .itemId = itemId,
                    },
                });
            });

    connect(m_orchestrator, &RuntimeOrchestrator::reasoningActiveChanged,
            this, [this](bool active) {
                emit eventEmitted({
                    .payload = ReasoningActivityChangedEvent{
                        .active = active,
                    },
                });
            });

    connect(m_orchestrator, &RuntimeOrchestrator::conversationReset,
            this, [this] {
                emit eventEmitted({
                    .payload = ConversationResetEvent{},
                });
            });

    connect(m_orchestrator, &RuntimeOrchestrator::errorOccurred,
            this, [this](const QString &message) {
                emit eventEmitted({
                    .payload = RuntimeErrorEvent{
                        .message = message,
                    },
                });
            });

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
