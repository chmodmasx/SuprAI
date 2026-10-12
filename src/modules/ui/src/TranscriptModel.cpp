#include "TranscriptModel.h"
#include "SafeMarkdownRenderer.h"
#include "MarkdownBlocks.h"

#include <utility>

namespace suprai::ui::internal {

namespace {
// Streaming previews are intentionally smaller than final Markdown projections:
// coalescing avoids per-token reparses, while the size bound avoids expensive
// synchronous rich formatting in the GUI thread for very long generations.
constexpr qsizetype kMarkdownLimit = 65536;
constexpr qsizetype kStreamingPreviewLimit = 32768;
constexpr int kPreviewIntervalMs = 90;

QString itemStateName(suprai::domain::ConversationItemState state)
{
    using suprai::domain::ConversationItemState;
    switch (state) {
    case ConversationItemState::Pending: return QStringLiteral("pending");
    case ConversationItemState::Streaming: return QStringLiteral("streaming");
    case ConversationItemState::Completed: return QStringLiteral("completed");
    case ConversationItemState::Failed: return QStringLiteral("failed");
    case ConversationItemState::Cancelled: return QStringLiteral("cancelled");
    }
    return QStringLiteral("failed");
}
} // namespace

TranscriptModel::TranscriptModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_previewTimer.setSingleShot(true);
    connect(&m_previewTimer, &QTimer::timeout, this,
            &TranscriptModel::flushStreamingPreview);
}

int TranscriptModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant TranscriptModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }

    const auto &item = m_items.at(index.row());
    const auto *message = suprai::domain::messageContent(item);
    if (!message) {
        return {};
    }

    switch (role) {
    case IdRole:
        return item.id;
    case SpeakerRole:
        return suprai::domain::roleName(message->role);
    case TextRole:
        return message->text;
    case StreamingRole:
        return suprai::domain::isStreaming(item);
    case StateRole:
        return itemStateName(item.state);
    case DisplayTextRole:
        return m_styledRows.at(index.row()) ? m_styledText.at(index.row()) : message->text;
    case StyledTextRole:
        return m_styledRows.at(index.row());
    case MessageBlocksRole:
        return m_messageBlocks.at(index.row());
    default:
        return {};
    }
}

QHash<int, QByteArray> TranscriptModel::roleNames() const
{
    return {
        {IdRole, "itemId"},
        {SpeakerRole, "speaker"},
        {TextRole, "text"},
        {StreamingRole, "streaming"},
        {StateRole, "messageState"},
        {DisplayTextRole, "displayText"},
        {StyledTextRole, "displayStyled"},
        {MessageBlocksRole, "messageBlocks"},
    };
}

void TranscriptModel::append(const suprai::domain::ConversationItem &item)
{
    if (suprai::domain::itemKind(item) != suprai::domain::ConversationItemKind::Message) {
        return;
    }

    if (m_rowsById.contains(item.id)) {
        return;
    }

    const int row = m_items.size();
    beginInsertRows({}, row, row);
    m_items.push_back(item);
    m_styledText.push_back({});
    m_styledRows.push_back(false);
    m_messageBlocks.push_back({});
    updateCompletedProjection(row);
    m_rowsById.insert(item.id, row);
    endInsertRows();
}

void TranscriptModel::appendDelta(const QString &itemId, const QString &delta)
{
    const int row = rowForId(itemId);
    if (row < 0) {
        return;
    }

    auto *message = suprai::domain::messageContent(m_items[row]);
    if (!message || m_items[row].state != suprai::domain::ConversationItemState::Streaming) {
        return;
    }

    if (delta.isEmpty()) return;
    message->text += delta;
    m_pendingPreviewIds.insert(itemId);
    if (!m_previewTimer.isActive()) m_previewTimer.start(kPreviewIntervalMs);
}

void TranscriptModel::finish(const QString &itemId)
{
    const int row = rowForId(itemId);
    if (row < 0 || !suprai::domain::isStreaming(m_items[row])) {
        return;
    }

    m_pendingPreviewIds.remove(itemId);
    m_items[row].state = suprai::domain::ConversationItemState::Completed;
    updateCompletedProjection(row);
    const auto modelIndex = index(row);
    emit dataChanged(modelIndex, modelIndex, {StreamingRole, StateRole, DisplayTextRole, StyledTextRole, MessageBlocksRole});
}

void TranscriptModel::stop(
    const QString &itemId,
    suprai::domain::ConversationItemState terminalState)
{
    if (terminalState != suprai::domain::ConversationItemState::Failed
        && terminalState != suprai::domain::ConversationItemState::Cancelled) {
        return;
    }
    const int row = rowForId(itemId);
    if (row < 0 || !suprai::domain::isStreaming(m_items[row])) {
        return;
    }
    m_pendingPreviewIds.remove(itemId);
    m_items[row].state = terminalState;
    // Failed/cancelled streams use the plain-text error projection.
    m_messageBlocks[row].clear();
    m_styledText[row].clear();
    m_styledRows[row] = false;
    const auto modelIndex = index(row);
    emit dataChanged(modelIndex, modelIndex,
                     {TextRole, StreamingRole, StateRole, DisplayTextRole,
                      StyledTextRole, MessageBlocksRole});
}

void TranscriptModel::clear()
{
    m_previewTimer.stop();
    m_pendingPreviewIds.clear();
    if (m_items.isEmpty()) {
        return;
    }

    beginResetModel();
    m_items.clear();
    m_styledText.clear();
    m_styledRows.clear();
    m_messageBlocks.clear();
    m_rowsById.clear();
    endResetModel();
}

void TranscriptModel::updateCompletedProjection(int row)
{
    const auto &item = m_items.at(row);
    const auto *message = suprai::domain::messageContent(item);
    if (!message || message->role != suprai::domain::ConversationRole::Assistant
        || item.state != suprai::domain::ConversationItemState::Completed
        || message->text.size() > kMarkdownLimit) {
        m_styledText[row].clear();
        m_styledRows[row] = false;
        m_messageBlocks[row].clear();
        return;
    }
    m_styledText[row] = safeMarkdownToStyledText(message->text);
    m_styledRows[row] = true;
    m_messageBlocks[row] = safeMarkdownBlocks(message->text);
}

void TranscriptModel::flushStreamingPreview()
{
    // Only the changed streaming rows are projected, never the whole history.
    // The complete canonical text is kept even when a preview is too large.
    const auto pending = std::exchange(m_pendingPreviewIds, QSet<QString>{});
    for (const QString &itemId : pending) {
        const int row = rowForId(itemId);
        if (row < 0 || !suprai::domain::isStreaming(m_items[row])) continue;

        const auto *message = suprai::domain::messageContent(m_items[row]);
        if (!message) continue;

        if (message->role == suprai::domain::ConversationRole::Assistant
            && message->text.size() <= kStreamingPreviewLimit) {
            m_messageBlocks[row] = safeStreamingMarkdownBlocks(message->text);
        } else {
            m_messageBlocks[row].clear();
        }
        const auto modelIndex = index(row);
        emit dataChanged(modelIndex, modelIndex,
                         {TextRole, DisplayTextRole, MessageBlocksRole});
    }
}

int TranscriptModel::rowForId(const QString &itemId) const
{
    const auto it = m_rowsById.constFind(itemId);
    return it == m_rowsById.cend() ? -1 : it.value();
}

} // namespace suprai::ui::internal
