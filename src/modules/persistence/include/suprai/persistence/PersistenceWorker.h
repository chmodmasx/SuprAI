#pragma once

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
    void shutdown();

signals:
    void ready();
    void errorOccurred(const QString &message);
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
