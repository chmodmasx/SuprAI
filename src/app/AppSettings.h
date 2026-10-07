#pragma once

#include "runtime/RuntimeConfig.h"

#include <QObject>
#include <QString>

class QSettings;

namespace suprai::app {

class AppSettings final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString runtimeMode READ runtimeMode CONSTANT)
    Q_PROPERTY(QString baseUrl READ baseUrl CONSTANT)
    Q_PROPERTY(QString model READ model CONSTANT)

public:
    explicit AppSettings(QObject *parent = nullptr);
    ~AppSettings() override;

    QString runtimeMode() const;
    QString baseUrl() const;
    QString model() const;

    suprai::runtime::RuntimeConfig runtimeConfig() const;

private:
    static QString envOrDefault(const char *name, const QString &fallback);

    QSettings *m_settings = nullptr;
    suprai::runtime::RuntimeConfig m_runtimeConfig;
};

} // namespace suprai::app
