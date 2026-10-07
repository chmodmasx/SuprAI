#pragma once

#include <suprai/runtime/RuntimeCapabilities.h>
#include <suprai/runtime/RuntimeState.h>

#include <QMetaType>
#include <QString>

#include <variant>

namespace suprai::runtime {

struct RuntimeStateChanged {
    RuntimeState state = RuntimeState::Stopped;
};

struct RuntimeCapabilitiesChanged {
    RuntimeCapabilities capabilities;
};

struct UserMessageAccepted {
    QString itemId;
    QString text;
};

struct AssistantMessageStarted {
    QString itemId;
};

struct AssistantTextDelta {
    QString itemId;
    QString delta;
};

struct AssistantMessageCompleted {
    QString itemId;
    QString finalText;
};

struct ReasoningActiveChanged {
    bool active = false;
};

struct ConversationReset {
};

struct RuntimeError {
    QString message;
};

using RuntimeApplicationEventPayload = std::variant<
    RuntimeStateChanged,
    RuntimeCapabilitiesChanged,
    UserMessageAccepted,
    AssistantMessageStarted,
    AssistantTextDelta,
    AssistantMessageCompleted,
    ReasoningActiveChanged,
    ConversationReset,
    RuntimeError>;

struct RuntimeApplicationEvent {
    RuntimeApplicationEventPayload payload;
};

template <typename T>
inline const T *eventPayload(const RuntimeApplicationEvent &event)
{
    return std::get_if<T>(&event.payload);
}

} // namespace suprai::runtime

Q_DECLARE_METATYPE(suprai::runtime::RuntimeApplicationEvent)
