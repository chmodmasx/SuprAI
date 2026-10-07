#include "NativeSuprAIRuntime.h"

#include <suprai/domain/ConversationItem.h>

#include <utility>

namespace suprai::runtime::internal {

NativeSuprAIRuntime::NativeSuprAIRuntime(
    suprai::runtime::AgentRuntimeConfig config,
    suprai::providers::Provider *provider,
    QObject *parent)
    : AgentRuntime(parent)
    , m_config(std::move(config))
    , m_provider(provider)
{
    Q_ASSERT(m_provider);

    if (m_provider && !m_provider->parent()) {
        m_provider->setParent(this);
    }

    connectProvider();
}

void NativeSuprAIRuntime::start()
{
    if (m_state != RuntimeState::Stopped) {
        return;
    }

    setState(RuntimeState::Starting);

    if (!m_provider) {
        setState(RuntimeState::Failed);
        emit errorOccurred(QStringLiteral("No hay un provider configurado."));
        return;
    }

    setState(RuntimeState::Ready);
}

void NativeSuprAIRuntime::shutdown()
{
    if (m_provider && m_provider->isBusy()) {
        m_provider->cancel();
    }

    m_activeAssistantId.clear();
    m_activeAssistantText.clear();

    if (m_reasoningActive) {
        m_reasoningActive = false;
        emit reasoningActiveChanged(false);
    }

    setState(RuntimeState::Stopped);
    emit stopped();
}

QVector<suprai::providers::ProviderMessage> NativeSuprAIRuntime::providerMessages() const
{
    QVector<suprai::providers::ProviderMessage> messages;
    messages.reserve(m_history.size() + 1);

    if (!m_config.systemPrompt.trimmed().isEmpty()) {
        messages.push_back({
            .role = QStringLiteral("system"),
            .content = m_config.systemPrompt,
        });
    }

    for (const auto &message : m_history) {
        messages.push_back({
            .role = message.role,
            .content = message.content,
        });
    }

    return messages;
}

void NativeSuprAIRuntime::submitPrompt(const QString &prompt)
{
    const QString text = prompt.trimmed();
    if (text.isEmpty()) {
        return;
    }

    if (m_state != RuntimeState::Ready || !m_provider) {
        emit errorOccurred(QStringLiteral("El runtime no está listo."));
        return;
    }

    const QString userId = suprai::domain::newItemId();
    m_history.push_back({
        .role = QStringLiteral("user"),
        .content = text,
    });
    emit userMessageAccepted(userId, text);

    m_activeAssistantId = suprai::domain::newItemId();
    m_activeAssistantText.clear();
    emit assistantMessageStarted(m_activeAssistantId);

    setState(RuntimeState::Working);

    suprai::providers::ProviderRequest request{
        .model = m_config.model,
        .messages = providerMessages(),
    };
    m_provider->generate(request);
}

void NativeSuprAIRuntime::cancelTurn()
{
    if (m_state != RuntimeState::Working || !m_provider) {
        return;
    }

    setState(RuntimeState::Cancelling);
    m_provider->cancel();
}

void NativeSuprAIRuntime::resetSession()
{
    if (m_state == RuntimeState::Working || m_state == RuntimeState::Cancelling) {
        emit errorOccurred(QStringLiteral("Cancelá el turno activo antes de iniciar una conversación nueva."));
        return;
    }

    m_history.clear();
    m_activeAssistantId.clear();
    m_activeAssistantText.clear();
    emit conversationReset();
}

void NativeSuprAIRuntime::connectProvider()
{
    if (!m_provider) {
        return;
    }

    connect(m_provider, &suprai::providers::Provider::textDelta, this, [this](const QString &delta) {
        if (m_activeAssistantId.isEmpty()) {
            return;
        }
        m_activeAssistantText += delta;
        emit assistantTextDelta(m_activeAssistantId, delta);
    });

    connect(m_provider, &suprai::providers::Provider::reasoningDelta, this, [this](const QString &) {
        if (!m_reasoningActive) {
            m_reasoningActive = true;
            emit reasoningActiveChanged(true);
        }
    });

    connect(m_provider, &suprai::providers::Provider::completed, this, [this] {
        finishAssistant();
        setState(RuntimeState::Ready);
    });

    connect(m_provider, &suprai::providers::Provider::cancelled, this, [this] {
        if (m_reasoningActive) {
            m_reasoningActive = false;
            emit reasoningActiveChanged(false);
        }

        if (!m_activeAssistantId.isEmpty()) {
            emit assistantMessageCompleted(m_activeAssistantId, m_activeAssistantText);
        }

        m_activeAssistantId.clear();
        m_activeAssistantText.clear();
        setState(RuntimeState::Ready);
    });

    connect(m_provider, &suprai::providers::Provider::failed, this, [this](const QString &message) {
        if (m_reasoningActive) {
            m_reasoningActive = false;
            emit reasoningActiveChanged(false);
        }

        if (!m_activeAssistantId.isEmpty()) {
            emit assistantMessageCompleted(m_activeAssistantId, m_activeAssistantText);
        }

        m_activeAssistantId.clear();
        m_activeAssistantText.clear();
        setState(RuntimeState::Ready);
        emit errorOccurred(message);
    });
}

void NativeSuprAIRuntime::finishAssistant()
{
    if (m_reasoningActive) {
        m_reasoningActive = false;
        emit reasoningActiveChanged(false);
    }

    if (m_activeAssistantId.isEmpty()) {
        return;
    }

    m_history.push_back({
        .role = QStringLiteral("assistant"),
        .content = m_activeAssistantText,
    });

    emit assistantMessageCompleted(m_activeAssistantId, m_activeAssistantText);
    m_activeAssistantId.clear();
    m_activeAssistantText.clear();
}

void NativeSuprAIRuntime::setState(RuntimeState state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    emit stateChanged(state);
}

} // namespace suprai::runtime::internal
