#include <suprai/platform/AppPaths.h>
#include <suprai/platform/ApplicationIdentity.h>
#include <suprai/platform/Logging.h>
#include <suprai/platform/SingleInstanceService.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class PlatformTest final : public QObject
{
    Q_OBJECT

private slots:
    void appPathsResolveAndCreateDirectories()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("SuprAITest"));
        QCoreApplication::setApplicationName(QStringLiteral("SuprAITest"));
        QStandardPaths::setTestModeEnabled(true);

        const auto paths = suprai::platform::AppPaths::resolve();

        QVERIFY(!paths.configDir.isEmpty());
        QVERIFY(!paths.dataDir.isEmpty());
        QVERIFY(!paths.cacheDir.isEmpty());
        QVERIFY(!paths.stateDir.isEmpty());

        QString error;
        QVERIFY2(paths.ensureDirectories(&error), qPrintable(error));

        QVERIFY(QFileInfo(paths.configDir).isDir());
        QVERIFY(QFileInfo(paths.dataDir).isDir());
        QVERIFY(QFileInfo(paths.cacheDir).isDir());
        QVERIFY(QFileInfo(paths.stateDir).isDir());
    }

    void applicationIdentityIsStableAndDbusCompatible()
    {
        QCOMPARE(
            suprai::platform::applicationId(),
            QStringLiteral("org.supralinux.SuprAI"));
        QCOMPARE(
            suprai::platform::applicationObjectPath(),
            QStringLiteral("/org/supralinux/SuprAI"));

        QVERIFY(!suprai::platform::applicationId().contains(QLatin1Char('/')));
        QVERIFY(suprai::platform::applicationObjectPath().startsWith(QLatin1Char('/')));
    }

    void freedesktopActivationMethodsProjectToSignals()
    {
        suprai::platform::SingleInstanceService service;

        QSignalSpy activated(
            &service,
            &suprai::platform::SingleInstanceService::activationRequested);
        QSignalSpy opened(
            &service,
            &suprai::platform::SingleInstanceService::openRequested);
        QSignalSpy actioned(
            &service,
            &suprai::platform::SingleInstanceService::actionRequested);

        const QVariantMap platformData{
            {QStringLiteral("activation-token"), QStringLiteral("test-token")},
        };

        service.Activate(platformData);
        QCOMPARE(activated.size(), 1);

        service.Open(
            {QStringLiteral("file:///tmp/example.txt")},
            platformData);
        QCOMPARE(opened.size(), 1);
        QCOMPARE(activated.size(), 2);

        service.ActivateAction(
            QStringLiteral("new-chat"),
            {},
            platformData);
        QCOMPARE(actioned.size(), 1);
        QCOMPARE(activated.size(), 3);
    }

    void loggingCategoriesAreStable()
    {
        QCOMPARE(
            QString::fromLatin1(suprai::platform::logApp().categoryName()),
            QStringLiteral("suprai.app"));
        QCOMPARE(
            QString::fromLatin1(suprai::platform::logRuntime().categoryName()),
            QStringLiteral("suprai.runtime"));
        QCOMPARE(
            QString::fromLatin1(suprai::platform::logProvider().categoryName()),
            QStringLiteral("suprai.provider"));
        QCOMPARE(
            QString::fromLatin1(suprai::platform::logPersistence().categoryName()),
            QStringLiteral("suprai.persistence"));
        QCOMPARE(
            QString::fromLatin1(suprai::platform::logPlatform().categoryName()),
            QStringLiteral("suprai.platform"));
    }
};

QTEST_MAIN(PlatformTest)
#include "tst_platform.moc"
