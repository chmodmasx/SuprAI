#pragma once

#include <QString>

namespace suprai::platform {

struct AppPaths {
    QString configDir;
    QString dataDir;
    QString cacheDir;
    QString stateDir;

    static AppPaths resolve();

    bool ensureDirectories(QString *errorMessage = nullptr) const;
};

} // namespace suprai::platform
