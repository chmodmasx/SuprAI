#include "RuntimeOrchestrator.h"

#include "AgentEngine.h"
#include "RuntimeEventAdapter.h"

#include <suprai/domain/ConversationItem.h>

#include <utility>

namespace suprai::runtime::internal {

RuntimeOrchestrator::RuntimeOrchestrator(
    suprai::runtime::AgentRuntimeConfig config,
    AgentEngine *engine,
    QObject *parent)
    : QObject(parent)
    , m_config(std::move(config))
    , m_engine(engine)
    , m_eventAdapter(new RuntimeEventAdapter(engine, this))
{
    Q_ASSERT(m_engine);

    connect(m_eventAdapter, &RuntimeEventAdapter::runtimeEvent,
            this, &RuntimeOrchestrator::handleRuntimeEvent);
}

void RuntimeOrchestrator::start()
{
    if (m_state != RuntimeState::Stopped) {
        return;
    }

    setState(RuntimeState::Starting);

    if (!m_engine) {
        setState(RuntimeState::Failed);
        emit errorOccurred(QStringLiteral("No hay un motor de agente configurado."));
        return;
    }

    setState(RuntimeState::Ready);
}

void RuntimeOrchestrator::shutdown()
{
    if (m_engine && m_engine->isBusy()) {
        m_engine->cancel();
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

    for (const auto &message : m_history) {
        messages.push_back({
            .role = message.role,
            .content = message.content,
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
    m_engine->generate(providerRequest());
}

void RuntimeOrchestrator::cancelTurn()
{
    if (m_state != RuntimeState::Working || !m_engine) {
        return;
    }

    setState(RuntimeState::Cancelling);
    m_engine->cancel();
}

void RuntimeOrchestrator::resetSession()
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

void RuntimeOrchestrator::handleRuntimeEvent(const RuntimeEvent &event)
{
    switch (event.type) {
    case RuntimeEventType::AssistantTextDelta:
        if (!m_activeAssistantId.isEmpty()) {
            m_activeAssistantText += event.payload;
            emit assistantTextDelta(m_activeAssistantId, event.payload);
        }
        break;

    case RuntimeEventType::ReasoningDelta:
        if (!m_reasoningActive) {
            m_reasoningActive = true;
            emit reasoningActiveChanged(true);
        }
        break;

    case RuntimeEventType::ProviderCompleted:
        finishAssistant(true);
        setState(RuntimeState::Ready);
        break;

    case RuntimeEventType::ProviderCancelled:
        finishAssistant(false);
        setState(RuntimeState::Ready);
        break;

    case RuntimeEventType::ProviderFailed:
        finishAssistant(false);
        setState(RuntimeState::Ready);
        emit errorOccurred(event.payload);
        break;
    }
}

void RuntimeOrchestrator::finishAssistant(bool persistAnswer)
{
    if (m_reasoningActive) {
        m_reasoningActive = false;
        emit reasoningActiveChanged(false);
    }

    if (m_activeAssistantId.isEmpty()) {
        return;
    }

    if (persistAnswer) {
        m_history.push_back({
            .role = QStringLiteral("assistant"),
            .content = m_activeAssistantText,
        });
    }

    emit assistantMessageCompleted(m_activeAssistantId, m_activeAssistantText);
    m_activeAssistantId.clear();
    m_activeAssistantText.clear();
}

void RuntimeOrchestrator::setState(RuntimeState state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    emit stateChanged(state);
}

} // namespace suprai::runtime::internal
