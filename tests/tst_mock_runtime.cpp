#include <suprai/runtime/AgentRuntime.h>
#include <suprai/runtime/RuntimeFactory.h>

#include <QTest>

class MockRuntimeTest final : public QObject
{
    Q_OBJECT

private slots:
    void completesAStreamingTurn()
    {
        auto *runtime = suprai::runtime::createMockRuntime();

        int accepted = 0;
        int started = 0;
        int deltas = 0;
        int completed = 0;
        bool capabilitiesSeen = false;
        suprai::runtime::RuntimeCapabilities capabilities;

        connect(runtime, &suprai::runtime::AgentRuntime::eventOccurred,
                this, [&](const suprai::runtime::RuntimeApplicationEvent &event) {
            if (suprai::runtime::eventPayload<suprai::runtime::UserMessageAccepted>(event)) {
                ++accepted;
            } else if (suprai::runtime::eventPayload<suprai::runtime::AssistantMessageStarted>(event)) {
                ++started;
            } else if (suprai::runtime::eventPayload<suprai::runtime::AssistantTextDelta>(event)) {
                ++deltas;
            } else if (suprai::runtime::eventPayload<suprai::runtime::AssistantMessageCompleted>(event)) {
                ++completed;
            } else if (const auto *changed =
                           suprai::runtime::eventPayload<suprai::runtime::RuntimeCapabilitiesChanged>(event)) {
                capabilities = changed->capabilities;
                capabilitiesSeen = true;
            }
        });

        runtime->start();
        runtime->submitPrompt(QStringLiteral("test"));

        QTRY_COMPARE_WITH_TIMEOUT(completed, 1, 2000);

        QCOMPARE(accepted, 1);
        QCOMPARE(started, 1);
        QVERIFY(deltas >= 1);
        QVERIFY(capabilitiesSeen);
        QCOMPARE(capabilities.textGeneration, suprai::runtime::CapabilityState::Supported);
        QCOMPARE(capabilities.toolCalling, suprai::runtime::CapabilityState::Unsupported);
        QCOMPARE(capabilities.imageInput, suprai::runtime::CapabilityState::Unsupported);
        QCOMPARE(capabilities.reasoningOutput, suprai::runtime::CapabilityState::Supported);
        QCOMPARE(capabilities.exactInputTokenCounting, suprai::runtime::CapabilityState::Unsupported);

        runtime->shutdown();
        delete runtime;
    }
};

QTEST_MAIN(MockRuntimeTest)
#include "tst_mock_runtime.moc"
