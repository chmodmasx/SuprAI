#include <suprai/persistence/PersistencePort.h>

#include <utility>

namespace suprai::persistence {

PersistencePort::PersistencePort(QObject *parent)
    : QObject(parent)
{
}

void PersistencePort::persistTurnStart(TurnStartWrite request)
{
    emit persistTurnStartRequested(std::move(request));
}

void PersistencePort::persistTurnTerminal(TurnTerminalWrite request)
{
    emit persistTurnTerminalRequested(std::move(request));
}

} // namespace suprai::persistence
