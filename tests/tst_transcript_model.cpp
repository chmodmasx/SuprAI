#include "TranscriptModel.h"

#include <suprai/domain/ConversationItem.h>

#include <QTest>

class TranscriptModelTest final : public QObject
{
    Q_OBJECT

private slots:
    void streamsIntoOneItem()
    {
        suprai::ui::internal::TranscriptModel model;

        model.append({
            .id = QStringLiteral("assistant-1"),
            .role = suprai::domain::ConversationRole::Assistant,
            .text = {},
            .streaming = true,
        });

        model.appendDelta(QStringLiteral("assistant-1"), QStringLiteral("hola"));
        model.appendDelta(QStringLiteral("assistant-1"), QStringLiteral(" mundo"));

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), suprai::ui::internal::TranscriptModel::TextRole).toString(),
                 QStringLiteral("hola mundo"));

        model.finish(QStringLiteral("assistant-1"));
        QCOMPARE(model.data(model.index(0), suprai::ui::internal::TranscriptModel::StreamingRole).toBool(),
                 false);
    }

    void clearResetsModel()
    {
        suprai::ui::internal::TranscriptModel model;
        model.append({
            .id = QStringLiteral("user-1"),
            .role = suprai::domain::ConversationRole::User,
            .text = QStringLiteral("hola"),
            .streaming = false,
        });

        model.clear();
        QCOMPARE(model.rowCount(), 0);
    }
};

QTEST_MAIN(TranscriptModelTest)
#include "tst_transcript_model.moc"
