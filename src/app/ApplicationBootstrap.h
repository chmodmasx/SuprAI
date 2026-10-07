#pragma once

#include "AppConfig.h"

namespace suprai::runtime {
class AgentRuntime;
}

namespace suprai::persistence {
class PersistenceWorker;
}

namespace suprai::app {

class ApplicationBootstrap
{
public:
    static suprai::runtime::AgentRuntime *createRuntime(const AppConfig &config);
    static suprai::persistence::PersistenceWorker *createPersistenceWorker(
        const QString &stateDirectory);
};

} // namespace suprai::app
