#include "runtime/MockRuntime.h"
#include "runtime/RuntimeState.h"

#include <QSignalSpy>
#include <QTest>

class MockRuntimeTest final : public QObject
{
    Q_OBJECT

private slots:
    void completesAStreamingTurn()
    {
        suprai::runtime::MockRuntime runtime;

        QSignalSpy accepted(&runtime, &suprai::runtime::AgentRuntime::userMessageAccepted);
        QSignalSpy started(&runtime, &suprai::runtime::AgentRuntime::assistantMessageStarted);
        QSignalSpy deltas(&runtime, &suprai::runtime::AgentRuntime::assistantTextDelta);
        QSignalSpy completed(&runtime, &suprai::runtime::AgentRuntime::assistantMessageCompleted);

        runtime.start();
        runtime.submitPrompt(QStringLiteral("test"));

        QTRY_COMPARE_WITH_TIMEOUT(completed.size(), 1, 2000);

        QCOMPARE(accepted.size(), 1);
        QCOMPARE(started.size(), 1);
        QVERIFY(deltas.size() >= 1);
        QVERIFY(!completed.first().at(1).toString().isEmpty());
    }
};

QTEST_MAIN(MockRuntimeTest)
#include "tst_mock_runtime.moc"
