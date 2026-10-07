#pragma once

#include <suprai/runtime/ApplicationEvent.h>
#include <suprai/runtime/RuntimeState.h>

#include <QAbstractItemModel>
#include <QObject>
#include <QString>

namespace suprai::runtime {
class AgentRuntime;
}

namespace suprai::ui {

namespace internal {
class TranscriptModel;
}

class ChatController final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *transcript READ transcript CONSTANT)
    Q_PROPERTY(QString runtimeState READ runtimeState NOTIFY runtimeStateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool reasoning READ reasoning NOTIFY reasoningChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit ChatController(suprai::runtime::AgentRuntime *runtime, QObject *parent = nullptr);

    QAbstractItemModel *transcript() const;
    QString runtimeState() const;
    bool busy() const;
    bool reasoning() const;
    QString lastError() const;

    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void newConversation();
    Q_INVOKABLE void clearError();

signals:
    void runtimeStateChanged();
    void busyChanged();
    void reasoningChanged();
    void lastErrorChanged();

private:
    void connectRuntime();
    void handleApplicationEvent(const suprai::runtime::ApplicationEvent &event);
    void setRuntimeState(suprai::runtime::RuntimeState state);
    void setReasoning(bool active);
    void setLastError(const QString &message);

    suprai::runtime::AgentRuntime *m_runtime = nullptr;
    internal::TranscriptModel *m_transcript = nullptr;
    suprai::runtime::RuntimeState m_runtimeState = suprai::runtime::RuntimeState::Stopped;
    bool m_reasoning = false;
    QString m_lastError;
};

} // namespace suprai::ui
