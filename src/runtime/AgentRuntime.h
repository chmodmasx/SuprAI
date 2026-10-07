#pragma once

#include "runtime/RuntimeState.h"

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
    void stateChanged(suprai::runtime::RuntimeState state);
    void userMessageAccepted(const QString &itemId, const QString &text);
    void assistantMessageStarted(const QString &itemId);
    void assistantTextDelta(const QString &itemId, const QString &delta);
    void assistantMessageCompleted(const QString &itemId, const QString &finalText);
    void reasoningActiveChanged(bool active);
    void conversationReset();
    void errorOccurred(const QString &message);
    void stopped();
};

} // namespace suprai::runtime
