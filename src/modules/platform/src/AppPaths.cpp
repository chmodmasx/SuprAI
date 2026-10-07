#include <suprai/platform/AppPaths.h>

#include <QDir>
#include <QStandardPaths>

namespace suprai::platform {

AppPaths AppPaths::resolve()
{
    return {
        .configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation),
        .dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
        .cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation),
        .stateDir = QStandardPaths::writableLocation(QStandardPaths::StateLocation),
    };
}

bool AppPaths::ensureDirectories(QString *errorMessage) const
{
    const auto ensure = [errorMessage](const QString &path, const QString &label) {
        if (!path.isEmpty() && QDir().mkpath(path)) {
            return true;
        }

        if (errorMessage) {
            *errorMessage = QStringLiteral("No se pudo crear el directorio XDG %1: %2")
                                .arg(label, path);
        }
        return false;
    };

    return ensure(configDir, QStringLiteral("config"))
        && ensure(dataDir, QStringLiteral("data"))
        && ensure(cacheDir, QStringLiteral("cache"))
        && ensure(stateDir, QStringLiteral("state"));
}

} // namespace suprai::platform
