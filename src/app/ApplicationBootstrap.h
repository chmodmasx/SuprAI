#pragma once

#include "AppConfig.h"

namespace suprai::runtime {
class AgentRuntime;
}

namespace suprai::persistence {
class PersistencePort;
class PersistenceWorker;
}

namespace suprai::app {

class ApplicationBootstrap
{
public:
    static suprai::runtime::AgentRuntime *createRuntime(
        const AppConfig &config,
        suprai::persistence::PersistencePort *persistence = nullptr);

    static suprai::persistence::PersistenceWorker *createPersistenceWorker(
        const QString &stateDirectory);

    static suprai::persistence::PersistencePort *createPersistencePort();
};

} // namespace suprai::app
