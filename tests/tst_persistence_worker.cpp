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
        const QString databasePath = stateDirectory + QStringLiteral("/suprai.sqlite3");

        QThread thread;
        thread.setObjectName(QStringLiteral("PersistenceWorkerTestThread"));

        auto *worker = new suprai::persistence::PersistenceWorker(stateDirectory);
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

        QMetaObject::invokeMethod(
            worker,
            &suprai::persistence::PersistenceWorker::shutdown,
            Qt::QueuedConnection);

        QTRY_COMPARE_WITH_TIMEOUT(stopped.size(), 1, 3000);
        QVERIFY(thread.wait(3000));

        const QString connectionName =
            QStringLiteral("persistence-test-%1")
                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

        {
            auto database = QSqlDatabase::addDatabase(
                QStringLiteral("QSQLITE"),
                connectionName);
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

            QSet<QString> tables;
            QSqlQuery tableQuery(database);
            QVERIFY(tableQuery.exec(
                QStringLiteral(
                    "SELECT name FROM sqlite_master "
                    "WHERE type IN ('table','view')")));
            while (tableQuery.next()) {
                tables.insert(tableQuery.value(0).toString());
            }

            for (const auto &required : {
                     QStringLiteral("sessions"),
                     QStringLiteral("inputs"),
                     QStringLiteral("turns"),
                     QStringLiteral("runs"),
                     QStringLiteral("conversation_items"),
                     QStringLiteral("conversation_fts")}) {
                QVERIFY2(
                    tables.contains(required),
                    qPrintable(QStringLiteral("Missing table: %1").arg(required)));
            }

            QSqlQuery foreignKeys(database);
            QVERIFY(foreignKeys.exec(QStringLiteral("PRAGMA foreign_key_list(turns)")));
            QVERIFY(foreignKeys.next());

            QSqlQuery ftsInsert(database);
            QVERIFY(ftsInsert.exec(
                QStringLiteral(
                    "INSERT INTO conversation_fts(item_id, session_id, text) "
                    "VALUES('item_test','session_test','hola mundo persistente')")));

            QSqlQuery ftsSearch(database);
            QVERIFY(ftsSearch.exec(
                QStringLiteral(
                    "SELECT item_id FROM conversation_fts "
                    "WHERE conversation_fts MATCH 'mundo'")));
            QVERIFY(ftsSearch.next());
            QCOMPARE(
                ftsSearch.value(0).toString(),
                QStringLiteral("item_test"));

            QSqlQuery integrity(database);
            QVERIFY(integrity.exec(QStringLiteral("PRAGMA integrity_check")));
            QVERIFY(integrity.next());
            QCOMPARE(integrity.value(0).toString(), QStringLiteral("ok"));

            database.close();
        }

        QSqlDatabase::removeDatabase(connectionName);
    }
};

QTEST_MAIN(PersistenceWorkerTest)
#include "tst_persistence_worker.moc"
