#include "RuntimeEventAdapter.h"

#include "AgentEngine.h"

namespace suprai::runtime::internal {

RuntimeEventAdapter::RuntimeEventAdapter(
    AgentEngine *engine,
    QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(engine);

    if (engine) {
        connect(engine, &AgentEngine::engineEvent,
                this, &RuntimeEventAdapter::translate);
    }
}

void RuntimeEventAdapter::translate(const AgentEngineEvent &event)
{
    RuntimeEvent translated;

    switch (event.type) {
    case AgentEngineEventType::TextDelta:
        translated.type = RuntimeEventType::AssistantTextDelta;
        translated.payload = event.payload;
        break;
    case AgentEngineEventType::ReasoningDelta:
        translated.type = RuntimeEventType::ReasoningDelta;
        translated.payload = event.payload;
        break;
    case AgentEngineEventType::Completed:
        translated.type = RuntimeEventType::ProviderCompleted;
        break;
    case AgentEngineEventType::Failed:
        translated.type = RuntimeEventType::ProviderFailed;
        translated.payload = event.payload;
        break;
    case AgentEngineEventType::Cancelled:
        translated.type = RuntimeEventType::ProviderCancelled;
        break;
    }

    emit runtimeEvent(translated);
}

} // namespace suprai::runtime::internal
