#pragma once

#include <QLoggingCategory>

namespace suprai::platform {

Q_DECLARE_LOGGING_CATEGORY(logApp)
Q_DECLARE_LOGGING_CATEGORY(logRuntime)
Q_DECLARE_LOGGING_CATEGORY(logProvider)
Q_DECLARE_LOGGING_CATEGORY(logPersistence)
Q_DECLARE_LOGGING_CATEGORY(logPlatform)

void initializeLogging();

} // namespace suprai::platform
