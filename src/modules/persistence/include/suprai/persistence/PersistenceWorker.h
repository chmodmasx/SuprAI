#pragma once

#include <QObject>
#include <QString>

namespace suprai::persistence {

class PersistenceWorker final : public QObject
{
    Q_OBJECT

public:
    explicit PersistenceWorker(QString stateDirectory, QObject *parent = nullptr);

public slots:
    void initialize();
    void shutdown();

signals:
    void ready();
    void errorOccurred(const QString &message);
    void stopped();

private:
    QString m_stateDirectory;
    bool m_ready = false;
};

} // namespace suprai::persistence
