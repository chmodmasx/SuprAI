#include "AppSettings.h"

#include <qt6keychain/keychain.h>

#include <QCryptographicHash>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcessEnvironment>
#include <QSettings>
#include <QTimer>
#include <QUrl>

namespace suprai::app {

namespace {
const QString Service = QStringLiteral("org.supralinux.SuprAI");
QString credentialKey(const QString &baseUrl)
{
    return QStringLiteral("provider/")
        + QString::fromLatin1(QCryptographicHash::hash(
            baseUrl.toUtf8(), QCryptographicHash::Sha256).toHex());
}
}

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(new QSettings(QStringLiteral("SuprAI"), QStringLiteral("SuprAI"), this))
    , m_network(new QNetworkAccessManager(this))
{
    m_config.runtimeMode = envOrDefault(
        "SUPRAI_RUNTIME",
        m_settings->value(QStringLiteral("runtime/mode"), QStringLiteral("native")).toString());
    m_config.baseUrl = envOrDefault(
        "SUPRAI_BASE_URL",
        m_settings->value(QStringLiteral("provider/baseUrl"), QStringLiteral("http://127.0.0.1:8090/v1")).toString());
    // No arbitrary default model. Discovery is driven by the chosen endpoint.
    m_config.model = envOrDefault(
        "SUPRAI_MODEL",
        m_settings->value(QStringLiteral("provider/model")).toString());
    m_config.apiKey = envOrDefault("SUPRAI_API_KEY", {});
    m_config.systemPrompt = envOrDefault("SUPRAI_SYSTEM_PROMPT", m_config.systemPrompt);
}

AppSettings::~AppSettings() = default;

QString AppSettings::runtimeMode() const { return m_config.runtimeMode; }
QString AppSettings::baseUrl() const { return m_config.baseUrl; }
QString AppSettings::model() const { return m_config.model; }
QString AppSettings::activeBaseUrl() const { return m_activeConfig.baseUrl; }
QString AppSettings::activeModel() const { return m_activeConfig.model; }
QStringList AppSettings::discoveredModels() const { return m_discoveredModels; }
QString AppSettings::statusMessage() const { return m_statusMessage; }
bool AppSettings::busy() const { return m_busy; }
bool AppSettings::hasStoredKey() const { return m_hasStoredKey; }
AppConfig AppSettings::config() const { return m_activeConfig; }

QString AppSettings::envOrDefault(const char *name, const QString &fallback)
{
    const auto environment = QProcessEnvironment::systemEnvironment();
    const QString key = QString::fromLatin1(name);
    const QString value = environment.value(key).trimmed();
    return !environment.contains(key) || value.isEmpty() ? fallback : value;
}

QString AppSettings::validateEndpoint(const QString &value)
{
    const QUrl url(value.trimmed());
    if (!url.isValid() || url.host().isEmpty()
        || !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment()
        || url.path() != QStringLiteral("/v1")
        || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        return QStringLiteral("Ingresá una URL http(s) válida que termine en /v1, sin parámetros ni credenciales.");
    }

    // Do not transmit bearer tokens in cleartext over the public internet.
    if (url.scheme() == QStringLiteral("http")) {
        const QString host = url.host().toLower();
        const QHostAddress ip(host);
        const bool privateIp = ip.isLoopback()
            || ip.isInSubnet(QHostAddress(QStringLiteral("10.0.0.0")), 8)
            || ip.isInSubnet(QHostAddress(QStringLiteral("172.16.0.0")), 12)
            || ip.isInSubnet(QHostAddress(QStringLiteral("192.168.0.0")), 16);
        if (!privateIp && host != QStringLiteral("localhost")) {
            return QStringLiteral("Para servidores públicos necesitás HTTPS. HTTP solamente se permite en direcciones locales/privadas.");
        }
    }
    return {};
}

void AppSettings::setBusy(bool value)
{
    if (m_busy == value) return;
    m_busy = value;
    emit busyChanged();
}

void AppSettings::setStatusMessage(const QString &text)
{
    if (m_statusMessage == text) return;
    m_statusMessage = text;
    emit statusMessageChanged();
}

void AppSettings::resolveStartupCredential()
{
    if (m_secretResolved) return;
    // Mock tests and first-run setup do not need to wake a desktop wallet.
    if (m_config.runtimeMode.compare(QStringLiteral("mock"), Qt::CaseInsensitive) == 0
        || m_config.model.isEmpty()) {
        m_activeConfig = m_config;
        m_secretResolved = true;
        emit activeConfigurationChanged();
        QTimer::singleShot(0, this, [this] { emit credentialsReady(); });
        return;
    }
    if (!m_config.apiKey.isEmpty()) {
        m_activeConfig = m_config;
        m_secretResolved = true;
        emit activeConfigurationChanged();
        QTimer::singleShot(0, this, [this] { emit credentialsReady(); });
        return;
    }

    auto *job = new QKeychain::ReadPasswordJob(Service, this);
    job->setKey(credentialKey(m_config.baseUrl));
    job->setInsecureFallback(false);
    connect(job, &QKeychain::Job::finished, this, [this](QKeychain::Job *finished) {
        const auto *read = static_cast<QKeychain::ReadPasswordJob *>(finished);
        if (read->error() == QKeychain::NoError) {
            m_config.apiKey = read->textData();
            m_hasStoredKey = !m_config.apiKey.isEmpty();
            emit credentialStatusChanged();
        } else if (read->error() != QKeychain::EntryNotFound) {
            setStatusMessage(QStringLiteral("No se pudo leer KWallet/Secret Service. La clave no se cargó; revisá tu gestor de secretos."));
        }
        m_activeConfig = m_config;
        m_secretResolved = true;
        emit activeConfigurationChanged();
        emit credentialsReady();
    });
    job->start();
}

void AppSettings::discoverModels(const QString &baseUrl, const QString &temporaryKey)
{
    const QString error = validateEndpoint(baseUrl);
    if (!error.isEmpty()) {
        setStatusMessage(error);
        return;
    }
    if (m_busy) return;

    setBusy(true);
    setStatusMessage(QStringLiteral("Consultando modelos…"));
    if (!m_discoveredModels.isEmpty()) {
        m_discoveredModels.clear();
        emit discoveredModelsChanged();
    }
    QNetworkRequest request(QUrl(baseUrl.trimmed() + QStringLiteral("/models")));
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(12000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    const QString key = !temporaryKey.isEmpty()
        ? temporaryKey
        : (baseUrl.trimmed() == m_config.baseUrl ? m_config.apiKey : QString{});
    if (!key.isEmpty()) {
        request.setRawHeader("Authorization", QByteArray("Bearer ") + key.toUtf8());
    }
    m_catalogReply = m_network->get(request);
    connect(m_catalogReply, &QNetworkReply::finished, this, [this] {
        QNetworkReply *reply = m_catalogReply;
        m_catalogReply = nullptr;
        if (!reply) return;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->read(256 * 1024 + 1);
        const auto networkError = reply->error();
        reply->deleteLater();
        setBusy(false);
        if (networkError != QNetworkReply::NoError || status != 200) {
            setStatusMessage(QStringLiteral("No se pudieron consultar los modelos (HTTP %1). Comprobá la URL y la clave.").arg(status));
            return;
        }
        if (payload.size() > 256 * 1024) {
            setStatusMessage(QStringLiteral("El catálogo supera el límite de tamaño permitido."));
            return;
        }
        const auto json = QJsonDocument::fromJson(payload);
        if (!json.isObject() || !json.object().value(QStringLiteral("data")).isArray()) {
            setStatusMessage(QStringLiteral("El servidor no devolvió un catálogo OpenAI-compatible."));
            return;
        }
        QStringList models;
        for (const auto &entry : json.object().value(QStringLiteral("data")).toArray()) {
            const QString id = entry.toObject().value(QStringLiteral("id")).toString();
            if (!id.trimmed().isEmpty() && id.size() <= 256 && !models.contains(id)) {
                models.push_back(id);
            }
            if (models.size() >= 128) break;
        }
        m_discoveredModels = models;
        emit discoveredModelsChanged();
        setStatusMessage(models.isEmpty()
            ? QStringLiteral("El servidor no anunció modelos.")
            : QStringLiteral("Se encontraron %1 modelos. Seleccioná uno y guardá.").arg(models.size()));
    });
}

void AppSettings::persistMetadata(const QString &baseUrl, const QString &model)
{
    m_settings->setValue(QStringLiteral("provider/baseUrl"), baseUrl);
    m_settings->setValue(QStringLiteral("provider/model"), model);
    m_settings->sync();
    if (m_settings->status() != QSettings::NoError) {
        setStatusMessage(QStringLiteral("No se pudo guardar la configuración en disco."));
        return;
    }
    m_config.baseUrl = baseUrl;
    m_config.model = model;
    emit configurationChanged();
    setStatusMessage(QStringLiteral("Configuración guardada. Cerrá SuprAI y volvé a abrirlo para aplicar el cambio."));
}

void AppSettings::beginCredentialWrite(const QString &baseUrl, const QString &model,
                                       const QString &replacementKey)
{
    setBusy(true);
    auto *job = new QKeychain::WritePasswordJob(Service, this);
    job->setKey(credentialKey(baseUrl));
    job->setTextData(replacementKey);
    job->setInsecureFallback(false);
    connect(job, &QKeychain::Job::finished, this,
            [this, baseUrl, model, replacementKey](QKeychain::Job *finished) {
        setBusy(false);
        if (finished->error() != QKeychain::NoError) {
            setStatusMessage(QStringLiteral("KWallet/Secret Service rechazó la clave. No se guardó la configuración."));
            return;
        }
        m_config.apiKey = replacementKey;
        m_hasStoredKey = true;
        emit credentialStatusChanged();
        persistMetadata(baseUrl, model);
    });
    job->start();
}

void AppSettings::saveProvider(const QString &baseUrl, const QString &model,
                               const QString &replacementKey)
{
    if (!m_secretResolved || m_busy) return;
    const QString error = validateEndpoint(baseUrl);
    if (!error.isEmpty()) {
        setStatusMessage(error);
        return;
    }
    const QString cleanedModel = model.trimmed();
    if (cleanedModel.isEmpty() || cleanedModel.size() > 256) {
        setStatusMessage(QStringLiteral("Seleccioná o escribí un identificador de modelo válido."));
        return;
    }
    if (replacementKey.contains(QLatin1Char('\n')) || replacementKey.contains(QLatin1Char('\r'))) {
        setStatusMessage(QStringLiteral("La clave no puede contener saltos de línea."));
        return;
    }
    // A credential is scoped to its configured endpoint. Changing the URL
    // must not accidentally forward an existing bearer token to a new host.
    if (baseUrl.trimmed() != m_config.baseUrl && !m_config.apiKey.isEmpty()
        && replacementKey.isEmpty()) {
        setStatusMessage(QStringLiteral("Para cambiar de endpoint, ingresá la clave del nuevo servidor o borrá primero la credencial anterior."));
        return;
    }
    if (!replacementKey.isEmpty()) {
        beginCredentialWrite(baseUrl.trimmed(), cleanedModel, replacementKey);
    } else {
        persistMetadata(baseUrl.trimmed(), cleanedModel);
    }
}

void AppSettings::deleteStoredKey()
{
    if (!m_secretResolved || m_busy) return;
    setBusy(true);
    auto *job = new QKeychain::DeletePasswordJob(Service, this);
    job->setKey(credentialKey(m_config.baseUrl));
    job->setInsecureFallback(false);
    connect(job, &QKeychain::Job::finished, this, [this](QKeychain::Job *finished) {
        setBusy(false);
        if (finished->error() != QKeychain::NoError
            && finished->error() != QKeychain::EntryNotFound) {
            setStatusMessage(QStringLiteral("No se pudo borrar la clave del gestor seguro."));
            return;
        }
        m_hasStoredKey = false;
        m_config.apiKey.clear();
        emit credentialStatusChanged();
        setStatusMessage(QStringLiteral("Clave borrada. Reiniciá SuprAI para aplicar el cambio."));
    });
    job->start();
}

} // namespace suprai::app
