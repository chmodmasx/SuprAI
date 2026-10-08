#pragma once

#include <suprai/domain/ConversationItem.h>
#include <suprai/domain/ExecutionModel.h>

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QVector>
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

// A full canonical projection, loaded without provider-side state.
struct SessionSnapshot {
    bool found = false;
    suprai::domain::Session session;
    QVector<suprai::domain::Input> inputs;
    QVector<suprai::domain::Turn> turns;
    QVector<suprai::domain::Run> runs;
    QVector<suprai::domain::ConversationItem> items;
};

class PersistencePort : public QObject
{
    Q_OBJECT

public:
    explicit PersistencePort(QObject *parent = nullptr);

    virtual void persistTurnStart(TurnStartWrite request);
    virtual void persistTurnTerminal(TurnTerminalWrite request);
    virtual void loadLatestSession();

signals:
    void persistTurnStartRequested(suprai::persistence::TurnStartWrite request);
    void persistTurnTerminalRequested(suprai::persistence::TurnTerminalWrite request);
    void loadLatestSessionRequested();
    void turnStartPersisted(const QString &requestId);
    void turnTerminalPersisted(const QString &requestId);
    void latestSessionLoaded(suprai::persistence::SessionSnapshot snapshot);
    void readFailed(const QString &message);
    void writeFailed(const QString &requestId, const QString &message);
};

} // namespace suprai::persistence

Q_DECLARE_METATYPE(suprai::persistence::TurnStartWrite)
Q_DECLARE_METATYPE(suprai::persistence::TurnTerminalWrite)
Q_DECLARE_METATYPE(suprai::persistence::SessionSnapshot)
