#include <suprai/persistence/PersistenceWorker.h>

#include <QFileInfo>
#include <QSet>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QUuid>

class PersistenceWorkerTest final : public QObject
{
    Q_OBJECT

private slots:
    void initializesMigratesAndStopsOnItsOwningThread()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());

        const QString stateDirectory = root.path() + QStringLiteral("/state");

        QThread thread;
        thread.setObjectName(QStringLiteral("PersistenceWorkerTestThread"));

        auto *worker = new suprai::persistence::PersistenceWorker(stateDirectory);
        const QString databasePath = worker->databasePath();
        worker->moveToThread(&thread);

        QSignalSpy ready(worker, &suprai::persistence::PersistenceWorker::ready);
        QSignalSpy errors(worker, &suprai::persistence::PersistenceWorker::errorOccurred);
        QSignalSpy stopped(worker, &suprai::persistence::PersistenceWorker::stopped);

        connect(&thread, &QThread::started,
                worker, &suprai::persistence::PersistenceWorker::initialize);
        connect(worker, &suprai::persistence::PersistenceWorker::stopped,
                &thread, &QThread::quit);
        connect(&thread, &QThread::finished,
                worker, &QObject::deleteLater);

        thread.start();

        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 3000);
        QCOMPARE(errors.size(), 0);
        QVERIFY(QFileInfo(stateDirectory).isDir());
        QVERIFY(QFileInfo(databasePath).isFile());

        const QString readConnectionName =
            QStringLiteral("persistence-test-%1")
                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

        {
            auto database = QSqlDatabase::addDatabase(
                QStringLiteral("QSQLITE"),
                readConnectionName);
            database.setDatabaseName(databasePath);
            QVERIFY2(database.open(), qPrintable(database.lastError().text()));

            QSqlQuery version(database);
            QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
            QVERIFY(version.next());
            QCOMPARE(version.value(0).toInt(), 1);

            QSqlQuery journal(database);
            QVERIFY(journal.exec(QStringLiteral("PRAGMA journal_mode")));
            QVERIFY(journal.next());
            QCOMPARE(
                journal.value(0).toString().toLower(),
                QStringLiteral("wal"));

            QSqlQuery migration(database);
            QVERIFY(migration.exec(
                QStringLiteral(
                    "SELECT version FROM schema_migrations ORDER BY version")));
            QVERIFY(migration.next());
            QCOMPARE(migration.value(0).toInt(), 1);
            QVERIFY(!migration.next());

            QSqlQuery tables(database);
            QVERIFY(tables.exec(
                QStringLiteral(
                    "SELECT name FROM sqlite_master "
                    "WHERE type IN ('table','view')")));

            QSet<QString> names;
            while (tables.next()) {
                names.insert(tables.value(0).toString());
            }

            const QSet<QString> required = {
                QStringLiteral("schema_migrations"),
                QStringLiteral("sessions"),
                QStringLiteral("inputs"),
                QStringLiteral("turns"),
                QStringLiteral("runs"),
                QStringLiteral("conversation_items"),
                QStringLiteral("conversation_items_fts"),
            };

            for (const auto &name : required) {
                QVERIFY2(
                    names.contains(name),
                    qPrintable(QStringLiteral("Missing SQLite object: %1").arg(name)));
            }

            QSqlQuery ftsProbe(database);
            QVERIFY(ftsProbe.exec(
                QStringLiteral(
                    "INSERT INTO conversation_items_fts(item_id, session_id, text) "
                    "VALUES('item_test','session_test','hola suprAI')")));
            QVERIFY(ftsProbe.exec(
                QStringLiteral(
                    "SELECT item_id FROM conversation_items_fts "
                    "WHERE conversation_items_fts MATCH 'hola'")));
            QVERIFY(ftsProbe.next());
            QCOMPARE(ftsProbe.value(0).toString(), QStringLiteral("item_test"));

            database.close();
        }

        QSqlDatabase::removeDatabase(readConnectionName);

        QMetaObject::invokeMethod(
            worker,
            &suprai::persistence::PersistenceWorker::shutdown,
            Qt::QueuedConnection);

        QTRY_COMPARE_WITH_TIMEOUT(stopped.size(), 1, 3000);
        QVERIFY(thread.wait(3000));
    }
};

QTEST_MAIN(PersistenceWorkerTest)
#include "tst_persistence_worker.moc"
