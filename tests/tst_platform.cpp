#include <suprai/platform/AppPaths.h>
#include <suprai/platform/Logging.h>

#include <QCoreApplication>
#include <QFileInfo>
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
