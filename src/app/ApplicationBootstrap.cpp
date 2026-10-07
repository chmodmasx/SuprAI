#include "app/ApplicationBootstrap.h"

#include "providers/OpenAIChatProvider.h"
#include "runtime/AgentRuntime.h"
#include "runtime/MockRuntime.h"
#include "runtime/NativeSuprAIRuntime.h"

namespace suprai::app {

suprai::runtime::AgentRuntime *ApplicationBootstrap::createRuntime(
    const suprai::runtime::RuntimeConfig &config)
{
    if (config.mode.compare(QStringLiteral("mock"), Qt::CaseInsensitive) == 0) {
        return new suprai::runtime::MockRuntime;
    }

    auto *provider = new suprai::providers::OpenAIChatProvider({
        .baseUrl = config.baseUrl,
        .apiKey = config.apiKey,
    });

    return new suprai::runtime::NativeSuprAIRuntime(config.agentConfig(), provider);
}

} // namespace suprai::app
