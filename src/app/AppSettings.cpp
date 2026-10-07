#include "app/AppSettings.h"

#include <QByteArray>
#include <QProcessEnvironment>
#include <QSettings>

namespace suprai::app {

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(new QSettings(QStringLiteral("SuprAI"), QStringLiteral("SuprAI"), this))
{
    m_runtimeConfig.mode = envOrDefault(
        "SUPRAI_RUNTIME",
        m_settings->value(QStringLiteral("runtime/mode"), QStringLiteral("native")).toString());

    m_runtimeConfig.baseUrl = envOrDefault(
        "SUPRAI_BASE_URL",
        m_settings->value(QStringLiteral("provider/baseUrl"), QStringLiteral("http://127.0.0.1:8090/v1")).toString());

    m_runtimeConfig.model = envOrDefault(
        "SUPRAI_MODEL",
        m_settings->value(QStringLiteral("provider/model"), QStringLiteral("bonsai2-27b")).toString());

    // Prototype rule: secrets are never persisted in QSettings.
    // Until SecretStore lands, API keys come from the process environment only.
    m_runtimeConfig.apiKey = envOrDefault("SUPRAI_API_KEY", QStringLiteral("no-key"));

    m_runtimeConfig.systemPrompt = envOrDefault(
        "SUPRAI_SYSTEM_PROMPT",
        m_runtimeConfig.systemPrompt);
}

AppSettings::~AppSettings() = default;

QString AppSettings::runtimeMode() const
{
    return m_runtimeConfig.mode;
}

QString AppSettings::baseUrl() const
{
    return m_runtimeConfig.baseUrl;
}

QString AppSettings::model() const
{
    return m_runtimeConfig.model;
}

suprai::runtime::RuntimeConfig AppSettings::runtimeConfig() const
{
    return m_runtimeConfig;
}

QString AppSettings::envOrDefault(const char *name, const QString &fallback)
{
    const auto environment = QProcessEnvironment::systemEnvironment();
    const QString key = QString::fromLatin1(name);
    if (!environment.contains(key)) {
        return fallback;
    }

    const QString value = environment.value(key).trimmed();
    return value.isEmpty() ? fallback : value;
}

} // namespace suprai::app
