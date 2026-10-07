#pragma once

#include "AgentEngineEvent.h"
#include "RuntimeEvent.h"

#include <QObject>

namespace suprai::runtime::internal {

class AgentEngine;

class RuntimeEventAdapter final : public QObject
{
    Q_OBJECT

public:
    explicit RuntimeEventAdapter(
        AgentEngine *engine,
        QObject *parent = nullptr);

signals:
    void runtimeEvent(const suprai::runtime::internal::RuntimeEvent &event);

private:
    void translate(const suprai::runtime::internal::AgentEngineEvent &event);
};

} // namespace suprai::runtime::internal
