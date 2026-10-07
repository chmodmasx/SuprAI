#include "AppSettings.h"

#include <QProcessEnvironment>
#include <QSettings>

namespace suprai::app {

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(new QSettings(QStringLiteral("SuprAI"), QStringLiteral("SuprAI"), this))
{
    m_config.runtimeMode = envOrDefault(
        "SUPRAI_RUNTIME",
        m_settings->value(QStringLiteral("runtime/mode"), QStringLiteral("native")).toString());

    m_config.baseUrl = envOrDefault(
        "SUPRAI_BASE_URL",
        m_settings->value(QStringLiteral("provider/baseUrl"), QStringLiteral("http://127.0.0.1:8090/v1")).toString());

    m_config.model = envOrDefault(
        "SUPRAI_MODEL",
        m_settings->value(QStringLiteral("provider/model"), QStringLiteral("bonsai2-27b")).toString());

    // Prototype rule: secrets are never persisted in QSettings.
    // Until SecretStore lands, API keys come from the process environment only.
    m_config.apiKey = envOrDefault("SUPRAI_API_KEY", QStringLiteral("no-key"));

    m_config.systemPrompt = envOrDefault(
        "SUPRAI_SYSTEM_PROMPT",
        m_config.systemPrompt);
}

AppSettings::~AppSettings() = default;

QString AppSettings::runtimeMode() const
{
    return m_config.runtimeMode;
}

QString AppSettings::baseUrl() const
{
    return m_config.baseUrl;
}

QString AppSettings::model() const
{
    return m_config.model;
}

AppConfig AppSettings::config() const
{
    return m_config;
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
