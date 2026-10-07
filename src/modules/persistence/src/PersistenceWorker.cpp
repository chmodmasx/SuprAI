#include <suprai/persistence/PersistenceWorker.h>

#include <QDir>
#include <QThread>

namespace suprai::persistence {

PersistenceWorker::PersistenceWorker(QString stateDirectory, QObject *parent)
    : QObject(parent)
    , m_stateDirectory(std::move(stateDirectory))
{
}

void PersistenceWorker::initialize()
{
    Q_ASSERT(thread() == QThread::currentThread());

    if (m_ready) {
        return;
    }

    if (m_stateDirectory.isEmpty() || !QDir().mkpath(m_stateDirectory)) {
        emit errorOccurred(
            QStringLiteral("No se pudo inicializar el directorio de estado: %1")
                .arg(m_stateDirectory));
        return;
    }

    m_ready = true;
    emit ready();
}

void PersistenceWorker::shutdown()
{
    Q_ASSERT(thread() == QThread::currentThread());

    m_ready = false;
    emit stopped();
}

} // namespace suprai::persistence
