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

QString ChatController::textGenerationCapability() const
{
    return suprai::runtime::capabilityStateName(m_capabilities.textGeneration);
}

QString ChatController::toolCallingCapability() const
{
    return suprai::runtime::capabilityStateName(m_capabilities.toolCalling);
}

QString ChatController::imageInputCapability() const
{
    return suprai::runtime::capabilityStateName(m_capabilities.imageInput);
}

QString ChatController::reasoningOutputCapability() const
{
    return suprai::runtime::capabilityStateName(m_capabilities.reasoningOutput);
}

QString ChatController::exactInputTokenCountingCapability() const
{
    return suprai::runtime::capabilityStateName(m_capabilities.exactInputTokenCounting);
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
    connect(m_runtime, &suprai::runtime::AgentRuntime::eventOccurred,
            this, &ChatController::handleRuntimeEvent);
}

void ChatController::handleRuntimeEvent(
    const suprai::runtime::RuntimeApplicationEvent &event)
{
    if (const auto *state = suprai::runtime::eventPayload<suprai::runtime::RuntimeStateChanged>(event)) {
        setRuntimeState(state->state);
        return;
    }

    if (const auto *capabilities =
            suprai::runtime::eventPayload<suprai::runtime::RuntimeCapabilitiesChanged>(event)) {
        setCapabilities(capabilities->capabilities);
        return;
    }

    if (const auto *accepted =
            suprai::runtime::eventPayload<suprai::runtime::UserMessageAccepted>(event)) {
        m_transcript->append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::User,
            accepted->text,
            suprai::domain::ConversationItemState::Completed,
            accepted->itemId));
        return;
    }

    if (const auto *started =
            suprai::runtime::eventPayload<suprai::runtime::AssistantMessageStarted>(event)) {
        m_transcript->append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant,
            {},
            suprai::domain::ConversationItemState::Streaming,
            started->itemId));
        return;
    }

    if (const auto *delta =
            suprai::runtime::eventPayload<suprai::runtime::AssistantTextDelta>(event)) {
        m_transcript->appendDelta(delta->itemId, delta->delta);
        return;
    }

    if (const auto *completed =
            suprai::runtime::eventPayload<suprai::runtime::AssistantMessageCompleted>(event)) {
        m_transcript->finish(completed->itemId);
        return;
    }

    if (const auto *reasoning =
            suprai::runtime::eventPayload<suprai::runtime::ReasoningActiveChanged>(event)) {
        setReasoning(reasoning->active);
        return;
    }

    if (suprai::runtime::eventPayload<suprai::runtime::ConversationReset>(event)) {
        m_transcript->clear();
        return;
    }

    if (const auto *error =
            suprai::runtime::eventPayload<suprai::runtime::RuntimeError>(event)) {
        setLastError(error->message);
    }
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

void ChatController::setCapabilities(
    const suprai::runtime::RuntimeCapabilities &capabilities)
{
    if (m_capabilities == capabilities) {
        return;
    }

    m_capabilities = capabilities;
    emit capabilitiesChanged();
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
