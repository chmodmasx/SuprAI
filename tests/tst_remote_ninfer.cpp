#include <suprai/persistence/PersistencePort.h>
#include <suprai/persistence/PersistenceWorker.h>
#include <suprai/providers/OpenAIProviderFactory.h>
#include <suprai/runtime/AgentRuntime.h>
#include <suprai/runtime/RuntimeApplicationEvent.h>
#include <suprai/runtime/RuntimeFactory.h>

#include <QObject>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

class RemoteNInferRuntimeTest : public QObject
{
    Q_OBJECT

private slots:
    void authenticatedStreamingAndRestoredContext()
    {
        const QString base = qEnvironmentVariable("NINFER_BASE_URL");
        const QString model = qEnvironmentVariable("NINFER_MODEL");
        const QString key = qEnvironmentVariable("NINFER_API_KEY");
        if (base.isEmpty() || model.isEmpty() || key.isEmpty()) {
            QSKIP("Remote NInfer credentials/model not configured; CI runs this test separately.");
        }
        QCOMPARE(base, QStringLiteral("https://ninfer.minilinux.win/v1"));

        QTemporaryDir isolatedState;
        QVERIFY(isolatedState.isValid());
        suprai::persistence::PersistenceWorker worker(isolatedState.path());
        QObject ownershipGuard;
        suprai::persistence::PersistencePort port(&ownershipGuard);

        QObject::connect(&port, &suprai::persistence::PersistencePort::persistTurnStartRequested,
                         &worker, &suprai::persistence::PersistenceWorker::persistTurnStart,
                         Qt::QueuedConnection);
        QObject::connect(&port, &suprai::persistence::PersistencePort::persistTurnTerminalRequested,
                         &worker, &suprai::persistence::PersistenceWorker::persistTurnTerminal,
                         Qt::QueuedConnection);
        QObject::connect(&port, &suprai::persistence::PersistencePort::loadLatestSessionRequested,
                         &worker, &suprai::persistence::PersistenceWorker::loadLatestSession,
                         Qt::QueuedConnection);
        QObject::connect(&worker, &suprai::persistence::PersistenceWorker::turnStartPersisted,
                         &port, &suprai::persistence::PersistencePort::turnStartPersisted,
                         Qt::QueuedConnection);
        QObject::connect(&worker, &suprai::persistence::PersistenceWorker::turnTerminalPersisted,
                         &port, &suprai::persistence::PersistencePort::turnTerminalPersisted,
                         Qt::QueuedConnection);
        QObject::connect(&worker, &suprai::persistence::PersistenceWorker::latestSessionLoaded,
                         &port, &suprai::persistence::PersistencePort::latestSessionLoaded,
                         Qt::QueuedConnection);
        QObject::connect(&worker, &suprai::persistence::PersistenceWorker::writeFailed,
                         &port, &suprai::persistence::PersistencePort::writeFailed,
                         Qt::QueuedConnection);
        QObject::connect(&worker, &suprai::persistence::PersistenceWorker::readFailed,
                         &port, &suprai::persistence::PersistencePort::readFailed,
                         Qt::QueuedConnection);

        bool databaseReady = false;
        QString databaseError;
        QObject::connect(&worker, &suprai::persistence::PersistenceWorker::ready,
                         &ownershipGuard, [&] { databaseReady = true; });
        QObject::connect(&worker, &suprai::persistence::PersistenceWorker::errorOccurred,
                         &ownershipGuard, [&](const QString &error) { databaseError = error; });
        worker.initialize();
        QVERIFY2(databaseReady, qPrintable(databaseError));

        const auto newRuntime = [&]() {
            auto *provider = suprai::providers::createOpenAIChatProvider({
                .baseUrl = base,
                .apiKey = key,
            });
            return suprai::runtime::createNativeRuntime({
                .model = model,
                .systemPrompt = QStringLiteral("Test de integración. Respondé conciso, sin explicaciones."),
            }, provider, &port);
        };
        const QString code = QStringLiteral("TEST%1")
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)
                                          .left(10).toUpper());

        auto *runtime = newRuntime();
        int completed = 0;
        bool ready = false;
        QString failure;
        QObject::connect(runtime, &suprai::runtime::AgentRuntime::eventOccurred,
                         &ownershipGuard, [&](const suprai::runtime::RuntimeApplicationEvent &event) {
            if (const auto *state =
                    suprai::runtime::eventPayload<suprai::runtime::RuntimeStateChanged>(event)) {
                ready = state->state == suprai::runtime::RuntimeState::Ready;
            }
            if (suprai::runtime::eventPayload<
                    suprai::runtime::ConversationItemCompleted>(event)) {
                ++completed;
            }
            if (const auto *error =
                    suprai::runtime::eventPayload<suprai::runtime::RuntimeError>(event)) {
                failure = error->message;
            }
        });

        runtime->start();
        QTRY_VERIFY_WITH_TIMEOUT(ready || !failure.isEmpty(), 5000);
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
        runtime->submitPrompt(
            QStringLiteral("Recordá este código literal: %1. Contestá solamente 'recibido'.").arg(code));
        QTRY_VERIFY_WITH_TIMEOUT(completed >= 1 || !failure.isEmpty(), 100000);
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
        runtime->shutdown();
        delete runtime;

        // Reconstruct canonical state using the same SQLite worker and a NEW runtime/provider.
        runtime = newRuntime();
        ready = false;
        completed = 0;
        int recoveredItems = 0;
        QString answer;
        QObject::connect(runtime, &suprai::runtime::AgentRuntime::eventOccurred,
                         &ownershipGuard, [&](const suprai::runtime::RuntimeApplicationEvent &event) {
            if (const auto *state =
                    suprai::runtime::eventPayload<suprai::runtime::RuntimeStateChanged>(event)) {
                ready = state->state == suprai::runtime::RuntimeState::Ready;
            }
            if (suprai::runtime::eventPayload<
                    suprai::runtime::ConversationItemStarted>(event)) {
                ++recoveredItems;
            }
            if (const auto *delta =
                    suprai::runtime::eventPayload<suprai::runtime::ConversationTextDelta>(event)) {
                answer += delta->delta;
            }
            if (suprai::runtime::eventPayload<
                    suprai::runtime::ConversationItemCompleted>(event)) {
                ++completed;
            }
            if (const auto *error =
                    suprai::runtime::eventPayload<suprai::runtime::RuntimeError>(event)) {
                failure = error->message;
            }
        });
        runtime->start();
        QTRY_VERIFY_WITH_TIMEOUT(ready || !failure.isEmpty(), 5000);
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
        QVERIFY2(recoveredItems >= 2, "SQLite must restore the previous user + assistant items.");

        runtime->submitPrompt(QStringLiteral("¿Qué código literal te pedí recordar? Solo el código."));
        QTRY_VERIFY_WITH_TIMEOUT(completed >= 1 || !failure.isEmpty(), 100000);
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
        QVERIFY2(answer.toUpper().contains(code),
                 "The second response must demonstrate recovered canonical context.");
        runtime->shutdown();
        delete runtime;
        worker.shutdown();
        // No generated prompts/outputs/API keys are written to CI logs.
    }
};

QTEST_MAIN(RemoteNInferRuntimeTest)
#include "tst_remote_ninfer.moc"
