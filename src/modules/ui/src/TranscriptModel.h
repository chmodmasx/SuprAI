#pragma once

#include <suprai/domain/ConversationItem.h>

#include <QAbstractListModel>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QVector>

namespace suprai::ui::internal {

class TranscriptModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        SpeakerRole,
        TextRole,
        StreamingRole,
        StateRole,
        DisplayTextRole,
        StyledTextRole,
        MessageBlocksRole
    };

    explicit TranscriptModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void append(const suprai::domain::ConversationItem &item);
    void appendDelta(const QString &itemId, const QString &delta);
    void finish(const QString &itemId);
    void stop(const QString &itemId, suprai::domain::ConversationItemState terminalState);
    void clear();

private:
    int rowForId(const QString &itemId) const;
    void updateCompletedProjection(int row);
    void flushStreamingPreview();

    QVector<QString> m_styledText;
    QVector<bool> m_styledRows;
    QVector<QVariantList> m_messageBlocks;
    QVector<suprai::domain::ConversationItem> m_items;
    QHash<QString, int> m_rowsById;
    QSet<QString> m_pendingPreviewIds;
    QTimer m_previewTimer;
};

} // namespace suprai::ui::internal
