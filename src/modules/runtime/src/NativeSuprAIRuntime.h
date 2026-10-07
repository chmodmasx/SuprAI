#pragma once

#include <suprai/providers/Provider.h>
#include <suprai/runtime/AgentRuntime.h>
#include <suprai/runtime/RuntimeConfig.h>

#include <QString>
#include <QVector>

namespace suprai::runtime::internal {

struct RuntimeMessage {
    QString role;
    QString content;
};

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
    void setState(suprai::runtime::RuntimeState state);
    void connectProvider();
    void finishAssistant();
    QVector<suprai::providers::ProviderMessage> providerMessages() const;

    suprai::runtime::AgentRuntimeConfig m_config;
    suprai::providers::Provider *m_provider = nullptr;
    QVector<RuntimeMessage> m_history;
    suprai::runtime::RuntimeState m_state = suprai::runtime::RuntimeState::Stopped;
    QString m_activeAssistantId;
    QString m_activeAssistantText;
    bool m_reasoningActive = false;
};

} // namespace suprai::runtime::internal
