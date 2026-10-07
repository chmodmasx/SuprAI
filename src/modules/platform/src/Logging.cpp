#include <suprai/platform/Logging.h>

#include <QByteArray>
#include <QProcessEnvironment>
#include <QtGlobal>

namespace suprai::platform {

Q_LOGGING_CATEGORY(logApp, "suprai.app")
Q_LOGGING_CATEGORY(logRuntime, "suprai.runtime")
Q_LOGGING_CATEGORY(logProvider, "suprai.provider")
Q_LOGGING_CATEGORY(logPersistence, "suprai.persistence")
Q_LOGGING_CATEGORY(logPlatform, "suprai.platform")

void initializeLogging()
{
    const auto environment = QProcessEnvironment::systemEnvironment();
    if (!environment.contains(QStringLiteral("QT_MESSAGE_PATTERN"))) {
        qSetMessagePattern(
            QStringLiteral("%{time yyyy-MM-ddTHH:mm:ss.zzz} %{type} %{category} %{message}"));
    }
}

} // namespace suprai::platform
