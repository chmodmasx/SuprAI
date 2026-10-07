#pragma once

#include "runtime/RuntimeConfig.h"

namespace suprai::runtime {

class AgentRuntime;

class RuntimeFactory
{
public:
    static AgentRuntime *create(const RuntimeConfig &config);
};

} // namespace suprai::runtime
