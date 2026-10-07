#pragma once

#include "providers/Provider.h"
#include "runtime/AgentRuntime.h"
#include "runtime/RuntimeConfig.h"

#include <QString>
#include <QVector>

namespace suprai::runtime {

struct RuntimeMessage {
    QString role;
    QString content;
};

class NativeSuprAIRuntime final : public AgentRuntime
{
    Q_OBJECT

public:
    NativeSuprAIRuntime(
        AgentRuntimeConfig config,
        suprai::providers::Provider *provider,
        QObject *parent = nullptr);

public slots:
    void start() override;
    void shutdown() override;
    void submitPrompt(const QString &prompt) override;
    void cancelTurn() override;
    void resetSession() override;

private:
    void setState(RuntimeState state);
    void connectProvider();
    void finishAssistant();
    QVector<suprai::providers::ProviderMessage> providerMessages() const;

    AgentRuntimeConfig m_config;
    suprai::providers::Provider *m_provider = nullptr;
    QVector<RuntimeMessage> m_history;
    RuntimeState m_state = RuntimeState::Stopped;
    QString m_activeAssistantId;
    QString m_activeAssistantText;
    bool m_reasoningActive = false;
};

} // namespace suprai::runtime
