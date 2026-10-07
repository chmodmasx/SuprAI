#pragma once

#include "AgentEngineEvent.h"

#include <suprai/providers/Provider.h>

#include <QObject>

namespace suprai::runtime::internal {

class AgentEngine final : public QObject
{
    Q_OBJECT

public:
    explicit AgentEngine(
        suprai::providers::Provider *provider,
        QObject *parent = nullptr);

    bool isBusy() const;

public slots:
    void generate(const suprai::providers::ProviderRequest &request);
    void cancel();

signals:
    void engineEvent(const suprai::runtime::internal::AgentEngineEvent &event);

private:
    void connectProvider();

    suprai::providers::Provider *m_provider = nullptr;
};

} // namespace suprai::runtime::internal
