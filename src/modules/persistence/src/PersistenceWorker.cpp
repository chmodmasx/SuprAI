#include <suprai/persistence/PersistenceWorker.h>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <QVariant>
#include <QVariantList>

#include <type_traits>
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

bool execPrepared(
    QSqlDatabase &database,
    const QString &sql,
    const QVariantList &bindings,
    QString *errorMessage)
{
    QSqlQuery query(database);
    if (!query.prepare(sql)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 | SQL: %2")
                                .arg(query.lastError().text(), sql);
        }
        return false;
    }

    for (const auto &binding : bindings) {
        query.addBindValue(binding);
    }

    if (query.exec()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("%1 | SQL: %2")
                            .arg(query.lastError().text(), sql);
    }
    return false;
}

QVariant nullableText(const QString &value)
{
    return value.isEmpty() ? QVariant{} : QVariant{value};
}

QString itemKindName(suprai::domain::ConversationItemKind kind)
{
    using suprai::domain::ConversationItemKind;

    switch (kind) {
    case ConversationItemKind::Message:
        return QStringLiteral("message");
    case ConversationItemKind::ReasoningSummary:
        return QStringLiteral("reasoning_summary");
    case ConversationItemKind::ToolCall:
        return QStringLiteral("tool_call");
    case ConversationItemKind::ToolResult:
        return QStringLiteral("tool_result");
    case ConversationItemKind::Attachment:
        return QStringLiteral("attachment");
    case ConversationItemKind::RuntimeAnnotation:
        return QStringLiteral("runtime_annotation");
    }

    return QStringLiteral("runtime_annotation");
}

QString itemStateName(suprai::domain::ConversationItemState state)
{
    using suprai::domain::ConversationItemState;

    switch (state) {
    case ConversationItemState::Pending:
        return QStringLiteral("pending");
    case ConversationItemState::Streaming:
        return QStringLiteral("streaming");
    case ConversationItemState::Completed:
        return QStringLiteral("completed");
    case ConversationItemState::Failed:
        return QStringLiteral("failed");
    case ConversationItemState::Cancelled:
        return QStringLiteral("cancelled");
    }

    return QStringLiteral("failed");
}

QString toolResultStatusName(suprai::domain::ToolResultStatus status)
{
    using suprai::domain::ToolResultStatus;

    switch (status) {
    case ToolResultStatus::Success:
        return QStringLiteral("success");
    case ToolResultStatus::Error:
        return QStringLiteral("error");
    case ToolResultStatus::Denied:
        return QStringLiteral("denied");
    case ToolResultStatus::Cancelled:
        return QStringLiteral("cancelled");
    case ToolResultStatus::SkippedBySteering:
        return QStringLiteral("skipped_by_steering");
    case ToolResultStatus::OutcomeUnknown:
        return QStringLiteral("outcome_unknown");
    }

    return QStringLiteral("error");
}

QJsonObject itemPayload(const suprai::domain::ConversationItem &item)
{
    return std::visit(
        [](const auto &content) -> QJsonObject {
            using T = std::decay_t<decltype(content)>;

            if constexpr (std::is_same_v<T, suprai::domain::MessageContent>) {
                return {
                    {QStringLiteral("role"), suprai::domain::roleName(content.role)},
                    {QStringLiteral("text"), content.text},
                };
            } else if constexpr (std::is_same_v<T, suprai::domain::ReasoningSummaryContent>) {
                return {
                    {QStringLiteral("summary"), content.summary},
                };
            } else if constexpr (std::is_same_v<T, suprai::domain::ToolCallContent>) {
                return {
                    {QStringLiteral("toolInvocationId"), content.toolInvocationId},
                    {QStringLiteral("name"), content.name},
                    {QStringLiteral("arguments"), content.arguments},
                };
            } else if constexpr (std::is_same_v<T, suprai::domain::ToolResultContent>) {
                return {
                    {QStringLiteral("toolInvocationId"), content.toolInvocationId},
                    {QStringLiteral("status"), toolResultStatusName(content.status)},
                    {QStringLiteral("text"), content.text},
                };
            } else if constexpr (std::is_same_v<T, suprai::domain::AttachmentContent>) {
                return {
                    {QStringLiteral("attachmentId"), content.attachmentId},
                    {QStringLiteral("displayName"), content.displayName},
                    {QStringLiteral("mimeType"), content.mimeType},
                };
            } else {
                return {
                    {QStringLiteral("code"), content.code},
                    {QStringLiteral("text"), content.text},
                };
            }
        },
        item.content);
}

QString indexText(const suprai::domain::ConversationItem &item)
{
    return std::visit(
        [](const auto &content) -> QString {
            using T = std::decay_t<decltype(content)>;

            if constexpr (std::is_same_v<T, suprai::domain::MessageContent>) {
                return content.text;
            } else if constexpr (std::is_same_v<T, suprai::domain::ReasoningSummaryContent>) {
                return content.summary;
            } else if constexpr (std::is_same_v<T, suprai::domain::ToolResultContent>) {
                return content.text;
            } else if constexpr (std::is_same_v<T, suprai::domain::RuntimeAnnotationContent>) {
                return content.text;
            } else {
                return {};
            }
        },
        item.content);
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

void PersistenceWorker::persistTurnStart(TurnStartWrite request)
{
    Q_ASSERT(thread() == QThread::currentThread());

    const auto fail = [this, &request](const QString &message) {
        emit writeFailed(request.requestId, message);
    };

    if (!m_ready || !m_database.isOpen()) {
        fail(QStringLiteral("SQLite no está listo para persistir el turno."));
        return;
    }

    if (request.requestId.isEmpty()
        || request.session.id.isEmpty()
        || request.input.id.isEmpty()
        || request.input.sessionId != request.session.id
        || request.input.sequence < 1
        || request.turn.id.isEmpty()
        || request.turn.sessionId != request.session.id
        || request.turn.inputId != request.input.id
        || request.turn.sequence < 1
        || request.run.id.isEmpty()
        || request.run.turnId != request.turn.id
        || request.run.generation < 1
        || request.userItem.id.isEmpty()
        || request.userItem.turnId != request.turn.id
        || request.userItem.sequence < 1) {
        fail(QStringLiteral("El lote durable de inicio de turno es inválido."));
        return;
    }

    if (!m_database.transaction()) {
        fail(QStringLiteral("No se pudo iniciar la transacción del turno: %1")
                 .arg(m_database.lastError().text()));
        return;
    }

    QString errorMessage;

    if (!execPrepared(
            m_database,
            QStringLiteral(
                "INSERT INTO sessions(id,parent_session_id) VALUES(?,?) "
                "ON CONFLICT(id) DO UPDATE SET "
                "updated_at=strftime('%Y-%m-%dT%H:%M:%fZ','now')"),
            {
                request.session.id,
                nullableText(request.session.parentSessionId),
            },
            &errorMessage)
        || !execPrepared(
            m_database,
            QStringLiteral(
                "INSERT INTO inputs(id,session_id,sequence,text) "
                "VALUES(?,?,?,?)"),
            {
                request.input.id,
                request.input.sessionId,
                request.input.sequence,
                request.input.text,
            },
            &errorMessage)
        || !execPrepared(
            m_database,
            QStringLiteral(
                "INSERT INTO turns(id,session_id,input_id,parent_turn_id,sequence) "
                "VALUES(?,?,?,?,?)"),
            {
                request.turn.id,
                request.turn.sessionId,
                request.turn.inputId,
                nullableText(request.turn.parentTurnId),
                request.turn.sequence,
            },
            &errorMessage)
        || !execPrepared(
            m_database,
            QStringLiteral(
                "INSERT INTO runs(id,turn_id,generation,status) "
                "VALUES(?,?,?,?)"),
            {
                request.run.id,
                request.run.turnId,
                request.run.generation,
                QStringLiteral("prepared"),
            },
            &errorMessage)) {
        m_database.rollback();
        fail(errorMessage);
        return;
    }

    const QString payloadJson = QString::fromUtf8(
        QJsonDocument(itemPayload(request.userItem))
            .toJson(QJsonDocument::Compact));

    if (!execPrepared(
            m_database,
            QStringLiteral(
                "INSERT INTO conversation_items("
                "id,turn_id,sequence,kind,state,payload_json"
                ") VALUES(?,?,?,?,?,?)"),
            {
                request.userItem.id,
                request.userItem.turnId,
                request.userItem.sequence,
                itemKindName(suprai::domain::itemKind(request.userItem)),
                itemStateName(request.userItem.state),
                payloadJson,
            },
            &errorMessage)) {
        m_database.rollback();
        fail(errorMessage);
        return;
    }

    const QString searchableText = indexText(request.userItem);
    if (!searchableText.isEmpty()
        && !execPrepared(
            m_database,
            QStringLiteral(
                "INSERT INTO conversation_items_fts(item_id,session_id,text) "
                "VALUES(?,?,?)"),
            {
                request.userItem.id,
                request.session.id,
                searchableText,
            },
            &errorMessage)) {
        m_database.rollback();
        fail(errorMessage);
        return;
    }

    if (!m_database.commit()) {
        errorMessage = QStringLiteral("No se pudo confirmar el inicio durable del turno: %1")
                           .arg(m_database.lastError().text());
        m_database.rollback();
        fail(errorMessage);
        return;
    }

    emit turnStartPersisted(request.requestId);
}

void PersistenceWorker::persistTurnTerminal(TurnTerminalWrite request)
{
    Q_ASSERT(thread() == QThread::currentThread());

    const auto fail = [this, &request](const QString &message) {
        emit writeFailed(request.requestId, message);
    };
    if (!m_ready || !m_database.isOpen()) {
        fail(QStringLiteral("SQLite no está listo para finalizar el turno."));
        return;
    }

    const bool validStatus = request.status == QStringLiteral("completed")
        || request.status == QStringLiteral("failed")
        || request.status == QStringLiteral("cancelled");
    const auto expectedItemState = request.status == QStringLiteral("completed")
        ? suprai::domain::ConversationItemState::Completed
        : request.status == QStringLiteral("failed")
            ? suprai::domain::ConversationItemState::Failed
            : suprai::domain::ConversationItemState::Cancelled;
    if (request.requestId.isEmpty() || request.runId.isEmpty()
        || request.turnId.isEmpty() || request.sessionId.isEmpty() || !validStatus
        || (request.assistantItem && (
            request.assistantItem->id.isEmpty()
            || request.assistantItem->turnId != request.turnId
            || request.assistantItem->sequence < 2
            || request.assistantItem->state != expectedItemState
            || suprai::domain::itemKind(*request.assistantItem)
                != suprai::domain::ConversationItemKind::Message
            || suprai::domain::messageContent(*request.assistantItem)->role
                != suprai::domain::ConversationRole::Assistant))) {
        fail(QStringLiteral("El lote durable terminal del turno es inválido."));
        return;
    }

    if (!m_database.transaction()) {
        fail(QStringLiteral("No se pudo iniciar la transacción terminal: %1")
                 .arg(m_database.lastError().text()));
        return;
    }
    QString errorMessage;
    if (!execPrepared(
            m_database,
            QStringLiteral(
                "UPDATE runs SET status=?, "
                "updated_at=strftime('%Y-%m-%dT%H:%M:%fZ','now') "
                "WHERE id=? AND turn_id=? AND status='prepared' "
                "AND EXISTS (SELECT 1 FROM turns "
                "WHERE turns.id=runs.turn_id AND turns.session_id=?)"),
            {request.status, request.runId, request.turnId, request.sessionId},
            &errorMessage)) {
        m_database.rollback();
        fail(errorMessage);
        return;
    }
    QSqlQuery changes(m_database);
    if (!changes.exec(QStringLiteral("SELECT changes()"))
        || !changes.next() || changes.value(0).toInt() != 1) {
        m_database.rollback();
        fail(QStringLiteral("El Run no existe, no está preparado o ya fue finalizado."));
        return;
    }

    if (request.assistantItem) {
        const auto &item = *request.assistantItem;
        const QString payloadJson = QString::fromUtf8(
            QJsonDocument(itemPayload(item)).toJson(QJsonDocument::Compact));
        if (!execPrepared(
                m_database,
                QStringLiteral(
                    "INSERT INTO conversation_items("
                    "id,turn_id,sequence,kind,state,payload_json"
                    ") VALUES(?,?,?,?,?,?)"),
                {item.id, item.turnId, item.sequence,
                 itemKindName(suprai::domain::itemKind(item)),
                 itemStateName(item.state), payloadJson},
                &errorMessage)) {
            m_database.rollback();
            fail(errorMessage);
            return;
        }

        const QString searchableText = indexText(item);
        if (!searchableText.isEmpty()
            && !execPrepared(
                m_database,
                QStringLiteral(
                    "INSERT INTO conversation_items_fts(item_id,session_id,text) "
                    "VALUES(?,?,?)"),
                {item.id, request.sessionId, searchableText},
                &errorMessage)) {
            m_database.rollback();
            fail(errorMessage);
            return;
        }
    }
    if (!m_database.commit()) {
        errorMessage = QStringLiteral("No se pudo confirmar el cierre durable del turno: %1")
                           .arg(m_database.lastError().text());
        m_database.rollback();
        fail(errorMessage);
        return;
    }
    emit turnTerminalPersisted(request.requestId);
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
