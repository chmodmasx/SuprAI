#include <suprai/runtime/RuntimeFactory.h>

#include "MockRuntime.h"
#include "NativeSuprAIRuntime.h"

#include <utility>

namespace suprai::runtime {

AgentRuntime *createNativeRuntime(
    AgentRuntimeConfig config,
    suprai::providers::Provider *provider,
    suprai::persistence::PersistencePort *persistence,
    QObject *parent)
{
    return new internal::NativeSuprAIRuntime(
        std::move(config),
        provider,
        persistence,
        parent);
}

AgentRuntime *createMockRuntime(QObject *parent)
{
    return new internal::MockRuntime(parent);
}

} // namespace suprai::runtime
