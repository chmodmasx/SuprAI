#pragma once

#include <suprai/persistence/PersistencePort.h>

#include <QSqlDatabase>
#include <QObject>
#include <QString>

namespace suprai::persistence {

class PersistenceWorker final : public QObject
{
    Q_OBJECT

public:
    explicit PersistenceWorker(QString stateDirectory, QObject *parent = nullptr);

    QString databasePath() const;

public slots:
    void initialize();
    void persistTurnStart(suprai::persistence::TurnStartWrite request);
    void persistTurnTerminal(suprai::persistence::TurnTerminalWrite request);
    void loadLatestSession();
    void shutdown();

signals:
    void ready();
    void errorOccurred(const QString &message);
    void turnStartPersisted(const QString &requestId);
    void turnTerminalPersisted(const QString &requestId);
    void latestSessionLoaded(suprai::persistence::SessionSnapshot snapshot);
    void readFailed(const QString &message);
    void writeFailed(const QString &requestId, const QString &message);
    void stopped();

private:
    bool openDatabase(QString *errorMessage);
    bool configureDatabase(QString *errorMessage);
    bool migrate(QString *errorMessage);
    bool migrateToV1(QString *errorMessage);
    bool verifyFts5(QString *errorMessage);
    bool verifyDatabase(QString *errorMessage);
    void closeDatabase();

    QString m_stateDirectory;
    QString m_databasePath;
    QString m_connectionName;
    QSqlDatabase m_database;
    bool m_ready = false;
};

} // namespace suprai::persistence
