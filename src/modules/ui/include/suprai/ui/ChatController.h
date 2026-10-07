#pragma once

#include <suprai/runtime/RuntimeApplicationEvent.h>
#include <suprai/runtime/RuntimeCapabilities.h>
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
    Q_PROPERTY(QString textGenerationCapability READ textGenerationCapability NOTIFY capabilitiesChanged)
    Q_PROPERTY(QString toolCallingCapability READ toolCallingCapability NOTIFY capabilitiesChanged)
    Q_PROPERTY(QString imageInputCapability READ imageInputCapability NOTIFY capabilitiesChanged)
    Q_PROPERTY(QString reasoningOutputCapability READ reasoningOutputCapability NOTIFY capabilitiesChanged)
    Q_PROPERTY(QString exactInputTokenCountingCapability READ exactInputTokenCountingCapability NOTIFY capabilitiesChanged)

public:
    explicit ChatController(suprai::runtime::AgentRuntime *runtime, QObject *parent = nullptr);

    QAbstractItemModel *transcript() const;
    QString runtimeState() const;
    bool busy() const;
    bool reasoning() const;
    QString lastError() const;

    QString textGenerationCapability() const;
    QString toolCallingCapability() const;
    QString imageInputCapability() const;
    QString reasoningOutputCapability() const;
    QString exactInputTokenCountingCapability() const;

    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void newConversation();
    Q_INVOKABLE void clearError();

signals:
    void runtimeStateChanged();
    void busyChanged();
    void reasoningChanged();
    void lastErrorChanged();
    void capabilitiesChanged();

private:
    void connectRuntime();
    void handleRuntimeEvent(const suprai::runtime::RuntimeApplicationEvent &event);
    void setRuntimeState(suprai::runtime::RuntimeState state);
    void setCapabilities(const suprai::runtime::RuntimeCapabilities &capabilities);
    void setReasoning(bool active);
    void setLastError(const QString &message);

    suprai::runtime::AgentRuntime *m_runtime = nullptr;
    internal::TranscriptModel *m_transcript = nullptr;
    suprai::runtime::RuntimeState m_runtimeState = suprai::runtime::RuntimeState::Stopped;
    suprai::runtime::RuntimeCapabilities m_capabilities;
    bool m_reasoning = false;
    QString m_lastError;
};

} // namespace suprai::ui
