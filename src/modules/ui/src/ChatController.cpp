#include <suprai/ui/ChatController.h>

#include "TranscriptModel.h"

#include <suprai/domain/ConversationItem.h>
#include <suprai/runtime/AgentRuntime.h>

#include <QMetaObject>

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
    connect(m_runtime, &suprai::runtime::AgentRuntime::stateChanged,
            this, &ChatController::setRuntimeState);

    connect(m_runtime, &suprai::runtime::AgentRuntime::userMessageAccepted,
            this, [this](const QString &id, const QString &text) {
                m_transcript->append({
                    .id = id,
                    .role = suprai::domain::ConversationRole::User,
                    .text = text,
                    .streaming = false,
                });
            });

    connect(m_runtime, &suprai::runtime::AgentRuntime::assistantMessageStarted,
            this, [this](const QString &id) {
                m_transcript->append({
                    .id = id,
                    .role = suprai::domain::ConversationRole::Assistant,
                    .text = {},
                    .streaming = true,
                });
            });

    connect(m_runtime, &suprai::runtime::AgentRuntime::assistantTextDelta,
            this, [this](const QString &id, const QString &delta) {
                m_transcript->appendDelta(id, delta);
            });

    connect(m_runtime, &suprai::runtime::AgentRuntime::assistantMessageCompleted,
            this, [this](const QString &id, const QString &) {
                m_transcript->finish(id);
            });

    connect(m_runtime, &suprai::runtime::AgentRuntime::reasoningActiveChanged,
            this, &ChatController::setReasoning);

    connect(m_runtime, &suprai::runtime::AgentRuntime::conversationReset,
            m_transcript, &internal::TranscriptModel::clear);

    connect(m_runtime, &suprai::runtime::AgentRuntime::errorOccurred,
            this, &ChatController::setLastError);
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
