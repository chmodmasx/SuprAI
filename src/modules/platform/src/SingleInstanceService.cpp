#include <suprai/platform/SingleInstanceService.h>

#include <suprai/platform/ApplicationIdentity.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>

namespace suprai::platform {

namespace {

constexpr auto FreedesktopApplicationInterface = "org.freedesktop.Application";

} // namespace

SingleInstanceService::SingleInstanceService(QObject *parent)
    : QObject(parent)
{
}

SingleInstanceService::~SingleInstanceService()
{
    if (!m_primary) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return;
    }

    bus.unregisterObject(applicationObjectPath());
    bus.unregisterService(applicationId());
}

SingleInstanceStartResult SingleInstanceService::start()
{
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return SingleInstanceStartResult::Unavailable;
    }

    if (bus.registerService(applicationId())) {
        if (!bus.registerObject(
                applicationObjectPath(),
                this,
                QDBusConnection::ExportAllSlots)) {
            bus.unregisterService(applicationId());
            return SingleInstanceStartResult::Unavailable;
        }

        m_primary = true;
        return SingleInstanceStartResult::Primary;
    }

    QDBusInterface primary(
        applicationId(),
        applicationObjectPath(),
        QString::fromLatin1(FreedesktopApplicationInterface),
        bus);

    if (!primary.isValid()) {
        return SingleInstanceStartResult::Unavailable;
    }

    const QDBusMessage reply =
        primary.call(QStringLiteral("Activate"), QVariantMap{});

    if (reply.type() == QDBusMessage::ErrorMessage) {
        return SingleInstanceStartResult::Unavailable;
    }

    return SingleInstanceStartResult::SecondaryActivated;
}

bool SingleInstanceService::isPrimary() const
{
    return m_primary;
}

void SingleInstanceService::Activate(const QVariantMap &platformData)
{
    emit activationRequested(platformData);
}

void SingleInstanceService::Open(
    const QStringList &uris,
    const QVariantMap &platformData)
{
    emit openRequested(uris, platformData);
    emit activationRequested(platformData);
}

void SingleInstanceService::ActivateAction(
    const QString &actionName,
    const QVariantList &parameters,
    const QVariantMap &platformData)
{
    emit actionRequested(actionName, parameters, platformData);
    emit activationRequested(platformData);
}

} // namespace suprai::platform
