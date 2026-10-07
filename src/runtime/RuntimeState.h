#pragma once

#include <QMetaType>
#include <QString>

namespace suprai::runtime {

enum class RuntimeState {
    Stopped,
    Starting,
    Ready,
    Working,
    Cancelling,
    Failed
};

inline QString runtimeStateName(RuntimeState state)
{
    switch (state) {
    case RuntimeState::Stopped:
        return QStringLiteral("Stopped");
    case RuntimeState::Starting:
        return QStringLiteral("Starting");
    case RuntimeState::Ready:
        return QStringLiteral("Ready");
    case RuntimeState::Working:
        return QStringLiteral("Working");
    case RuntimeState::Cancelling:
        return QStringLiteral("Cancelling");
    case RuntimeState::Failed:
        return QStringLiteral("Failed");
    }
    return QStringLiteral("Unknown");
}

} // namespace suprai::runtime

Q_DECLARE_METATYPE(suprai::runtime::RuntimeState)
