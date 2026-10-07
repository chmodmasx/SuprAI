#pragma once

#include "domain/ConversationItem.h"

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

namespace suprai::ui {

class TranscriptModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        SpeakerRole,
        TextRole,
        StreamingRole
    };

    explicit TranscriptModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void append(const suprai::domain::ConversationItem &item);
    void appendDelta(const QString &itemId, const QString &delta);
    void finish(const QString &itemId);
    void clear();

private:
    int rowForId(const QString &itemId) const;
    void rebuildIndex();

    QVector<suprai::domain::ConversationItem> m_items;
    QHash<QString, int> m_rowsById;
};

} // namespace suprai::ui
