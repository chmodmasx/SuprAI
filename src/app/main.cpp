#include "ApplicationBootstrap.h"
#include "AppSettings.h"

#include <suprai/persistence/PersistenceWorker.h>
#include <suprai/platform/AppPaths.h>
#include <suprai/platform/ApplicationIdentity.h>
#include <suprai/platform/Logging.h>
#include <suprai/platform/SingleInstanceService.h>
#include <suprai/runtime/AgentRuntime.h>
#include <suprai/runtime/RuntimeState.h>
#include <suprai/ui/ChatController.h>

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QTimer>
#include <QWindow>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("SuprAI"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("supralinux.org"));
    QCoreApplication::setApplicationName(QStringLiteral("SuprAI"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QGuiApplication::setDesktopFileName(suprai::platform::applicationId());

    suprai::platform::initializeLogging();

    const auto appPaths = suprai::platform::AppPaths::resolve();
    QString pathError;
    if (!appPaths.ensureDirectories(&pathError)) {
        qCCritical(suprai::platform::logPlatform).noquote()
            << "event=xdg_paths_failed"
            << "error=" + pathError;
        return 2;
    }

    qCInfo(suprai::platform::logApp).noquote()
        << "event=app_start"
        << "version=" + QCoreApplication::applicationVersion();

    qCInfo(suprai::platform::logPlatform).noquote()
        << "event=xdg_paths_ready"
        << "config=" + appPaths.configDir
        << "data=" + appPaths.dataDir
        << "cache=" + appPaths.cacheDir
        << "state=" + appPaths.stateDir;

    suprai::platform::SingleInstanceService singleInstance;
    const auto instanceResult = singleInstance.start();

    if (instanceResult == suprai::platform::SingleInstanceStartResult::SecondaryActivated) {
        qCInfo(suprai::platform::logPlatform).noquote()
            << "event=secondary_instance_forwarded";
        return 0;
    }

    if (instanceResult == suprai::platform::SingleInstanceStartResult::Unavailable) {
        qCWarning(suprai::platform::logPlatform).noquote()
            << "event=dbus_activation_unavailable";
    } else {
        qCInfo(suprai::platform::logPlatform).noquote()
            << "event=dbus_activation_primary"
            << "service=" + suprai::platform::applicationId();
    }

    qRegisterMetaType<suprai::runtime::RuntimeState>();

    suprai::app::AppSettings settings;

    QThread persistenceThread;
    persistenceThread.setObjectName(QStringLiteral("SuprAIPersistence"));

    auto *persistence =
        suprai::app::ApplicationBootstrap::createPersistenceWorker(appPaths.stateDir);
    persistence->moveToThread(&persistenceThread);

    QObject::connect(
        &persistenceThread,
        &QThread::started,
        persistence,
        &suprai::persistence::PersistenceWorker::initialize);

    QObject::connect(
        persistence,
        &suprai::persistence::PersistenceWorker::ready,
        &app,
        [] {
            qCInfo(suprai::platform::logPersistence).noquote()
                << "event=persistence_worker_ready";
        });

    QObject::connect(
        persistence,
        &suprai::persistence::PersistenceWorker::errorOccurred,
        &app,
        [](const QString &message) {
            qCCritical(suprai::platform::logPersistence).noquote()
                << "event=persistence_worker_error"
                << "error=" + message;
        });

    QObject::connect(
        &persistenceThread,
        &QThread::finished,
        persistence,
        &QObject::deleteLater);

    QThread runtimeThread;
    runtimeThread.setObjectName(QStringLiteral("SuprAIRuntime"));

    auto *runtime = suprai::app::ApplicationBootstrap::createRuntime(settings.config());
    runtime->moveToThread(&runtimeThread);

    QObject::connect(
        &runtimeThread,
        &QThread::started,
        runtime,
        &suprai::runtime::AgentRuntime::start);

    QObject::connect(
        &runtimeThread,
        &QThread::finished,
        runtime,
        &QObject::deleteLater);

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

    QObject::connect(
        &singleInstance,
        &suprai::platform::SingleInstanceService::activationRequested,
        &app,
        [&engine](const QVariantMap &) {
            if (engine.rootObjects().isEmpty()) {
                return;
            }

            auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst());
            if (!window) {
                return;
            }

            window->show();
            window->raise();
            window->requestActivate();
        });

    persistenceThread.start();
    runtimeThread.start();

    engine.loadFromModule(QStringLiteral("SuprAI"), QStringLiteral("Main"));

    const bool smokeTest =
        QCoreApplication::arguments().contains(QStringLiteral("--smoke-test"));
    if (smokeTest) {
        QTimer::singleShot(500, &app, &QCoreApplication::quit);
    }

    const int result = app.exec();

    if (runtimeThread.isRunning()) {
        QMetaObject::invokeMethod(
            runtime,
            [runtime] {
                runtime->shutdown();
            },
            Qt::BlockingQueuedConnection);

        runtimeThread.quit();
        if (!runtimeThread.wait(3000)) {
            qCWarning(suprai::platform::logRuntime).noquote()
                << "event=runtime_shutdown_timeout";
            runtimeThread.requestInterruption();
            runtimeThread.quit();
            runtimeThread.wait(1000);
        }
    }

    if (persistenceThread.isRunning()) {
        QMetaObject::invokeMethod(
            persistence,
            [persistence] {
                persistence->shutdown();
            },
            Qt::BlockingQueuedConnection);

        persistenceThread.quit();
        if (!persistenceThread.wait(3000)) {
            qCWarning(suprai::platform::logPersistence).noquote()
                << "event=persistence_shutdown_timeout";
            persistenceThread.requestInterruption();
            persistenceThread.quit();
            persistenceThread.wait(1000);
        }
    }

    qCInfo(suprai::platform::logApp).noquote()
        << "event=app_stop"
        << "exit_code=" + QString::number(result);

    return result;
}
