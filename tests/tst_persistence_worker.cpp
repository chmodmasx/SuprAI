#include <suprai/persistence/PersistenceWorker.h>

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

class PersistenceWorkerTest final : public QObject
{
    Q_OBJECT

private slots:
    void initializesAndStopsOnItsOwningThread()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());

        const QString stateDirectory = root.path() + QStringLiteral("/state");

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
