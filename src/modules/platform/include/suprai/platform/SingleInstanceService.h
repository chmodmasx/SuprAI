#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace suprai::platform {

enum class SingleInstanceStartResult {
    Primary,
    SecondaryActivated,
    Unavailable
};

class SingleInstanceService final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Application")

public:
    explicit SingleInstanceService(QObject *parent = nullptr);
    ~SingleInstanceService() override;

    SingleInstanceStartResult start();
    bool isPrimary() const;

signals:
    void activationRequested(const QVariantMap &platformData);
    void openRequested(const QStringList &uris, const QVariantMap &platformData);
    void actionRequested(
        const QString &actionName,
        const QVariantList &parameters,
        const QVariantMap &platformData);

public slots:
    void Activate(const QVariantMap &platformData);
    void Open(const QStringList &uris, const QVariantMap &platformData);
    void ActivateAction(
        const QString &actionName,
        const QVariantList &parameters,
        const QVariantMap &platformData);

private:
    bool m_primary = false;
};

} // namespace suprai::platform
