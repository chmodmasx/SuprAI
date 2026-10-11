#pragma once

#include "AppConfig.h"

#include <QObject>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;
class QSettings;

namespace suprai::app {

// Application-level facade. Provider transport and runtime ownership stay in
// the existing provider/runtime modules; this class only manages preferences,
// credential-store jobs and a bounded model catalogue request.
class AppSettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString runtimeMode READ runtimeMode CONSTANT)
    Q_PROPERTY(QString baseUrl READ baseUrl NOTIFY configurationChanged)
    Q_PROPERTY(QString model READ model NOTIFY configurationChanged)
    Q_PROPERTY(QString activeBaseUrl READ activeBaseUrl NOTIFY activeConfigurationChanged)
    Q_PROPERTY(QString activeModel READ activeModel NOTIFY activeConfigurationChanged)
    Q_PROPERTY(QStringList discoveredModels READ discoveredModels NOTIFY discoveredModelsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool hasStoredKey READ hasStoredKey NOTIFY credentialStatusChanged)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY configurationChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit AppSettings(QObject *parent = nullptr);
    ~AppSettings() override;

    QString runtimeMode() const;
    QString baseUrl() const;
    QString model() const;
    QString activeBaseUrl() const;
    QString activeModel() const;
    QStringList discoveredModels() const;
    QString statusMessage() const;
    bool busy() const;
    bool hasStoredKey() const;
    bool restartRequired() const;
    AppConfig config() const;

    // Called after QML is loaded. Never stores secrets in unencrypted files.
    void resolveStartupCredential();

    Q_INVOKABLE void discoverModels(const QString &baseUrl, const QString &temporaryKey);
    Q_INVOKABLE void saveProvider(const QString &baseUrl,
                                const QString &model,
                                const QString &replacementKey);
    Q_INVOKABLE void deleteStoredKey();

signals:
    void credentialsReady();
    void configurationChanged();
    void activeConfigurationChanged();
    void discoveredModelsChanged();
    void busyChanged();
    void credentialStatusChanged();
    void statusMessageChanged();

private:
    static QString envOrDefault(const char *name, const QString &fallback);
    static QString validateEndpoint(const QString &url);
    void setBusy(bool busy);
    void setStatusMessage(const QString &text);
    void persistMetadata(const QString &baseUrl, const QString &model);
    void beginCredentialWrite(const QString &baseUrl, const QString &model,
                              const QString &replacementKey);

    QSettings *m_settings = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_catalogReply = nullptr;
    AppConfig m_config;
    AppConfig m_activeConfig;
    QStringList m_discoveredModels;
    QString m_statusMessage;
    bool m_busy = false;
    bool m_hasStoredKey = false;
    bool m_secretResolved = false;
    bool m_restartRequired = false;
};

} // namespace suprai::app
