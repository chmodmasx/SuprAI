#include "RuntimeOrchestrator.h"

#include "AgentEngine.h"
#include "RuntimeEventAdapter.h"

#include <suprai/domain/ConversationItem.h>

#include <utility>

namespace suprai::runtime::internal {

RuntimeOrchestrator::RuntimeOrchestrator(
    suprai::runtime::AgentRuntimeConfig config,
    AgentEngine *engine,
    suprai::persistence::PersistencePort *persistence,
    QObject *parent)
    : QObject(parent)
    , m_config(std::move(config))
    , m_engine(engine)
    , m_eventAdapter(new RuntimeEventAdapter(engine, this))
    , m_persistence(persistence)
    , m_session{
          .id = suprai::domain::newSessionId(),
          .parentSessionId = {},
      }
{
    Q_ASSERT(m_engine);

    connect(m_eventAdapter, &RuntimeEventAdapter::runtimeEvent,
            this, &RuntimeOrchestrator::handleRuntimeEvent);

    if (m_persistence) {
        connect(m_persistence, &suprai::persistence::PersistencePort::latestSessionLoaded,
                this, &RuntimeOrchestrator::handleSessionLoaded);
        connect(m_persistence, &suprai::persistence::PersistencePort::readFailed,
                this, &RuntimeOrchestrator::handleSessionReadFailure);
        connect(m_persistence, &suprai::persistence::PersistencePort::turnStartPersisted,
                this, &RuntimeOrchestrator::handleTurnStartPersisted);
        connect(m_persistence, &suprai::persistence::PersistencePort::turnTerminalPersisted,
                this, &RuntimeOrchestrator::handleTurnTerminalPersisted);
        connect(m_persistence, &suprai::persistence::PersistencePort::writeFailed,
                this, &RuntimeOrchestrator::handlePersistenceFailure);
    }
}

const suprai::domain::Session &RuntimeOrchestrator::session() const
{
    return m_session;
}

const QVector<suprai::domain::Input> &RuntimeOrchestrator::inputs() const
{
    return m_inputs;
}

const QVector<suprai::domain::Turn> &RuntimeOrchestrator::turns() const
{
    return m_turns;
}

const QVector<suprai::domain::Run> &RuntimeOrchestrator::runs() const
{
    return m_runs;
}

const QVector<suprai::domain::ConversationItem> &RuntimeOrchestrator::history() const
{
    return m_history;
}

void RuntimeOrchestrator::start()
{
    if (m_state != RuntimeState::Stopped) {
        return;
    }

    setState(RuntimeState::Starting);

    if (!m_engine) {
        setState(RuntimeState::Failed);
        emitApplicationEvent({
            .payload = RuntimeError{
                .message = QStringLiteral("No hay un motor de agente configurado."),
            },
        });
        return;
    }

    setCapabilities(m_engine->capabilities(m_config.model));
    if (m_persistence) {
        m_persistence->loadLatestSession();
    } else {
        setState(RuntimeState::Ready);
    }
}

void RuntimeOrchestrator::shutdown()
{
    if (m_engine && m_engine->isBusy()) {
        m_engine->cancel();
    }

    clearActiveTurn();

    if (m_reasoningActive) {
        m_reasoningActive = false;
        emitApplicationEvent({
            .payload = ReasoningActiveChanged{
                .active = false,
            },
        });
    }

    setState(RuntimeState::Stopped);
    emit stopped();
}

suprai::providers::ProviderRequest RuntimeOrchestrator::providerRequest() const
{
    QVector<suprai::providers::ProviderMessage> messages;
    messages.reserve(m_history.size() + 1);

    if (!m_config.systemPrompt.trimmed().isEmpty()) {
        messages.push_back({
            .role = QStringLiteral("system"),
            .content = m_config.systemPrompt,
        });
    }

    for (const auto &item : m_history) {
        const auto *message = suprai::domain::messageContent(item);
        if (!message || item.state != suprai::domain::ConversationItemState::Completed) {
            continue;
        }

        messages.push_back({
            .role = suprai::domain::roleName(message->role),
            .content = message->text,
        });
    }

    return {
        .model = m_config.model,
        .messages = std::move(messages),
    };
}

void RuntimeOrchestrator::submitPrompt(const QString &prompt)
{
    const QString text = prompt.trimmed();
    if (text.isEmpty()) {
        return;
    }

    if (m_state != RuntimeState::Ready || !m_engine) {
        emitApplicationEvent({
            .payload = RuntimeError{
                .message = QStringLiteral("El runtime no está listo."),
            },
        });
        return;
    }

    m_activeInput = suprai::domain::Input{
        .id = suprai::domain::newInputId(),
        .sessionId = m_session.id,
        .sequence = m_inputs.size() + 1,
        .text = text,
    };

    m_activeTurn = suprai::domain::Turn{
        .id = suprai::domain::newTurnId(),
        .sessionId = m_session.id,
        .inputId = m_activeInput->id,
        .parentTurnId = m_turns.isEmpty() ? QString{} : m_turns.constLast().id,
        .sequence = m_turns.size() + 1,
    };

    m_activeRun = suprai::domain::Run{
        .id = suprai::domain::newRunId(),
        .turnId = m_activeTurn->id,
        .generation = 1,
    };

    m_pendingUserItem = suprai::domain::makeMessageItem(
        suprai::domain::ConversationRole::User,
        text,
        suprai::domain::ConversationItemState::Completed,
        suprai::domain::newItemId(),
        m_activeTurn->id,
        1);

    setState(RuntimeState::Working);

    if (m_persistence) {
        m_pendingPersistenceRequestId = m_activeRun->id;
        m_persistence->persistTurnStart({
            .requestId = m_pendingPersistenceRequestId,
            .session = m_session,
            .input = *m_activeInput,
            .turn = *m_activeTurn,
            .run = *m_activeRun,
            .userItem = *m_pendingUserItem,
        });
        return;
    }

    admitPendingTurnAndStartInference();
}

void RuntimeOrchestrator::cancelTurn()
{
    if (m_state != RuntimeState::Working || !m_engine) {
        return;
    }

    if (m_pendingTerminal) {
        return;
    }

    if (!m_pendingPersistenceRequestId.isEmpty()) {
        m_cancelBeforeInference = true;
        setState(RuntimeState::Cancelling);
        return;
    }

    setState(RuntimeState::Cancelling);
    m_engine->cancel();
}

void RuntimeOrchestrator::resetSession()
{
    if (m_state == RuntimeState::Working || m_state == RuntimeState::Cancelling) {
        emitApplicationEvent({
            .payload = RuntimeError{
                .message = QStringLiteral(
                    "Cancelá el turno activo antes de iniciar una conversación nueva."),
            },
        });
        return;
    }

    m_session = {
        .id = suprai::domain::newSessionId(),
        .parentSessionId = {},
    };
    m_inputs.clear();
    m_turns.clear();
    m_runs.clear();
    m_history.clear();
    clearActiveTurn();
    emitApplicationEvent({
        .payload = ConversationReset{},
    });
}

void RuntimeOrchestrator::handleSessionLoaded(suprai::persistence::SessionSnapshot snapshot)
{
    if (m_state != RuntimeState::Starting) {
        return;
    }
    if (snapshot.found) {
        m_session = std::move(snapshot.session);
        m_inputs = std::move(snapshot.inputs);
        m_turns = std::move(snapshot.turns);
        m_runs = std::move(snapshot.runs);
        m_history = std::move(snapshot.items);
        emitApplicationEvent({.payload = ConversationReset{}});
        for (const auto &item : m_history) {
            emitApplicationEvent({.payload = ConversationItemStarted{.item = item}});
        }
    }
    setState(RuntimeState::Ready);
}

void RuntimeOrchestrator::handleSessionReadFailure(const QString &message)
{
    if (m_state != RuntimeState::Starting) {
        return;
    }
    setState(RuntimeState::Failed);
    emitApplicationEvent({
        .payload = RuntimeError{
            .message = QStringLiteral("No se pudo recuperar la sesión: %1").arg(message),
        },
    });
}

void RuntimeOrchestrator::handleTurnStartPersisted(const QString &requestId)
{
    if (requestId != m_pendingPersistenceRequestId) {
        return;
    }

    admitPendingTurnAndStartInference();
}

void RuntimeOrchestrator::handleTurnTerminalPersisted(const QString &requestId)
{
    if (!m_pendingTerminal || requestId != m_pendingTerminal->requestId) {
        return;
    }
    finishDurableTerminal();
}

void RuntimeOrchestrator::handlePersistenceFailure(
    const QString &requestId,
    const QString &message)
{
    if (requestId == m_pendingPersistenceRequestId) {
        clearActiveTurn();
        setState(RuntimeState::Failed);
        emitApplicationEvent({
            .payload = RuntimeError{
                .message = QStringLiteral("No se pudo persistir el turno antes de inferir: %1")
                               .arg(message),
            },
        });
    } else if (m_pendingTerminal && requestId == m_pendingTerminal->requestId) {
        const QString itemId = m_activeAssistantId;
        clearActiveTurn();
        if (!itemId.isEmpty()) {
            emitApplicationEvent({.payload = ConversationItemStopped{
                .itemId = itemId,
                .state = suprai::domain::ConversationItemState::Failed,
            }});
        }
        setState(RuntimeState::Failed);
        emitApplicationEvent({
            .payload = RuntimeError{
                .message = QStringLiteral("No se pudo persistir el resultado terminal del turno: %1")
                               .arg(message),
            },
        });
    }
}

void RuntimeOrchestrator::admitPendingTurnAndStartInference()
{
    if (!m_activeInput
        || !m_activeTurn
        || !m_activeRun
        || !m_pendingUserItem) {
        clearActiveTurn();
        setState(RuntimeState::Failed);
        emitApplicationEvent({
            .payload = RuntimeError{
                .message = QStringLiteral(
                    "El runtime perdió el estado pendiente del turno."),
            },
        });
        return;
    }

    const auto userItem = *m_pendingUserItem;

    m_inputs.push_back(*m_activeInput);
    m_turns.push_back(*m_activeTurn);
    m_runs.push_back(*m_activeRun);
    m_history.push_back(userItem);

    m_pendingPersistenceRequestId.clear();
    m_pendingUserItem.reset();

    emitApplicationEvent({
        .payload = ConversationItemStarted{
            .item = userItem,
        },
    });

    if (m_cancelBeforeInference) {
        beginTerminalWrite(QStringLiteral("cancelled"));
        return;
    }

    m_activeAssistantId = suprai::domain::newItemId();
    m_activeAssistantText.clear();
    emitApplicationEvent({
        .payload = ConversationItemStarted{
            .item = suprai::domain::makeMessageItem(
                suprai::domain::ConversationRole::Assistant,
                {},
                suprai::domain::ConversationItemState::Streaming,
                m_activeAssistantId,
                m_activeTurn->id,
                2),
        },
    });

    m_engine->generate(providerRequest());
}

void RuntimeOrchestrator::handleRuntimeEvent(const RuntimeEvent &event)
{
    if (!m_activeRun || m_pendingTerminal || !m_pendingPersistenceRequestId.isEmpty()) {
        return;
    }
    switch (event.type) {
    case RuntimeEventType::AssistantTextDelta:
        if (!m_activeAssistantId.isEmpty()) {
            m_activeAssistantText += event.payload;
            emitApplicationEvent({
                .payload = ConversationTextDelta{
                    .itemId = m_activeAssistantId,
                    .delta = event.payload,
                },
            });
        }
        break;

    case RuntimeEventType::ReasoningDelta:
        if (!m_reasoningActive) {
            m_reasoningActive = true;
            emitApplicationEvent({
                .payload = ReasoningActiveChanged{
                    .active = true,
                },
            });
        }
        break;

    case RuntimeEventType::ProviderCompleted:
        beginTerminalWrite(QStringLiteral("completed"));
        break;

    case RuntimeEventType::ProviderCancelled:
        beginTerminalWrite(QStringLiteral("cancelled"));
        break;

    case RuntimeEventType::ProviderFailed:
        beginTerminalWrite(QStringLiteral("failed"), event.payload);
        break;
    }
}

void RuntimeOrchestrator::beginTerminalWrite(
    const QString &status,
    const QString &providerError)
{
    if (!m_activeRun || !m_activeTurn || m_pendingTerminal) {
        return;
    }

    if (m_reasoningActive) {
        m_reasoningActive = false;
        emitApplicationEvent({.payload = ReasoningActiveChanged{.active = false}});
    }

    suprai::persistence::TurnTerminalWrite terminal{
        .requestId = m_activeRun->id,
        .runId = m_activeRun->id,
        .turnId = m_activeTurn->id,
        .sessionId = m_session.id,
        .status = status,
    };

    if (!m_activeAssistantId.isEmpty()
        && (status == QStringLiteral("completed") || !m_activeAssistantText.isEmpty())) {
        const auto state = status == QStringLiteral("completed")
            ? suprai::domain::ConversationItemState::Completed
            : status == QStringLiteral("cancelled")
                ? suprai::domain::ConversationItemState::Cancelled
                : suprai::domain::ConversationItemState::Failed;
        terminal.assistantItem = suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant,
            m_activeAssistantText, state, m_activeAssistantId, m_activeTurn->id, 2);
    }

    m_pendingProviderError = providerError;
    m_pendingTerminal = std::move(terminal);
    if (m_persistence) {
        m_persistence->persistTurnTerminal(*m_pendingTerminal);
    } else {
        finishDurableTerminal();
    }
}

void RuntimeOrchestrator::finishDurableTerminal()
{
    if (!m_pendingTerminal) {
        return;
    }

    if (m_pendingTerminal->assistantItem) {
        m_history.push_back(*m_pendingTerminal->assistantItem);
    }
    for (auto &run : m_runs) {
        if (run.id != m_pendingTerminal->runId) continue;
        if (m_pendingTerminal->status == QStringLiteral("completed")) {
            run.status = suprai::domain::RunStatus::Completed;
        } else if (m_pendingTerminal->status == QStringLiteral("failed")) {
            run.status = suprai::domain::RunStatus::Failed;
        } else {
            run.status = suprai::domain::RunStatus::Cancelled;
        }
        break;
    }

    const QString itemId = m_activeAssistantId;
    const QString providerError = m_pendingProviderError;
    const QString terminalStatus = m_pendingTerminal->status;
    clearActiveTurn();
    if (!itemId.isEmpty()) {
        if (terminalStatus == QStringLiteral("completed")) {
            emitApplicationEvent({.payload = ConversationItemCompleted{.itemId = itemId}});
        } else {
            emitApplicationEvent({.payload = ConversationItemStopped{
                .itemId = itemId,
                .state = terminalStatus == QStringLiteral("cancelled")
                    ? suprai::domain::ConversationItemState::Cancelled
                    : suprai::domain::ConversationItemState::Failed,
            }});
        }
    }
    setState(RuntimeState::Ready);
    if (!providerError.isEmpty()) {
        emitApplicationEvent({.payload = RuntimeError{.message = providerError}});
    }
}

void RuntimeOrchestrator::clearActiveTurn()
{
    m_activeAssistantId.clear();
    m_activeAssistantText.clear();
    m_activeInput.reset();
    m_activeTurn.reset();
    m_activeRun.reset();
    m_pendingUserItem.reset();
    m_pendingPersistenceRequestId.clear();
    m_pendingTerminal.reset();
    m_pendingProviderError.clear();
    m_cancelBeforeInference = false;
}

void RuntimeOrchestrator::setState(RuntimeState state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    emitApplicationEvent({
        .payload = RuntimeStateChanged{
            .state = state,
        },
    });
}

void RuntimeOrchestrator::setCapabilities(
    const suprai::runtime::RuntimeCapabilities &capabilities)
{
    if (m_capabilities == capabilities) {
        return;
    }

    m_capabilities = capabilities;
    emitApplicationEvent({
        .payload = RuntimeCapabilitiesChanged{
            .capabilities = m_capabilities,
        },
    });
}

void RuntimeOrchestrator::emitApplicationEvent(
    suprai::runtime::RuntimeApplicationEvent event)
{
    emit eventOccurred(event);
}

} // namespace suprai::runtime::internal
