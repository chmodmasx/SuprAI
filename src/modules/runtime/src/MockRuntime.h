#pragma once

#include <suprai/runtime/AgentRuntime.h>

#include <QStringList>

class QTimer;

namespace suprai::runtime::internal {

class MockRuntime final : public suprai::runtime::AgentRuntime
{
    Q_OBJECT

public:
    explicit MockRuntime(QObject *parent = nullptr);

public slots:
    void start() override;
    void shutdown() override;
    void submitPrompt(const QString &prompt) override;
    void cancelTurn() override;
    void resetSession() override;

private:
    void emitNextChunk();
    void setState(suprai::runtime::RuntimeState state);
    void finish();

    QTimer *m_timer = nullptr;
    suprai::runtime::RuntimeState m_state = suprai::runtime::RuntimeState::Stopped;
    QString m_activeId;
    QString m_text;
    QStringList m_chunks;
    qsizetype m_index = 0;
};

} // namespace suprai::runtime::internal
