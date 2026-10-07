#pragma once

#include <QString>

namespace suprai::runtime::internal {

enum class AgentEngineEventType {
    TextDelta,
    ReasoningDelta,
    Completed,
    Failed,
    Cancelled
};

struct AgentEngineEvent {
    AgentEngineEventType type;
    QString payload;
};

} // namespace suprai::runtime::internal
