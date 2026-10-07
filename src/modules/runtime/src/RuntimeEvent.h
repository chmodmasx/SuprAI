#pragma once

#include <QString>

namespace suprai::runtime::internal {

enum class RuntimeEventType {
    AssistantTextDelta,
    ReasoningDelta,
    ProviderCompleted,
    ProviderFailed,
    ProviderCancelled
};

struct RuntimeEvent {
    RuntimeEventType type;
    QString payload;
};

} // namespace suprai::runtime::internal
