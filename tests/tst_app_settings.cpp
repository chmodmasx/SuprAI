#include "AppSettings.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class AppSettingsTest final : public QObject
{
    Q_OBJECT
private slots:
    void initializeTestCase()
    {
        QVERIFY(m_root.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_root.path());
        qunsetenv("SUPRAI_MODEL");
        qunsetenv("SUPRAI_BASE_URL");
        qputenv("SUPRAI_API_KEY", "test-only-credential");
    }

    void noHardcodedModelAndNoKeyInSettings()
    {
        suprai::app::AppSettings settings;
        QSignalSpy ready(&settings, &suprai::app::AppSettings::credentialsReady);
        QVERIFY(settings.model().isEmpty());
        settings.resolveStartupCredential();
        QTRY_COMPARE(ready.size(), 1);
        QCOMPARE(settings.config().apiKey, QStringLiteral("test-only-credential"));
        settings.saveProvider(QStringLiteral("http://127.0.0.1:8090/v1"),
                              QStringLiteral("custom-test-model"), {});
        QCOMPARE(settings.model(), QStringLiteral("custom-test-model"));
        QCOMPARE(settings.activeModel(), QString());
        QVERIFY(settings.restartRequired());
        QSettings persisted(QStringLiteral("SuprAI"), QStringLiteral("SuprAI"));
        QCOMPARE(persisted.value(QStringLiteral("provider/model")).toString(),
                 QStringLiteral("custom-test-model"));
        QVERIFY(!persisted.contains(QStringLiteral("provider/apiKey")));
        QVERIFY(!persisted.contains(QStringLiteral("apiKey")));
        QVERIFY(!persisted.contains(QStringLiteral("provider/active")));
    }

    void remoteHttpCannotLeakBearerAndChangingEndpointNeedsNewCredential()
    {
        suprai::app::AppSettings settings;
        QSignalSpy ready(&settings, &suprai::app::AppSettings::credentialsReady);
        settings.resolveStartupCredential();
        QTRY_COMPARE(ready.size(), 1);

        settings.discoverModels(QStringLiteral("http://example.com/v1"),
                                QStringLiteral("test-only-credential"));
        QVERIFY(settings.statusMessage().contains(QStringLiteral("HTTPS")));
        QVERIFY(!settings.busy());

        settings.saveProvider(QStringLiteral("https://another.example/v1"),
                              QStringLiteral("custom-test-model"), {});
        QVERIFY(settings.statusMessage().contains(QStringLiteral("clave")));
        QCOMPARE(settings.baseUrl(), QStringLiteral("http://127.0.0.1:8090/v1"));
    }

private:
    QTemporaryDir m_root;
};

QTEST_MAIN(AppSettingsTest)
#include "tst_app_settings.moc"
