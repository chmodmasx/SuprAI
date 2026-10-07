#pragma once

#include <suprai/domain/ConversationItem.h>
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

struct ConversationItemStarted {
    suprai::domain::ConversationItem item;
};

struct ConversationTextDelta {
    QString itemId;
    QString delta;
};

struct ConversationItemCompleted {
    QString itemId;
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
    ConversationItemStarted,
    ConversationTextDelta,
    ConversationItemCompleted,
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
