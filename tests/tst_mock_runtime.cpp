#include <suprai/runtime/AgentRuntime.h>
#include <suprai/runtime/RuntimeApplicationEvent.h>
#include <suprai/runtime/RuntimeFactory.h>

#include <QTest>
#include <QVector>

#include <algorithm>

class MockRuntimeTest final : public QObject
{
    Q_OBJECT

private slots:
    void completesAStreamingTurnThroughApplicationEvents()
    {
        auto *runtime = suprai::runtime::createMockRuntime();

        QVector<suprai::runtime::RuntimeApplicationEvent> events;
        connect(runtime, &suprai::runtime::AgentRuntime::eventOccurred,
                this, [&events](const suprai::runtime::RuntimeApplicationEvent &event) {
                    events.push_back(event);
                });

        runtime->start();
        runtime->submitPrompt(QStringLiteral("test"));

        QTRY_VERIFY_WITH_TIMEOUT(
            std::any_of(
                events.cbegin(),
                events.cend(),
                [](const auto &event) {
                    return suprai::runtime::eventPayload<
                        suprai::runtime::ConversationItemCompleted>(event) != nullptr;
                }),
            2000);

        int startedItems = 0;
        int deltas = 0;
        int completedItems = 0;
        bool sawWorking = false;
        bool sawReadyAfterWork = false;
        bool sawCapabilities = false;

        for (const auto &event : events) {
            if (const auto *state =
                    suprai::runtime::eventPayload<suprai::runtime::RuntimeStateChanged>(event)) {
                sawWorking |= state->state == suprai::runtime::RuntimeState::Working;
                if (sawWorking && state->state == suprai::runtime::RuntimeState::Ready) {
                    sawReadyAfterWork = true;
                }
            } else if (suprai::runtime::eventPayload<
                           suprai::runtime::RuntimeCapabilitiesChanged>(event)) {
                sawCapabilities = true;
            } else if (suprai::runtime::eventPayload<
                           suprai::runtime::ConversationItemStarted>(event)) {
                ++startedItems;
            } else if (suprai::runtime::eventPayload<
                           suprai::runtime::ConversationTextDelta>(event)) {
                ++deltas;
            } else if (suprai::runtime::eventPayload<
                           suprai::runtime::ConversationItemCompleted>(event)) {
                ++completedItems;
            }
        }

        QCOMPARE(startedItems, 2);
        QVERIFY(deltas >= 1);
        QCOMPARE(completedItems, 1);
        QVERIFY(sawCapabilities);
        QVERIFY(sawWorking);
        QVERIFY(sawReadyAfterWork);

        runtime->shutdown();
        delete runtime;
    }
};

QTEST_MAIN(MockRuntimeTest)
#include "tst_mock_runtime.moc"
