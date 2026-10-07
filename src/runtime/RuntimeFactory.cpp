#include "runtime/RuntimeFactory.h"

#include "runtime/AgentRuntime.h"
#include "runtime/MockRuntime.h"
#include "runtime/NativeSuprAIRuntime.h"

namespace suprai::runtime {

AgentRuntime *RuntimeFactory::create(const RuntimeConfig &config)
{
    if (config.mode.compare(QStringLiteral("mock"), Qt::CaseInsensitive) == 0) {
        return new MockRuntime;
    }

    return new NativeSuprAIRuntime(config);
}

} // namespace suprai::runtime
