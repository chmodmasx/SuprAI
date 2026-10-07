#include "app/AppSettings.h"
#include "runtime/AgentRuntime.h"
#include "runtime/RuntimeFactory.h"
#include "runtime/RuntimeState.h"
#include "ui/ChatController.h"

#include <QApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("SuprAI"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("supralinux.org"));
    QCoreApplication::setApplicationName(QStringLiteral("SuprAI"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    qRegisterMetaType<suprai::runtime::RuntimeState>();

    suprai::app::AppSettings settings;

    QThread runtimeThread;
    runtimeThread.setObjectName(QStringLiteral("SuprAIRuntime"));

    auto *runtime = suprai::runtime::RuntimeFactory::create(settings.runtimeConfig());
    runtime->moveToThread(&runtimeThread);

    QObject::connect(&runtimeThread, &QThread::started,
                     runtime, &suprai::runtime::AgentRuntime::start);

    QObject::connect(runtime, &suprai::runtime::AgentRuntime::stopped,
                     &runtimeThread, &QThread::quit);

    QObject::connect(runtime, &suprai::runtime::AgentRuntime::stopped,
                     runtime, &QObject::deleteLater);

    suprai::ui::ChatController chatController(runtime);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("chatController"), &chatController);
    engine.rootContext()->setContextProperty(QStringLiteral("appSettings"), &settings);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] {
            QCoreApplication::exit(1);
        },
        Qt::QueuedConnection);

    runtimeThread.start();
    engine.loadFromModule(QStringLiteral("SuprAI"), QStringLiteral("Main"));

    const bool smokeTest = QCoreApplication::arguments().contains(QStringLiteral("--smoke-test"));
    if (smokeTest) {
        // Give QML and the queued runtime startup enough time to complete one
        // event-loop cycle, then exercise the normal shutdown path.
        QTimer::singleShot(500, &app, &QCoreApplication::quit);
    }

    const int result = app.exec();

    if (runtimeThread.isRunning()) {
        QMetaObject::invokeMethod(
            runtime,
            [runtime] {
                runtime->shutdown();
            },
            Qt::QueuedConnection);

        if (!runtimeThread.wait(3000)) {
            runtimeThread.requestInterruption();
            runtimeThread.quit();
            runtimeThread.wait(1000);
        }
    }

    return result;
}
