#include <suprai/persistence/PersistenceWorker.h>

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QUuid>
#include <QVariant>

#include <utility>

namespace suprai::persistence {

namespace {

constexpr int CurrentSchemaVersion = 1;

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
        *errorMessage = QStringLiteral("%1 | SQL: %2")
                            .arg(query.lastError().text(), sql);
    }
    return false;
}

} // namespace

PersistenceWorker::PersistenceWorker(QString stateDirectory, QObject *parent)
    : QObject(parent)
    , m_stateDirectory(std::move(stateDirectory))
    , m_databasePath(
          m_stateDirectory.isEmpty()
              ? QString{}
              : QDir(m_stateDirectory).filePath(QStringLiteral("suprai.sqlite3")))
    , m_connectionName(
          QStringLiteral("suprai-writer-%1")
              .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
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
        || !verifyFts5(&errorMessage)
        || !migrate(&errorMessage)
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

    if (m_ready || m_database.isValid()) {
        closeDatabase();
    }

    m_ready = false;
    emit stopped();
}

bool PersistenceWorker::openDatabase(QString *errorMessage)
{
    Q_ASSERT(thread() == QThread::currentThread());

    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "El driver SQLite de Qt (QSQLITE) no está disponible.");
        }
        return false;
    }

    m_database = QSqlDatabase::addDatabase(
        QStringLiteral("QSQLITE"),
        m_connectionName);
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
    Q_ASSERT(thread() == QThread::currentThread());

    if (!execSql(m_database, QStringLiteral("PRAGMA foreign_keys = ON"), errorMessage)
        || !execSql(m_database, QStringLiteral("PRAGMA busy_timeout = 5000"), errorMessage)) {
        return false;
    }

    QSqlQuery journalQuery(m_database);
    if (!journalQuery.exec(QStringLiteral("PRAGMA journal_mode = WAL"))
        || !journalQuery.next()
        || journalQuery.value(0).toString().compare(
               QStringLiteral("wal"),
               Qt::CaseInsensitive) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SQLite no pudo activar WAL: %1")
                                .arg(journalQuery.lastError().text());
        }
        return false;
    }

    return execSql(
        m_database,
        QStringLiteral("PRAGMA synchronous = NORMAL"),
        errorMessage);
}

bool PersistenceWorker::migrate(QString *errorMessage)
{
    Q_ASSERT(thread() == QThread::currentThread());

    QSqlQuery versionQuery(m_database);
    if (!versionQuery.exec(QStringLiteral("PRAGMA user_version"))
        || !versionQuery.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No se pudo leer PRAGMA user_version: %1")
                                .arg(versionQuery.lastError().text());
        }
        return false;
    }

    const int version = versionQuery.value(0).toInt();

    if (version > CurrentSchemaVersion) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "La base de datos usa schema v%1, más nuevo que el soportado v%2.")
                                .arg(version)
                                .arg(CurrentSchemaVersion);
        }
        return false;
    }

    if (version == 0 && !migrateToV1(errorMessage)) {
        return false;
    }

    QSqlQuery finalVersionQuery(m_database);
    if (!finalVersionQuery.exec(QStringLiteral("PRAGMA user_version"))
        || !finalVersionQuery.next()
        || finalVersionQuery.value(0).toInt() != CurrentSchemaVersion) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "La migración SQLite no dejó el schema en la versión esperada v%1.")
                                .arg(CurrentSchemaVersion);
        }
        return false;
    }

    return true;
}

bool PersistenceWorker::migrateToV1(QString *errorMessage)
{
    Q_ASSERT(thread() == QThread::currentThread());

    if (!m_database.transaction()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No se pudo iniciar la migración SQLite v1: %1")
                                .arg(m_database.lastError().text());
        }
        return false;
    }

    const QStringList statements = {
        QStringLiteral(
            "CREATE TABLE schema_migrations ("
            "version INTEGER PRIMARY KEY,"
            "applied_at TEXT NOT NULL DEFAULT "
            "(strftime('%Y-%m-%dT%H:%M:%fZ','now'))"
            ")"),
        QStringLiteral(
            "CREATE TABLE sessions ("
            "id TEXT PRIMARY KEY,"
            "parent_session_id TEXT REFERENCES sessions(id) ON DELETE SET NULL,"
            "created_at TEXT NOT NULL DEFAULT "
            "(strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
            "updated_at TEXT NOT NULL DEFAULT "
            "(strftime('%Y-%m-%dT%H:%M:%fZ','now'))"
            ")"),
        QStringLiteral(
            "CREATE TABLE inputs ("
            "id TEXT PRIMARY KEY,"
            "session_id TEXT NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,"
            "sequence INTEGER NOT NULL,"
            "text TEXT NOT NULL,"
            "created_at TEXT NOT NULL DEFAULT "
            "(strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
            "UNIQUE(session_id, sequence)"
            ")"),
        QStringLiteral(
            "CREATE TABLE turns ("
            "id TEXT PRIMARY KEY,"
            "session_id TEXT NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,"
            "input_id TEXT NOT NULL REFERENCES inputs(id) ON DELETE RESTRICT,"
            "parent_turn_id TEXT REFERENCES turns(id) ON DELETE SET NULL,"
            "sequence INTEGER NOT NULL,"
            "created_at TEXT NOT NULL DEFAULT "
            "(strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
            "UNIQUE(session_id, sequence)"
            ")"),
        QStringLiteral(
            "CREATE TABLE runs ("
            "id TEXT PRIMARY KEY,"
            "turn_id TEXT NOT NULL REFERENCES turns(id) ON DELETE CASCADE,"
            "generation INTEGER NOT NULL CHECK(generation >= 1),"
            "status TEXT NOT NULL,"
            "created_at TEXT NOT NULL DEFAULT "
            "(strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
            "updated_at TEXT NOT NULL DEFAULT "
            "(strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
            "UNIQUE(turn_id, generation)"
            ")"),
        QStringLiteral(
            "CREATE TABLE conversation_items ("
            "id TEXT PRIMARY KEY,"
            "turn_id TEXT NOT NULL REFERENCES turns(id) ON DELETE CASCADE,"
            "sequence INTEGER NOT NULL,"
            "kind TEXT NOT NULL,"
            "state TEXT NOT NULL,"
            "payload_json TEXT NOT NULL,"
            "created_at TEXT NOT NULL DEFAULT "
            "(strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
            "UNIQUE(turn_id, sequence)"
            ")"),
        QStringLiteral(
            "CREATE INDEX idx_inputs_session "
            "ON inputs(session_id, sequence)"),
        QStringLiteral(
            "CREATE INDEX idx_turns_session "
            "ON turns(session_id, sequence)"),
        QStringLiteral(
            "CREATE INDEX idx_runs_turn "
            "ON runs(turn_id, generation)"),
        QStringLiteral(
            "CREATE INDEX idx_items_turn "
            "ON conversation_items(turn_id, sequence)"),
        QStringLiteral(
            "CREATE VIRTUAL TABLE conversation_items_fts USING fts5("
            "item_id UNINDEXED,"
            "session_id UNINDEXED,"
            "text,"
            "tokenize='unicode61'"
            ")"),
        QStringLiteral(
            "INSERT INTO schema_migrations(version) VALUES(1)"),
        QStringLiteral("PRAGMA user_version = 1"),
    };

    for (const auto &statement : statements) {
        if (!execSql(m_database, statement, errorMessage)) {
            m_database.rollback();
            return false;
        }
    }

    if (m_database.commit()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("No se pudo confirmar la migración SQLite v1: %1")
                            .arg(m_database.lastError().text());
    }
    m_database.rollback();
    return false;
}

bool PersistenceWorker::verifyFts5(QString *errorMessage)
{
    Q_ASSERT(thread() == QThread::currentThread());

    const QString probeName = QStringLiteral("__suprai_fts5_probe");

    if (!execSql(
            m_database,
            QStringLiteral(
                "CREATE VIRTUAL TABLE temp.%1 USING fts5(content)")
                .arg(probeName),
            errorMessage)) {
        if (errorMessage && !errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("SQLite FTS5 no está disponible: %1")
                                .arg(*errorMessage);
        }
        return false;
    }

    const bool dropped = execSql(
        m_database,
        QStringLiteral("DROP TABLE temp.%1").arg(probeName),
        errorMessage);

    return dropped;
}

bool PersistenceWorker::verifyDatabase(QString *errorMessage)
{
    Q_ASSERT(thread() == QThread::currentThread());

    QSqlQuery foreignKeysQuery(m_database);
    if (!foreignKeysQuery.exec(QStringLiteral("PRAGMA foreign_keys"))
        || !foreignKeysQuery.next()
        || foreignKeysQuery.value(0).toInt() != 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SQLite foreign_keys no está activo.");
        }
        return false;
    }

    QSqlQuery journalQuery(m_database);
    if (!journalQuery.exec(QStringLiteral("PRAGMA journal_mode"))
        || !journalQuery.next()
        || journalQuery.value(0).toString().compare(
               QStringLiteral("wal"),
               Qt::CaseInsensitive) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SQLite no está operando en WAL.");
        }
        return false;
    }

    QSqlQuery quickCheck(m_database);
    if (!quickCheck.exec(QStringLiteral("PRAGMA quick_check"))
        || !quickCheck.next()
        || quickCheck.value(0).toString() != QStringLiteral("ok")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SQLite quick_check falló: %1")
                                .arg(quickCheck.lastError().text());
        }
        return false;
    }

    QSqlQuery foreignKeyCheck(m_database);
    if (!foreignKeyCheck.exec(QStringLiteral("PRAGMA foreign_key_check"))) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SQLite foreign_key_check falló: %1")
                                .arg(foreignKeyCheck.lastError().text());
        }
        return false;
    }

    if (foreignKeyCheck.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "SQLite detectó una violación de clave foránea en %1.")
                                .arg(foreignKeyCheck.value(0).toString());
        }
        return false;
    }

    return true;
}

void PersistenceWorker::closeDatabase()
{
    Q_ASSERT(thread() == QThread::currentThread());

    if (m_database.isValid()) {
        m_database.close();
        m_database = QSqlDatabase{};
    }

    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

} // namespace suprai::persistence
