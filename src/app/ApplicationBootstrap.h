#pragma once

#include "runtime/RuntimeConfig.h"

namespace suprai::runtime {
class AgentRuntime;
}

namespace suprai::app {

class ApplicationBootstrap
{
public:
    static suprai::runtime::AgentRuntime *createRuntime(
        const suprai::runtime::RuntimeConfig &config);
};

} // namespace suprai::app
