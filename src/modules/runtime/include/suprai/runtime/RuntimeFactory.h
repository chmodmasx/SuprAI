#pragma once

#include <suprai/runtime/RuntimeConfig.h>

class QObject;

namespace suprai::providers {
class Provider;
}

namespace suprai::runtime {

class AgentRuntime;

AgentRuntime *createNativeRuntime(
    AgentRuntimeConfig config,
    suprai::providers::Provider *provider,
    QObject *parent = nullptr);

AgentRuntime *createMockRuntime(QObject *parent = nullptr);

} // namespace suprai::runtime
