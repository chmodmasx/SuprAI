#include "MockRuntime.h"

#include <suprai/domain/ConversationItem.h>

#include <QTimer>

namespace suprai::runtime::internal {

MockRuntime::MockRuntime(QObject *parent)
    : AgentRuntime(parent)
{
}

void MockRuntime::start()
{
    if (m_state != RuntimeState::Stopped) {
        return;
    }

    setState(RuntimeState::Starting);

    m_timer = new QTimer(this);
    m_timer->setInterval(35);
    connect(m_timer, &QTimer::timeout, this, &MockRuntime::emitNextChunk);

    setState(RuntimeState::Ready);
}

void MockRuntime::shutdown()
{
    if (m_timer) {
        m_timer->stop();
    }
    setState(RuntimeState::Stopped);
    emit stopped();
}

void MockRuntime::submitPrompt(const QString &prompt)
{
    const QString text = prompt.trimmed();
    if (text.isEmpty() || m_state != RuntimeState::Ready) {
        return;
    }

    emit userMessageAccepted(suprai::domain::newItemId(), text);

    m_activeId = suprai::domain::newItemId();
    m_text.clear();
    m_index = 0;
    m_chunks = {
        QStringLiteral("Este es el MockRuntime. "),
        QStringLiteral("La UI, el streaming, el runtime y el provider están desacoplados. "),
        QStringLiteral("Cambiá SUPRAI_RUNTIME=native para usar un endpoint OpenAI-compatible real."),
    };

    emit assistantMessageStarted(m_activeId);
    emit reasoningActiveChanged(true);
    setState(RuntimeState::Working);
    m_timer->start();
}

void MockRuntime::cancelTurn()
{
    if (m_state != RuntimeState::Working) {
        return;
    }

    setState(RuntimeState::Cancelling);
    if (m_timer) {
        m_timer->stop();
    }
    emit reasoningActiveChanged(false);
    emit assistantMessageCompleted(m_activeId, m_text);
    m_activeId.clear();
    m_text.clear();
    setState(RuntimeState::Ready);
}

void MockRuntime::resetSession()
{
    if (m_state == RuntimeState::Working || m_state == RuntimeState::Cancelling) {
        return;
    }
    emit conversationReset();
}

void MockRuntime::emitNextChunk()
{
    if (m_index >= m_chunks.size()) {
        finish();
        return;
    }

    if (m_index == 1) {
        emit reasoningActiveChanged(false);
    }

    const auto chunk = m_chunks.at(m_index++);
    m_text += chunk;
    emit assistantTextDelta(m_activeId, chunk);

    if (m_index >= m_chunks.size()) {
        finish();
    }
}

void MockRuntime::finish()
{
    if (m_timer) {
        m_timer->stop();
    }

    emit reasoningActiveChanged(false);
    emit assistantMessageCompleted(m_activeId, m_text);
    m_activeId.clear();
    m_text.clear();
    setState(RuntimeState::Ready);
}

void MockRuntime::setState(RuntimeState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged(state);
}

} // namespace suprai::runtime::internal
