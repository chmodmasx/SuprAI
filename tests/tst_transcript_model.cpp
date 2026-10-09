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

    void abortedStreamPreservesTerminalFailureState()
    {
        using suprai::domain::ConversationItemState;
        suprai::ui::internal::TranscriptModel model;
        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant, {},
            ConversationItemState::Streaming, QStringLiteral("fail-item")));
        model.appendDelta(QStringLiteral("fail-item"), QStringLiteral("partial"));
        model.stop(QStringLiteral("fail-item"), ConversationItemState::Failed);
        QCOMPARE(model.data(model.index(0),
            suprai::ui::internal::TranscriptModel::StreamingRole).toBool(), false);
        QCOMPARE(model.data(model.index(0),
            suprai::ui::internal::TranscriptModel::StateRole).toString(),
            QStringLiteral("failed"));
        QCOMPARE(model.data(model.index(0),
            suprai::ui::internal::TranscriptModel::TextRole).toString(),
            QStringLiteral("partial"));
    }

    void rendersOnlySafeTagsAfterCompletion()
    {
        using suprai::ui::internal::TranscriptModel;
        TranscriptModel model;
        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant, {},
            suprai::domain::ConversationItemState::Streaming,
            QStringLiteral("md-1")));
        const QString raw = QStringLiteral(
            "**negrita** ![imagen](https://example.org/tracker.png) "
            "<script>mal</script> [link](javascript:alert(1))");
        model.appendDelta(QStringLiteral("md-1"), raw);
        QCOMPARE(model.data(model.index(0), TranscriptModel::StyledTextRole).toBool(), false);
        QCOMPARE(model.data(model.index(0), TranscriptModel::DisplayTextRole).toString(), raw);
        model.finish(QStringLiteral("md-1"));
        QCOMPARE(model.data(model.index(0), TranscriptModel::StyledTextRole).toBool(), true);
        const QString safe = model.data(model.index(0), TranscriptModel::DisplayTextRole).toString();
        QVERIFY(safe.contains(QStringLiteral("<b>")));
        QVERIFY(!safe.contains(QStringLiteral("<img"), Qt::CaseInsensitive));
        QVERIFY(!safe.contains(QStringLiteral("<script"), Qt::CaseInsensitive));
        QVERIFY(!safe.contains(QStringLiteral("<a "), Qt::CaseInsensitive));
        QCOMPARE(model.data(model.index(0), TranscriptModel::TextRole).toString(), raw);
    }

    void linkDestinationRemainsVisibleWithoutAnchor()
    {
        using suprai::ui::internal::TranscriptModel;
        TranscriptModel model;
        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant,
            QStringLiteral("[sitio](https://example.org/path?q=1&x=2)"),
            suprai::domain::ConversationItemState::Completed,
            QStringLiteral("md-link")));
        const QString rendered = model.data(
            model.index(0), TranscriptModel::DisplayTextRole).toString();
        QVERIFY(rendered.contains(QStringLiteral("https://example.org/path?q=1&amp;x=2")));
        QVERIFY(!rendered.contains(QStringLiteral("<a "), Qt::CaseInsensitive));
    }

    void largeAnswerFallsBackToPlainText()
    {
        using suprai::ui::internal::TranscriptModel;
        TranscriptModel model;
        const QString large(65537, QLatin1Char('x'));
        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant, large,
            suprai::domain::ConversationItemState::Completed,
            QStringLiteral("md-large")));
        QCOMPARE(model.data(model.index(0), TranscriptModel::StyledTextRole).toBool(), false);
        QCOMPARE(model.data(model.index(0), TranscriptModel::DisplayTextRole).toString(), large);
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
