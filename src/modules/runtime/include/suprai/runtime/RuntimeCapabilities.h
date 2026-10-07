#pragma once

#include <QMetaType>
#include <QString>

namespace suprai::runtime {

enum class CapabilityState {
    Unknown,
    Supported,
    Unsupported
};

struct RuntimeCapabilities {
    CapabilityState textGeneration = CapabilityState::Unknown;
    CapabilityState toolCalling = CapabilityState::Unknown;
    CapabilityState imageInput = CapabilityState::Unknown;
    CapabilityState reasoningOutput = CapabilityState::Unknown;
    CapabilityState exactInputTokenCounting = CapabilityState::Unknown;

    friend bool operator==(const RuntimeCapabilities &, const RuntimeCapabilities &) = default;
};

inline QString capabilityStateName(CapabilityState state)
{
    switch (state) {
    case CapabilityState::Unknown:
        return QStringLiteral("Unknown");
    case CapabilityState::Supported:
        return QStringLiteral("Supported");
    case CapabilityState::Unsupported:
        return QStringLiteral("Unsupported");
    }
    return QStringLiteral("Unknown");
}

} // namespace suprai::runtime

Q_DECLARE_METATYPE(suprai::runtime::RuntimeCapabilities)
