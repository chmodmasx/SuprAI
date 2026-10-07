#include "AgentEngine.h"

namespace suprai::runtime::internal {

AgentEngine::AgentEngine(
    suprai::providers::Provider *provider,
    QObject *parent)
    : QObject(parent)
    , m_provider(provider)
{
    if (m_provider && !m_provider->parent()) {
        m_provider->setParent(this);
    }

    connectProvider();
}

bool AgentEngine::isBusy() const
{
    return m_provider && m_provider->isBusy();
}

void AgentEngine::generate(const suprai::providers::ProviderRequest &request)
{
    if (!m_provider) {
        emit engineEvent({
            .type = AgentEngineEventType::Failed,
            .payload = QStringLiteral("No hay un provider configurado."),
        });
        return;
    }

    m_provider->generate(request);
}

void AgentEngine::cancel()
{
    if (m_provider && m_provider->isBusy()) {
        m_provider->cancel();
    }
}

void AgentEngine::connectProvider()
{
    if (!m_provider) {
        return;
    }

    connect(m_provider, &suprai::providers::Provider::textDelta,
            this, [this](const QString &delta) {
                emit engineEvent({
                    .type = AgentEngineEventType::TextDelta,
                    .payload = delta,
                });
            });

    connect(m_provider, &suprai::providers::Provider::reasoningDelta,
            this, [this](const QString &delta) {
                emit engineEvent({
                    .type = AgentEngineEventType::ReasoningDelta,
                    .payload = delta,
                });
            });

    connect(m_provider, &suprai::providers::Provider::completed,
            this, [this] {
                emit engineEvent({
                    .type = AgentEngineEventType::Completed,
                    .payload = {},
                });
            });

    connect(m_provider, &suprai::providers::Provider::failed,
            this, [this](const QString &message) {
                emit engineEvent({
                    .type = AgentEngineEventType::Failed,
                    .payload = message,
                });
            });

    connect(m_provider, &suprai::providers::Provider::cancelled,
            this, [this] {
                emit engineEvent({
                    .type = AgentEngineEventType::Cancelled,
                    .payload = {},
                });
            });
}

} // namespace suprai::runtime::internal
