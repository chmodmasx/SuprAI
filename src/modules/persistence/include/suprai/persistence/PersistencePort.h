#pragma once

#include <suprai/domain/ConversationItem.h>
#include <suprai/domain/ExecutionModel.h>

#include <QMetaType>
#include <QObject>
#include <QString>
#include <optional>

namespace suprai::persistence {

struct TurnStartWrite {
    QString requestId;
    suprai::domain::Session session;
    suprai::domain::Input input;
    suprai::domain::Turn turn;
    suprai::domain::Run run;
    suprai::domain::ConversationItem userItem;
};

// Atomic terminal Run transition with optional assistant output.
struct TurnTerminalWrite {
    QString requestId;
    QString runId;
    QString turnId;
    QString sessionId;
    QString status; // completed, failed, cancelled
    std::optional<suprai::domain::ConversationItem> assistantItem;
};

class PersistencePort : public QObject
{
    Q_OBJECT

public:
    explicit PersistencePort(QObject *parent = nullptr);

    virtual void persistTurnStart(TurnStartWrite request);
    virtual void persistTurnTerminal(TurnTerminalWrite request);

signals:
    void persistTurnStartRequested(suprai::persistence::TurnStartWrite request);
    void persistTurnTerminalRequested(suprai::persistence::TurnTerminalWrite request);
    void turnStartPersisted(const QString &requestId);
    void turnTerminalPersisted(const QString &requestId);
    void writeFailed(const QString &requestId, const QString &message);
};

} // namespace suprai::persistence

Q_DECLARE_METATYPE(suprai::persistence::TurnStartWrite)
Q_DECLARE_METATYPE(suprai::persistence::TurnTerminalWrite)
