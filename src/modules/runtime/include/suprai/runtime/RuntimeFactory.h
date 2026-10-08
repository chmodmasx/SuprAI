#pragma once

#include <suprai/runtime/RuntimeConfig.h>

class QObject;

namespace suprai::providers {
class Provider;
}

namespace suprai::persistence {
class PersistencePort;
}

namespace suprai::runtime {

class AgentRuntime;

AgentRuntime *createNativeRuntime(
    AgentRuntimeConfig config,
    suprai::providers::Provider *provider,
    suprai::persistence::PersistencePort *persistence = nullptr,
    QObject *parent = nullptr);

AgentRuntime *createMockRuntime(QObject *parent = nullptr);

} // namespace suprai::runtime
