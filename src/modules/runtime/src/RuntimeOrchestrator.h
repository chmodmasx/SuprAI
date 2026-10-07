#pragma once

#include "RuntimeEvent.h"

#include <suprai/providers/Provider.h>
#include <suprai/runtime/RuntimeConfig.h>
#include <suprai/runtime/RuntimeState.h>

#include <QObject>
#include <QString>
#include <QVector>

namespace suprai::runtime::internal {

class AgentEngine;
class RuntimeEventAdapter;

struct RuntimeMessage {
    QString role;
    QString content;
};

class RuntimeOrchestrator final : public QObject
{
    Q_OBJECT

public:
    RuntimeOrchestrator(
        suprai::runtime::AgentRuntimeConfig config,
        AgentEngine *engine,
        QObject *parent = nullptr);

public slots:
    void start();
    void shutdown();
    void submitPrompt(const QString &prompt);
    void cancelTurn();
    void resetSession();

signals:
    void stateChanged(suprai::runtime::RuntimeState state);
    void userMessageAccepted(const QString &itemId, const QString &text);
    void assistantMessageStarted(const QString &itemId);
    void assistantTextDelta(const QString &itemId, const QString &delta);
    void assistantMessageCompleted(const QString &itemId, const QString &finalText);
    void reasoningActiveChanged(bool active);
    void conversationReset();
    void errorOccurred(const QString &message);
    void stopped();

private:
    void handleRuntimeEvent(const suprai::runtime::internal::RuntimeEvent &event);
    void setState(suprai::runtime::RuntimeState state);
    void finishAssistant(bool persistAnswer);
    suprai::providers::ProviderRequest providerRequest() const;

    suprai::runtime::AgentRuntimeConfig m_config;
    AgentEngine *m_engine = nullptr;
    RuntimeEventAdapter *m_eventAdapter = nullptr;
    QVector<RuntimeMessage> m_history;
    suprai::runtime::RuntimeState m_state = suprai::runtime::RuntimeState::Stopped;
    QString m_activeAssistantId;
    QString m_activeAssistantText;
    bool m_reasoningActive = false;
};

} // namespace suprai::runtime::internal
