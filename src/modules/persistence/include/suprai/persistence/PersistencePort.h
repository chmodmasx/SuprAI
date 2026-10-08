#pragma once

#include <suprai/domain/ConversationItem.h>
#include <suprai/domain/ExecutionModel.h>

#include <QMetaType>
#include <QObject>
#include <QString>

namespace suprai::persistence {

struct TurnStartWrite {
    QString requestId;
    suprai::domain::Session session;
    suprai::domain::Input input;
    suprai::domain::Turn turn;
    suprai::domain::Run run;
    suprai::domain::ConversationItem userItem;
};

class PersistencePort : public QObject
{
    Q_OBJECT

public:
    explicit PersistencePort(QObject *parent = nullptr);

    virtual void persistTurnStart(TurnStartWrite request);

signals:
    void persistTurnStartRequested(suprai::persistence::TurnStartWrite request);
    void turnStartPersisted(const QString &requestId);
    void writeFailed(const QString &requestId, const QString &message);
};

} // namespace suprai::persistence

Q_DECLARE_METATYPE(suprai::persistence::TurnStartWrite)
