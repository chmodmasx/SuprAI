#pragma once

#include <suprai/runtime/ApplicationEvent.h>

#include <QObject>
#include <QString>

namespace suprai::runtime {

class AgentRuntime : public QObject
{
    Q_OBJECT

public:
    explicit AgentRuntime(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~AgentRuntime() override = default;

public slots:
    virtual void start() = 0;
    virtual void shutdown() = 0;
    virtual void submitPrompt(const QString &prompt) = 0;
    virtual void cancelTurn() = 0;
    virtual void resetSession() = 0;

signals:
    void eventEmitted(const suprai::runtime::ApplicationEvent &event);
    void stopped();
};

} // namespace suprai::runtime
