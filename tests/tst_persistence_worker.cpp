#include <suprai/persistence/PersistencePort.h>
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
    void initializesMigratesPersistsTurnStartAndStopsOnItsOwningThread()
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
        QSignalSpy writeCommitted(
            worker,
            &suprai::persistence::PersistenceWorker::turnStartPersisted);
        QSignalSpy writeFailures(
            worker,
            &suprai::persistence::PersistenceWorker::writeFailed);
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

        const suprai::domain::Session session{
            .id = suprai::domain::newSessionId(),
            .parentSessionId = {},
        };
        const suprai::domain::Input input{
            .id = suprai::domain::newInputId(),
            .sessionId = session.id,
            .sequence = 1,
            .text = QStringLiteral("hola durable"),
        };
        const suprai::domain::Turn turn{
            .id = suprai::domain::newTurnId(),
            .sessionId = session.id,
            .inputId = input.id,
            .parentTurnId = {},
            .sequence = 1,
        };
        const suprai::domain::Run run{
            .id = suprai::domain::newRunId(),
            .turnId = turn.id,
            .generation = 1,
        };
        const auto userItem = suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::User,
            input.text,
            suprai::domain::ConversationItemState::Completed,
            suprai::domain::newItemId(),
            turn.id,
            1);

        const suprai::persistence::TurnStartWrite write{
            .requestId = run.id,
            .session = session,
            .input = input,
            .turn = turn,
            .run = run,
            .userItem = userItem,
        };

        QMetaObject::invokeMethod(
            worker,
            [worker, write] {
                worker->persistTurnStart(write);
            },
            Qt::QueuedConnection);

        QTRY_COMPARE_WITH_TIMEOUT(writeCommitted.size(), 1, 3000);
        QCOMPARE(writeFailures.size(), 0);

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

            QSqlQuery counts(database);
            QVERIFY(counts.exec(QStringLiteral(
                "SELECT "
                "(SELECT COUNT(*) FROM sessions),"
                "(SELECT COUNT(*) FROM inputs),"
                "(SELECT COUNT(*) FROM turns),"
                "(SELECT COUNT(*) FROM runs),"
                "(SELECT COUNT(*) FROM conversation_items)")));
            QVERIFY(counts.next());
            QCOMPARE(counts.value(0).toInt(), 1);
            QCOMPARE(counts.value(1).toInt(), 1);
            QCOMPARE(counts.value(2).toInt(), 1);
            QCOMPARE(counts.value(3).toInt(), 1);
            QCOMPARE(counts.value(4).toInt(), 1);

            QSqlQuery runState(database);
            runState.prepare(QStringLiteral("SELECT status FROM runs WHERE id=?"));
            runState.addBindValue(run.id);
            QVERIFY(runState.exec());
            QVERIFY(runState.next());
            QCOMPARE(runState.value(0).toString(), QStringLiteral("prepared"));

            QSqlQuery ftsProbe(database);
            QVERIFY(ftsProbe.exec(
                QStringLiteral(
                    "SELECT item_id FROM conversation_items_fts "
                    "WHERE conversation_items_fts MATCH 'durable'")));
            QVERIFY(ftsProbe.next());
            QCOMPARE(ftsProbe.value(0).toString(), userItem.id);

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
