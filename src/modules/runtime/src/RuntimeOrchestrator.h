#pragma once

#include "RuntimeEvent.h"

#include <suprai/domain/ConversationItem.h>
#include <suprai/domain/ExecutionModel.h>
#include <suprai/persistence/PersistencePort.h>
#include <suprai/providers/Provider.h>
#include <suprai/runtime/RuntimeApplicationEvent.h>
#include <suprai/runtime/RuntimeConfig.h>
#include <suprai/runtime/RuntimeState.h>

#include <QObject>
#include <QString>
#include <QVector>

#include <optional>

namespace suprai::runtime::internal {

class AgentEngine;
class RuntimeEventAdapter;

class RuntimeOrchestrator final : public QObject
{
    Q_OBJECT

public:
    RuntimeOrchestrator(
        suprai::runtime::AgentRuntimeConfig config,
        AgentEngine *engine,
        suprai::persistence::PersistencePort *persistence = nullptr,
        QObject *parent = nullptr);

public:
    const suprai::domain::Session &session() const;
    const QVector<suprai::domain::Input> &inputs() const;
    const QVector<suprai::domain::Turn> &turns() const;
    const QVector<suprai::domain::Run> &runs() const;
    const QVector<suprai::domain::ConversationItem> &history() const;

public slots:
    void start();
    void shutdown();
    void submitPrompt(const QString &prompt);
    void cancelTurn();
    void resetSession();

signals:
    void eventOccurred(const suprai::runtime::RuntimeApplicationEvent &event);
    void stopped();

private:
    void handleRuntimeEvent(const suprai::runtime::internal::RuntimeEvent &event);
    void handleTurnStartPersisted(const QString &requestId);
    void handleSessionLoaded(suprai::persistence::SessionSnapshot snapshot);
    void handleSessionReadFailure(const QString &message);
    void handleTurnTerminalPersisted(const QString &requestId);
    void handlePersistenceFailure(const QString &requestId, const QString &message);
    void admitPendingTurnAndStartInference();
    void setState(suprai::runtime::RuntimeState state);
    void setCapabilities(const suprai::runtime::RuntimeCapabilities &capabilities);
    void emitApplicationEvent(suprai::runtime::RuntimeApplicationEvent event);
    void beginTerminalWrite(const QString &status, const QString &providerError = {});
    void finishDurableTerminal();
    suprai::providers::ProviderRequest providerRequest() const;
    void clearActiveTurn();

    suprai::runtime::AgentRuntimeConfig m_config;
    AgentEngine *m_engine = nullptr;
    RuntimeEventAdapter *m_eventAdapter = nullptr;
    suprai::persistence::PersistencePort *m_persistence = nullptr;
    suprai::domain::Session m_session;
    QVector<suprai::domain::Input> m_inputs;
    QVector<suprai::domain::Turn> m_turns;
    QVector<suprai::domain::Run> m_runs;
    QVector<suprai::domain::ConversationItem> m_history;
    std::optional<suprai::domain::Input> m_activeInput;
    std::optional<suprai::domain::Turn> m_activeTurn;
    std::optional<suprai::domain::Run> m_activeRun;
    std::optional<suprai::domain::ConversationItem> m_pendingUserItem;
    QString m_pendingPersistenceRequestId;
    std::optional<suprai::persistence::TurnTerminalWrite> m_pendingTerminal;
    QString m_pendingProviderError;
    suprai::runtime::RuntimeState m_state = suprai::runtime::RuntimeState::Stopped;
    suprai::runtime::RuntimeCapabilities m_capabilities;
    QString m_activeAssistantId;
    QString m_activeAssistantText;
    bool m_reasoningActive = false;
    bool m_cancelBeforeInference = false;
};

} // namespace suprai::runtime::internal
