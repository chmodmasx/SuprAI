#pragma once

#include <suprai/domain/ConversationItem.h>
#include <suprai/runtime/RuntimeState.h>

#include <QMetaType>
#include <QString>

#include <type_traits>
#include <variant>

namespace suprai::runtime {

enum class ApplicationEventKind {
    RuntimeStateChanged,
    ConversationItemStarted,
    ConversationTextDelta,
    ConversationItemCompleted,
    ReasoningActivityChanged,
    ConversationReset,
    Error
};

struct RuntimeStateChangedEvent {
    RuntimeState state = RuntimeState::Stopped;
};

struct ConversationItemStartedEvent {
    suprai::domain::ConversationItem item;
};

struct ConversationTextDeltaEvent {
    QString itemId;
    QString delta;
};

struct ConversationItemCompletedEvent {
    QString itemId;
};

struct ReasoningActivityChangedEvent {
    bool active = false;
};

struct ConversationResetEvent {
};

struct RuntimeErrorEvent {
    QString message;
};

using ApplicationEventPayload = std::variant<
    RuntimeStateChangedEvent,
    ConversationItemStartedEvent,
    ConversationTextDeltaEvent,
    ConversationItemCompletedEvent,
    ReasoningActivityChangedEvent,
    ConversationResetEvent,
    RuntimeErrorEvent>;

struct ApplicationEvent {
    ApplicationEventPayload payload;
};

inline ApplicationEventKind applicationEventKind(const ApplicationEvent &event)
{
    return std::visit(
        [](const auto &payload) -> ApplicationEventKind {
            using T = std::decay_t<decltype(payload)>;

            if constexpr (std::is_same_v<T, RuntimeStateChangedEvent>) {
                return ApplicationEventKind::RuntimeStateChanged;
            } else if constexpr (std::is_same_v<T, ConversationItemStartedEvent>) {
                return ApplicationEventKind::ConversationItemStarted;
            } else if constexpr (std::is_same_v<T, ConversationTextDeltaEvent>) {
                return ApplicationEventKind::ConversationTextDelta;
            } else if constexpr (std::is_same_v<T, ConversationItemCompletedEvent>) {
                return ApplicationEventKind::ConversationItemCompleted;
            } else if constexpr (std::is_same_v<T, ReasoningActivityChangedEvent>) {
                return ApplicationEventKind::ReasoningActivityChanged;
            } else if constexpr (std::is_same_v<T, ConversationResetEvent>) {
                return ApplicationEventKind::ConversationReset;
            } else {
                return ApplicationEventKind::Error;
            }
        },
        event.payload);
}

} // namespace suprai::runtime

Q_DECLARE_METATYPE(suprai::runtime::ApplicationEvent)
