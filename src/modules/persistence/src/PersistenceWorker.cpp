#include <suprai/persistence/PersistenceWorker.h>

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QUuid>

#include <utility>

namespace suprai::persistence {

namespace {

constexpr int CurrentSchemaVersion = 1;
constexpr int BusyTimeoutMs = 5000;

bool execSql(
    QSqlDatabase &database,
    const QString &sql,
    QString *errorMessage)
{
    QSqlQuery query(database);
    if (query.exec(sql)) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("SQLite error: %1 | SQL: %2")
                            .arg(query.lastError().text(), sql);
    }
    return false;
}

} // namespace

PersistenceWorker::PersistenceWorker(QString stateDirectory, QObject *parent)
    : QObject(parent)
    , m_stateDirectory(std::move(stateDirectory))
    , m_databasePath(QDir(m_stateDirectory).filePath(QStringLiteral("suprai.sqlite3")))
{
}

QString PersistenceWorker::databasePath() const
{
    return m_databasePath;
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

    QString errorMessage;

    if (!openDatabase(&errorMessage)
        || !configureDatabase(&errorMessage)
        || !migrate(&errorMessage)
        || !verifyFts5(&errorMessage)
        || !verifyDatabase(&errorMessage)) {
        closeDatabase();
        emit errorOccurred(errorMessage);
        return;
    }

    m_ready = true;
    emit ready();
}

void PersistenceWorker::shutdown()
{
    Q_ASSERT(thread() == QThread::currentThread());

    m_ready = false;
    closeDatabase();
    emit stopped();
}

bool PersistenceWorker::openDatabase(QString *errorMessage)
{
    if (m_database.isValid() && m_database.isOpen()) {
        return true;
    }

    m_connectionName =
        QStringLiteral("suprai-writer-%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setDatabaseName(m_databasePath);

    if (m_database.open()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("No se pudo abrir SQLite en %1: %2")
                            .arg(m_databasePath, m_database.lastError().text());
    }
    return false;
}

bool PersistenceWorker::configureDatabase(QString *errorMessage)
{
    QSqlQuery walQuery(m_database);
    if (!walQuery.exec(QStringLiteral("PRAGMA journal_mode=WAL"))
        || !walQuery.next()
        || walQuery.value(0).toString().compare(
               QStringLiteral("wal"),
               Qt::CaseInsensitive) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SQLite no pudo activar WAL: %1")
                                .arg(walQuery.lastError().text());
        }
        return false;
    }

    if (!execSql(m_database, QStringLiteral("PRAGMA foreign_keys=ON"), errorMessage)
        || !execSql(
            m_database,
            QStringLiteral("PRAGMA busy_timeout=%1").arg(BusyTimeoutMs),
            errorMessage)) {
        return false;
    }

    QSqlQuery foreignKeys(m_database);
    if (!foreignKeys.exec(QStringLiteral("PRAGMA foreign_keys"))
        || !foreignKeys.next()
        || foreignKeys.value(0).toInt() != 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SQLite foreign_keys no quedó habilitado.");
        }
        return false;
    }

    return true;
}

bool PersistenceWorker::migrate(QString *errorMessage)
{
    QSqlQuery versionQuery(m_database);
    if (!versionQuery.exec(QStringLiteral("PRAGMA user_version"))
        || !versionQuery.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No se pudo leer SQLite user_version: %1")
                                .arg(versionQuery.lastError().text());
        }
        return false;
    }

    const int version = versionQuery.value(0).toInt();

    if (version > CurrentSchemaVersion) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("La base usa schema %1 pero esta versión de SuprAI soporta hasta %2.")
                    .arg(version)
                    .arg(CurrentSchemaVersion);
        }
        return false;
    }

    if (version == 0) {
        return migrateToV1(errorMessage);
    }

    return true;
}

bool PersistenceWorker::migrateToV1(QString *errorMessage)
{
    if (!m_database.transaction()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No se pudo iniciar migración SQLite: %1")
                                .arg(m_database.lastError().text());
        }
        return false;
    }

    const QStringList statements{
        QStringLiteral(
            "CREATE TABLE sessions ("
            "id TEXT PRIMARY KEY,"
            "parent_session_id TEXT REFERENCES sessions(id),"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ")"),
        QStringLiteral(
            "CREATE TABLE inputs ("
            "id TEXT PRIMARY KEY,"
            "session_id TEXT NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,"
            "text TEXT NOT NULL,"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ")"),
        QStringLiteral(
            "CREATE TABLE turns ("
            "id TEXT PRIMARY KEY,"
            "session_id TEXT NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,"
            "input_id TEXT NOT NULL REFERENCES inputs(id),"
            "parent_turn_id TEXT REFERENCES turns(id),"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ")"),
        QStringLiteral(
            "CREATE TABLE runs ("
            "id TEXT PRIMARY KEY,"
            "turn_id TEXT NOT NULL REFERENCES turns(id) ON DELETE CASCADE,"
            "generation INTEGER NOT NULL CHECK(generation > 0),"
            "state TEXT NOT NULL DEFAULT 'created',"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "UNIQUE(turn_id, generation)"
            ")"),
        QStringLiteral(
            "CREATE TABLE conversation_items ("
            "id TEXT PRIMARY KEY,"
            "session_id TEXT NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,"
            "turn_id TEXT NOT NULL REFERENCES turns(id) ON DELETE CASCADE,"
            "ordinal INTEGER NOT NULL CHECK(ordinal >= 0),"
            "kind TEXT NOT NULL,"
            "state TEXT NOT NULL,"
            "payload_json TEXT NOT NULL,"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "UNIQUE(session_id, ordinal)"
            ")"),
        QStringLiteral(
            "CREATE INDEX idx_inputs_session ON inputs(session_id, created_at)"),
        QStringLiteral(
            "CREATE INDEX idx_turns_session ON turns(session_id, created_at)"),
        QStringLiteral(
            "CREATE INDEX idx_runs_turn ON runs(turn_id, generation)"),
        QStringLiteral(
            "CREATE INDEX idx_items_turn ON conversation_items(turn_id, ordinal)"),
        QStringLiteral(
            "CREATE VIRTUAL TABLE conversation_fts USING fts5("
            "item_id UNINDEXED,"
            "session_id UNINDEXED,"
            "text"
            ")"),
        QStringLiteral("PRAGMA user_version=1"),
    };

    for (const auto &statement : statements) {
        if (!execSql(m_database, statement, errorMessage)) {
            m_database.rollback();
            return false;
        }
    }

    if (!m_database.commit()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No se pudo confirmar migración SQLite: %1")
                                .arg(m_database.lastError().text());
        }
        m_database.rollback();
        return false;
    }

    return true;
}

bool PersistenceWorker::verifyFts5(QString *errorMessage)
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("SELECT count(*) FROM conversation_fts"))
        || !query.next()) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("FTS5 no está disponible en la SQLite empaquetada: %1")
                    .arg(query.lastError().text());
        }
        return false;
    }

    return true;
}

bool PersistenceWorker::verifyDatabase(QString *errorMessage)
{
    QSqlQuery versionQuery(m_database);
    if (!versionQuery.exec(QStringLiteral("PRAGMA user_version"))
        || !versionQuery.next()
        || versionQuery.value(0).toInt() != CurrentSchemaVersion) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SQLite schema version inválido.");
        }
        return false;
    }

    QSqlQuery checkQuery(m_database);
    if (!checkQuery.exec(QStringLiteral("PRAGMA quick_check"))
        || !checkQuery.next()
        || checkQuery.value(0).toString() != QStringLiteral("ok")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SQLite quick_check falló: %1")
                                .arg(checkQuery.lastError().text());
        }
        return false;
    }

    return true;
}

void PersistenceWorker::closeDatabase()
{
    if (!m_database.isValid()) {
        return;
    }

    const QString connectionName = m_connectionName;

    if (m_database.isOpen()) {
        m_database.close();
    }

    m_database = QSqlDatabase{};
    m_connectionName.clear();

    if (!connectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(connectionName);
    }
}

} // namespace suprai::persistence
