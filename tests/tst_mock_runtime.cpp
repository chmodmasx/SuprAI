#include <suprai/runtime/AgentRuntime.h>
#include <suprai/runtime/ApplicationEvent.h>
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

        QVector<suprai::runtime::ApplicationEvent> events;
        connect(runtime, &suprai::runtime::AgentRuntime::eventEmitted,
                this, [&events](const suprai::runtime::ApplicationEvent &event) {
                    events.push_back(event);
                });

        runtime->start();
        runtime->submitPrompt(QStringLiteral("test"));

        QTRY_VERIFY_WITH_TIMEOUT(
            std::any_of(
                events.cbegin(),
                events.cend(),
                [](const auto &event) {
                    return suprai::runtime::applicationEventKind(event)
                        == suprai::runtime::ApplicationEventKind::ConversationItemCompleted;
                }),
            2000);

        int startedItems = 0;
        int deltas = 0;
        int completedItems = 0;
        bool sawWorking = false;
        bool sawReadyAfterWork = false;

        for (const auto &event : events) {
            switch (suprai::runtime::applicationEventKind(event)) {
            case suprai::runtime::ApplicationEventKind::RuntimeStateChanged: {
                const auto &state =
                    std::get<suprai::runtime::RuntimeStateChangedEvent>(event.payload).state;
                sawWorking |= state == suprai::runtime::RuntimeState::Working;
                if (sawWorking && state == suprai::runtime::RuntimeState::Ready) {
                    sawReadyAfterWork = true;
                }
                break;
            }
            case suprai::runtime::ApplicationEventKind::ConversationItemStarted:
                ++startedItems;
                break;
            case suprai::runtime::ApplicationEventKind::ConversationTextDelta:
                ++deltas;
                break;
            case suprai::runtime::ApplicationEventKind::ConversationItemCompleted:
                ++completedItems;
                break;
            default:
                break;
            }
        }

        QCOMPARE(startedItems, 2);
        QVERIFY(deltas >= 1);
        QCOMPARE(completedItems, 1);
        QVERIFY(sawWorking);
        QVERIFY(sawReadyAfterWork);

        runtime->shutdown();
        delete runtime;
    }
};

QTEST_MAIN(MockRuntimeTest)
#include "tst_mock_runtime.moc"
