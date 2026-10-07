#include <suprai/ui/ChatController.h>

#include "TranscriptModel.h"

#include <suprai/domain/ConversationItem.h>
#include <suprai/runtime/AgentRuntime.h>

#include <QMetaObject>

#include <type_traits>
#include <variant>

namespace suprai::ui {

ChatController::ChatController(suprai::runtime::AgentRuntime *runtime, QObject *parent)
    : QObject(parent)
    , m_runtime(runtime)
    , m_transcript(new internal::TranscriptModel(this))
{
    connectRuntime();
}

QAbstractItemModel *ChatController::transcript() const
{
    return m_transcript;
}

QString ChatController::runtimeState() const
{
    return suprai::runtime::runtimeStateName(m_runtimeState);
}

bool ChatController::busy() const
{
    return m_runtimeState == suprai::runtime::RuntimeState::Working
        || m_runtimeState == suprai::runtime::RuntimeState::Cancelling
        || m_runtimeState == suprai::runtime::RuntimeState::Starting;
}

bool ChatController::reasoning() const
{
    return m_reasoning;
}

QString ChatController::lastError() const
{
    return m_lastError;
}

void ChatController::sendMessage(const QString &text)
{
    if (!m_runtime || busy() || text.trimmed().isEmpty()) {
        return;
    }

    clearError();
    const QString prompt = text;
    QMetaObject::invokeMethod(
        m_runtime,
        [runtime = m_runtime, prompt] {
            runtime->submitPrompt(prompt);
        },
        Qt::QueuedConnection);
}

void ChatController::cancel()
{
    if (!m_runtime || !busy()) {
        return;
    }

    QMetaObject::invokeMethod(
        m_runtime,
        [runtime = m_runtime] {
            runtime->cancelTurn();
        },
        Qt::QueuedConnection);
}

void ChatController::newConversation()
{
    if (!m_runtime || busy()) {
        return;
    }

    clearError();
    QMetaObject::invokeMethod(
        m_runtime,
        [runtime = m_runtime] {
            runtime->resetSession();
        },
        Qt::QueuedConnection);
}

void ChatController::clearError()
{
    setLastError({});
}

void ChatController::connectRuntime()
{
    connect(m_runtime, &suprai::runtime::AgentRuntime::eventEmitted,
            this, &ChatController::handleApplicationEvent);
}

void ChatController::handleApplicationEvent(const suprai::runtime::ApplicationEvent &event)
{
    std::visit(
        [this](const auto &payload) {
            using T = std::decay_t<decltype(payload)>;

            if constexpr (std::is_same_v<T, suprai::runtime::RuntimeStateChangedEvent>) {
                setRuntimeState(payload.state);
            } else if constexpr (std::is_same_v<T, suprai::runtime::ConversationItemStartedEvent>) {
                m_transcript->append(payload.item);
            } else if constexpr (std::is_same_v<T, suprai::runtime::ConversationTextDeltaEvent>) {
                m_transcript->appendDelta(payload.itemId, payload.delta);
            } else if constexpr (std::is_same_v<T, suprai::runtime::ConversationItemCompletedEvent>) {
                m_transcript->finish(payload.itemId);
            } else if constexpr (std::is_same_v<T, suprai::runtime::ReasoningActivityChangedEvent>) {
                setReasoning(payload.active);
            } else if constexpr (std::is_same_v<T, suprai::runtime::ConversationResetEvent>) {
                m_transcript->clear();
            } else if constexpr (std::is_same_v<T, suprai::runtime::RuntimeErrorEvent>) {
                setLastError(payload.message);
            }
        },
        event.payload);
}

void ChatController::setRuntimeState(suprai::runtime::RuntimeState state)
{
    const bool wasBusy = busy();
    if (m_runtimeState == state) {
        return;
    }

    m_runtimeState = state;
    emit runtimeStateChanged();

    if (wasBusy != busy()) {
        emit busyChanged();
    }
}

void ChatController::setReasoning(bool active)
{
    if (m_reasoning == active) {
        return;
    }

    m_reasoning = active;
    emit reasoningChanged();
}

void ChatController::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
}

} // namespace suprai::ui
