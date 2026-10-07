#include "ui/TranscriptModel.h"

namespace suprai::ui {

TranscriptModel::TranscriptModel(QObject *parent)
    : QAbstractListModel(parent)
{
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

    switch (role) {
    case IdRole:
        return item.id;
    case SpeakerRole:
        return suprai::domain::roleName(item.role);
    case TextRole:
        return item.text;
    case StreamingRole:
        return item.streaming;
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
    };
}

void TranscriptModel::append(const suprai::domain::ConversationItem &item)
{
    const int row = m_items.size();
    beginInsertRows({}, row, row);
    m_items.push_back(item);
    m_rowsById.insert(item.id, row);
    endInsertRows();
}

void TranscriptModel::appendDelta(const QString &itemId, const QString &delta)
{
    const int row = rowForId(itemId);
    if (row < 0) {
        return;
    }

    m_items[row].text += delta;
    const auto modelIndex = index(row);
    emit dataChanged(modelIndex, modelIndex, {TextRole});
}

void TranscriptModel::finish(const QString &itemId)
{
    const int row = rowForId(itemId);
    if (row < 0) {
        return;
    }

    if (!m_items[row].streaming) {
        return;
    }

    m_items[row].streaming = false;
    const auto modelIndex = index(row);
    emit dataChanged(modelIndex, modelIndex, {StreamingRole});
}

void TranscriptModel::clear()
{
    if (m_items.isEmpty()) {
        return;
    }

    beginResetModel();
    m_items.clear();
    m_rowsById.clear();
    endResetModel();
}

int TranscriptModel::rowForId(const QString &itemId) const
{
    const auto it = m_rowsById.constFind(itemId);
    return it == m_rowsById.cend() ? -1 : it.value();
}

void TranscriptModel::rebuildIndex()
{
    m_rowsById.clear();
    for (int row = 0; row < m_items.size(); ++row) {
        m_rowsById.insert(m_items.at(row).id, row);
    }
}

} // namespace suprai::ui
