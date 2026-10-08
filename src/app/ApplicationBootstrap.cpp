#include "ApplicationBootstrap.h"

#include <suprai/persistence/PersistencePort.h>
#include <suprai/persistence/PersistenceWorker.h>
#include <suprai/providers/OpenAIProviderFactory.h>
#include <suprai/runtime/RuntimeFactory.h>

namespace suprai::app {

suprai::runtime::AgentRuntime *ApplicationBootstrap::createRuntime(
    const AppConfig &config,
    suprai::persistence::PersistencePort *persistence)
{
    if (config.runtimeMode.compare(QStringLiteral("mock"), Qt::CaseInsensitive) == 0) {
        return suprai::runtime::createMockRuntime();
    }

    auto *provider = suprai::providers::createOpenAIChatProvider({
        .baseUrl = config.baseUrl,
        .apiKey = config.apiKey,
    });

    return suprai::runtime::createNativeRuntime(
        config.agentConfig(),
        provider,
        persistence);
}

suprai::persistence::PersistenceWorker *ApplicationBootstrap::createPersistenceWorker(
    const QString &stateDirectory)
{
    return new suprai::persistence::PersistenceWorker(stateDirectory);
}

suprai::persistence::PersistencePort *ApplicationBootstrap::createPersistencePort()
{
    return new suprai::persistence::PersistencePort;
}

} // namespace suprai::app
