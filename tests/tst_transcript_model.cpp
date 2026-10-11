#include "TranscriptModel.h"
#include "MarkdownBlocks.h"
#include "SafeMarkdownRenderer.h"

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

    void allowedLinkDestinationVisibleAndConfirmedInUi()
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
        QVERIFY(rendered.contains(QStringLiteral("<a href="), Qt::CaseInsensitive));
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

    void projectsFencedCodeIntoSeparateBlock()
    {
        const QString markdown = QStringLiteral(
            "Ejemplo **listo**:\n"
            "```cpp\nint main() { return 7; }\n```\n"
            "Más texto con [sitio](https://example.org/docs).");
        const auto segments = suprai::ui::internal::safeMarkdownBlocks(markdown);
        QCOMPARE(segments.size(), 3);
        QCOMPARE(segments.at(0).toMap().value("kind").toString(), QStringLiteral("text"));
        QVERIFY(segments.at(0).toMap().value("formatted").toString().contains(QStringLiteral("<b>")));
        QCOMPARE(segments.at(1).toMap().value("kind").toString(), QStringLiteral("code"));
        QCOMPARE(segments.at(1).toMap().value("language").toString(), QStringLiteral("cpp"));
        QCOMPARE(segments.at(1).toMap().value("raw").toString(), QStringLiteral("int main() { return 7; }"));
        QCOMPARE(segments.at(2).toMap().value("kind").toString(), QStringLiteral("text"));
        QVERIFY(segments.at(2).toMap().value("formatted").toString().contains(QStringLiteral("<a href=")));

        suprai::ui::internal::TranscriptModel model;
        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant, markdown,
            suprai::domain::ConversationItemState::Completed,
            QStringLiteral("code-block")));
        QCOMPARE(model.data(model.index(0),
            suprai::ui::internal::TranscriptModel::MessageBlocksRole).toList().size(), 3);
        QCOMPARE(model.data(model.index(0),
            suprai::ui::internal::TranscriptModel::TextRole).toString(), markdown);
    }

    void unsafeTargetsStayInertAndVisible()
    {
        using suprai::ui::internal::isAllowedExternalUrl;
        QVERIFY(isAllowedExternalUrl(QStringLiteral("https://example.org/a?q=1&b=2")));
        QVERIFY(isAllowedExternalUrl(QStringLiteral("http://localhost:8080/")));
        QVERIFY(!isAllowedExternalUrl(QStringLiteral("javascript:alert(1)")));
        QVERIFY(!isAllowedExternalUrl(QStringLiteral("file:///etc/passwd")));
        QVERIFY(!isAllowedExternalUrl(QStringLiteral("https://user:pass@example.org")));
        QVERIFY(!isAllowedExternalUrl(QStringLiteral("//example.org/path")));
        QVERIFY(!isAllowedExternalUrl(QStringLiteral("https://example.org/\nInjected")));

        const QString safe = suprai::ui::internal::safeMarkdownToStyledText(
            QStringLiteral("[seguro](https://example.org/a?q=1&b=2) "
                           "[malo](javascript:alert(1)) ![pixel](https://example.org/pixel.png) "
                           "<a href='https://example.net'>injection</a>"));
        QVERIFY(safe.contains(QStringLiteral("<a href=")));
        QVERIFY(!safe.contains(QStringLiteral("href=\"javascript"), Qt::CaseInsensitive));
        QVERIFY(!safe.contains(QStringLiteral("<img"), Qt::CaseInsensitive));
        QVERIFY(!safe.contains(QStringLiteral("<script"), Qt::CaseInsensitive));
        QVERIFY(safe.contains(QStringLiteral("javascript:alert")));
    }

    void incompleteFencesRemainPlainAndExcessiveMarkupFallsBack()
    {
        const auto unclosed = suprai::ui::internal::safeMarkdownBlocks(
            QStringLiteral("Primero\n```rust\nfn main() {}"));
        QCOMPARE(unclosed.size(), 1);
        QCOMPARE(unclosed.at(0).toMap().value("kind").toString(),
                 QStringLiteral("text"));
        const QString oversized(65537, QLatin1Char('x'));
        QVERIFY(suprai::ui::internal::safeMarkdownBlocks(oversized).isEmpty());
        const QString stress = QStringLiteral("```js\n1\n```\n").repeated(120);
        QVERIFY(suprai::ui::internal::safeMarkdownBlocks(stress).isEmpty());
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
