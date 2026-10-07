#include <suprai/runtime/AgentRuntime.h>
#include <suprai/runtime/RuntimeFactory.h>
#include <suprai/ui/ChatController.h>

#include <QTest>

class ChatControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void projectsTriStateRuntimeCapabilities()
    {
        auto *runtime = suprai::runtime::createMockRuntime();
        suprai::ui::ChatController controller(runtime);

        QCOMPARE(controller.textGenerationCapability(), QStringLiteral("Unknown"));
        QCOMPARE(controller.toolCallingCapability(), QStringLiteral("Unknown"));
        QCOMPARE(controller.imageInputCapability(), QStringLiteral("Unknown"));
        QCOMPARE(controller.reasoningOutputCapability(), QStringLiteral("Unknown"));
        QCOMPARE(
            controller.exactInputTokenCountingCapability(),
            QStringLiteral("Unknown"));

        runtime->start();

        QCOMPARE(controller.runtimeState(), QStringLiteral("Ready"));
        QCOMPARE(controller.textGenerationCapability(), QStringLiteral("Supported"));
        QCOMPARE(controller.toolCallingCapability(), QStringLiteral("Unsupported"));
        QCOMPARE(controller.imageInputCapability(), QStringLiteral("Unsupported"));
        QCOMPARE(controller.reasoningOutputCapability(), QStringLiteral("Supported"));
        QCOMPARE(
            controller.exactInputTokenCountingCapability(),
            QStringLiteral("Unsupported"));

        runtime->shutdown();
        delete runtime;
    }
};

QTEST_MAIN(ChatControllerTest)
#include "tst_chat_controller.moc"
