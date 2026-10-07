#include <suprai/platform/ApplicationIdentity.h>

namespace suprai::platform {

QString applicationId()
{
    // Development identity selected by ADR-0013. Confirm before first packaged release.
    return QStringLiteral("org.supralinux.SuprAI");
}

QString applicationObjectPath()
{
    return QStringLiteral("/org/supralinux/SuprAI");
}

} // namespace suprai::platform
