#pragma once

#include "AppConfig.h"

namespace suprai::runtime {
class AgentRuntime;
}

namespace suprai::app {

class ApplicationBootstrap
{
public:
    static suprai::runtime::AgentRuntime *createRuntime(const AppConfig &config);
};

} // namespace suprai::app
