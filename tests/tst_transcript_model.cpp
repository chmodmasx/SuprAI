#include "TranscriptModel.h"

#include <suprai/domain/ConversationItem.h>

#include <QTest>

class TranscriptModelTest final : public QObject
{
    Q_OBJECT

private slots:
    void streamsIntoOneMessageItem()
    {
        suprai::ui::internal::TranscriptModel model;

        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant,
            {},
            suprai::domain::ConversationItemState::Streaming,
            QStringLiteral("assistant-1")));

        model.appendDelta(QStringLiteral("assistant-1"), QStringLiteral("hola"));
        model.appendDelta(QStringLiteral("assistant-1"), QStringLiteral(" mundo"));

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(
            model.data(
                model.index(0),
                suprai::ui::internal::TranscriptModel::TextRole).toString(),
            QStringLiteral("hola mundo"));

        model.finish(QStringLiteral("assistant-1"));
        QCOMPARE(
            model.data(
                model.index(0),
                suprai::ui::internal::TranscriptModel::StreamingRole).toBool(),
            false);
    }

    void ignoresNonMessageDomainItems()
    {
        suprai::ui::internal::TranscriptModel model;

        model.append({
            .id = QStringLiteral("tool-call-1"),
            .state = suprai::domain::ConversationItemState::Completed,
            .content = suprai::domain::ToolCallContent{
                .toolInvocationId = QStringLiteral("tool-1"),
                .name = QStringLiteral("read_file"),
                .arguments = {
                    {QStringLiteral("path"), QStringLiteral("/tmp/test")},
                },
            },
        });

        QCOMPARE(model.rowCount(), 0);
    }

    void clearResetsModel()
    {
        suprai::ui::internal::TranscriptModel model;
        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::User,
            QStringLiteral("hola"),
            suprai::domain::ConversationItemState::Completed,
            QStringLiteral("user-1")));

        model.clear();
        QCOMPARE(model.rowCount(), 0);
    }
};

QTEST_MAIN(TranscriptModelTest)
#include "tst_transcript_model.moc"
