#pragma once

#include <suprai/domain/ConversationItem.h>

#include <QAbstractListModel>
#include <QHash>
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
        StateRole
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

    QVector<suprai::domain::ConversationItem> m_items;
    QHash<QString, int> m_rowsById;
};

} // namespace suprai::ui::internal
