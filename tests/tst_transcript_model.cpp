#include "TranscriptModel.h"
#include "MarkdownBlocks.h"
#include "SafeMarkdownRenderer.h"

#include <suprai/domain/ConversationItem.h>

#include <QSignalSpy>
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

    void streamsSafeMarkdownOnCoalescedUiUpdates()
    {
        using suprai::ui::internal::TranscriptModel;
        TranscriptModel model;
        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant, {},
            suprai::domain::ConversationItemState::Streaming,
            QStringLiteral("preview")));
        QSignalSpy changes(&model, &QAbstractItemModel::dataChanged);
        model.appendDelta(QStringLiteral("preview"), QStringLiteral("# Título\n"));
        model.appendDelta(QStringLiteral("preview"), QStringLiteral("**negrita** "));
        model.appendDelta(QStringLiteral("preview"), QStringLiteral("![pixel](https://example.org/x.png)"));
        QCOMPARE(changes.count(), 0); // no synchronous per-token reparse or UI update
        QCOMPARE(model.data(model.index(0), TranscriptModel::TextRole).toString(),
                 QStringLiteral("# Título\n**negrita** ![pixel](https://example.org/x.png)"));
        QTRY_COMPARE_WITH_TIMEOUT(changes.count(), 1, 1500);
        const auto blocks = model.data(model.index(0), TranscriptModel::MessageBlocksRole).toList();
        QCOMPARE(blocks.size(), 1);
        const QString preview = blocks.first().toMap().value("formatted").toString();
        QVERIFY(preview.contains(QStringLiteral("font-size:26px")));
        QVERIFY(preview.contains(QStringLiteral("<b>negrita</b>")));
        QVERIFY(!preview.contains(QStringLiteral("<img"), Qt::CaseInsensitive));
        QVERIFY(!preview.contains(QStringLiteral("<script"), Qt::CaseInsensitive));
        QVERIFY(model.data(model.index(0), TranscriptModel::StreamingRole).toBool());

        model.finish(QStringLiteral("preview"));
        QCOMPARE(model.data(model.index(0), TranscriptModel::StreamingRole).toBool(), false);
        QVERIFY(!model.data(model.index(0), TranscriptModel::MessageBlocksRole).toList().isEmpty());
    }

    void openFenceIsShownAsCodeDuringStreaming()
    {
        using suprai::ui::internal::TranscriptModel;
        const QString unfinished = QStringLiteral("Antes **listo**\n```cpp\nint n = 2;");
        const auto completed = suprai::ui::internal::safeMarkdownBlocks(unfinished);
        QCOMPARE(completed.size(), 1); // strict final projection unchanged
        const auto preview = suprai::ui::internal::safeStreamingMarkdownBlocks(unfinished);
        QCOMPARE(preview.size(), 2);
        QCOMPARE(preview.at(0).toMap().value("kind").toString(), QStringLiteral("text"));
        QCOMPARE(preview.at(1).toMap().value("kind").toString(), QStringLiteral("code"));
        QCOMPARE(preview.at(1).toMap().value("language").toString(), QStringLiteral("cpp"));
        QCOMPARE(preview.at(1).toMap().value("raw").toString(), QStringLiteral("int n = 2;"));

        TranscriptModel model;
        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant, {},
            suprai::domain::ConversationItemState::Streaming, QStringLiteral("fence")));
        model.appendDelta(QStringLiteral("fence"), unfinished);
        QTRY_COMPARE_WITH_TIMEOUT(model.data(model.index(0),
            TranscriptModel::MessageBlocksRole).toList().size(), 2, 1500);
    }

    void stoppedPreviewReturnsToPlainTextAndCannotResurrect()
    {
        using suprai::ui::internal::TranscriptModel;
        TranscriptModel model;
        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant, {},
            suprai::domain::ConversationItemState::Streaming, QStringLiteral("stopped")));
        model.appendDelta(QStringLiteral("stopped"), QStringLiteral("**parcial**"));
        QTRY_VERIFY_WITH_TIMEOUT(!model.data(model.index(0),
            TranscriptModel::MessageBlocksRole).toList().isEmpty(), 1500);
        model.stop(QStringLiteral("stopped"), suprai::domain::ConversationItemState::Failed);
        QCOMPARE(model.data(model.index(0), TranscriptModel::MessageBlocksRole).toList().size(), 0);
        QCOMPARE(model.data(model.index(0), TranscriptModel::DisplayTextRole).toString(),
                 QStringLiteral("**parcial**"));
        QCOMPARE(model.data(model.index(0), TranscriptModel::StateRole).toString(),
                 QStringLiteral("failed"));
        model.appendDelta(QStringLiteral("stopped"), QStringLiteral("rejected"));
        QCOMPARE(model.data(model.index(0), TranscriptModel::TextRole).toString(),
                 QStringLiteral("**parcial**"));
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

    void allowedLinkDestinationVisibleAndClickable()
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


    void separatesTablesQuotesAndCodeWithoutChangingRawHistory()
    {
        const QString markdown = QStringLiteral(
            "Texto previo\n"
            "| Modelo | Velocidad |\n"
            "|:---|---:|\n"
            "| A | 50 t/s |\n"
            "| B | **80** t/s |\n"
            "\n> Una **cita** importante\n"
            "> con otra línea\n"
            "\n```cpp\nint x = 42;\n```\n");
        const auto blocks = suprai::ui::internal::safeMarkdownBlocks(markdown);
        QCOMPARE(blocks.size(), 4);
        QCOMPARE(blocks.at(0).toMap().value("kind").toString(), QStringLiteral("text"));
        const auto table = blocks.at(1).toMap();
        QCOMPARE(table.value("kind").toString(), QStringLiteral("table"));
        QCOMPARE(table.value("headers").toList().size(), 2);
        QCOMPARE(table.value("rows").toList().size(), 2);
        QCOMPARE(table.value("alignment").toList().at(1).toString(), QStringLiteral("right"));
        const auto lastRow = table.value("rows").toList().at(1).toList();
        QVERIFY(lastRow.at(1).toMap().value("formatted").toString()
            .contains(QStringLiteral("<b>80</b>")));
        QCOMPARE(blocks.at(2).toMap().value("kind").toString(), QStringLiteral("quote"));
        QVERIFY(blocks.at(2).toMap().value("formatted").toString()
            .contains(QStringLiteral("<b>cita</b>")));
        QCOMPARE(blocks.at(3).toMap().value("kind").toString(), QStringLiteral("code"));

        suprai::ui::internal::TranscriptModel model;
        model.append(suprai::domain::makeMessageItem(
            suprai::domain::ConversationRole::Assistant,
            markdown, suprai::domain::ConversationItemState::Completed,
            QStringLiteral("structured")));
        QCOMPARE(model.data(model.index(0),
            suprai::ui::internal::TranscriptModel::TextRole).toString(), markdown);
    }

    void keepsEscapedPipesAndInlineCodeInsideCells()
    {
        const QString tick(QChar(0x60));
        const QString input = QStringLiteral(
            "| Izquierda | Derecha |\n"
            "|---|---|\n"
            "| uno \\| dos | ") + tick + QStringLiteral("x|y") + tick
            + QStringLiteral(" |\n");
        const auto blocks = suprai::ui::internal::safeMarkdownBlocks(input);
        QCOMPARE(blocks.size(), 1);
        const auto table = blocks.at(0).toMap();
        QCOMPARE(table.value("kind").toString(), QStringLiteral("table"));
        const auto row = table.value("rows").toList().at(0).toList();
        QCOMPARE(row.size(), 2);
        QCOMPARE(row.at(0).toMap().value("raw").toString(),
                 QStringLiteral("uno \\| dos"));
        QCOMPARE(row.at(1).toMap().value("raw").toString(), tick + QStringLiteral("x|y") + tick);
    }

    void unsafeTableAndQuoteContentNeverBecomesResourceMarkup()
    {
        const QString source = QStringLiteral(
            "| Acción | Vista |\n"
            "|---|---|\n"
            "| [malo](javascript:alert(1)) | ![pixel](https://example.org/t.png) |\n"
            "\n> <script>mal</script> [seguro](https://example.org/docs)");
        const auto blocks = suprai::ui::internal::safeStreamingMarkdownBlocks(source);
        QCOMPARE(blocks.size(), 2);
        const auto table = blocks.at(0).toMap();
        const auto row = table.value("rows").toList().at(0).toList();
        for (const QVariant &cell : row) {
            const QString formatted = cell.toMap().value("formatted").toString();
            QVERIFY(!formatted.contains(QStringLiteral("<img"), Qt::CaseInsensitive));
            QVERIFY(!formatted.contains(QStringLiteral("<script"), Qt::CaseInsensitive));
            QVERIFY(!formatted.contains(QStringLiteral("href=\"javascript"), Qt::CaseInsensitive));
        }
        const QString quote = blocks.at(1).toMap().value("formatted").toString();
        QVERIFY(!quote.contains(QStringLiteral("<script"), Qt::CaseInsensitive));
        QVERIFY(quote.contains(QStringLiteral("<a href=\"https://example.org/docs\"")));
    }

    void rejectsMalformedAndOversizedTablesWithoutDroppingSource()
    {
        const QString bad = QStringLiteral("A | B\n---|not-a-rule\none|two");
        const auto badBlocks = suprai::ui::internal::safeMarkdownBlocks(bad);
        QCOMPARE(badBlocks.size(), 1);
        QCOMPARE(badBlocks.first().toMap().value("kind").toString(),
                 QStringLiteral("text"));
        const QString large = QStringLiteral("| A | B |\n|---|---|\n")
                              + QStringLiteral("| x | y |\n").repeated(61);
        const auto result = suprai::ui::internal::safeMarkdownBlocks(large);
        QVERIFY(!result.isEmpty());
        QCOMPARE(result.at(0).toMap().value("kind").toString(), QStringLiteral("text"));
        QVERIFY(result.at(0).toMap().value("raw").toString().contains(QStringLiteral("| x | y |")));
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

    void projectsHeadingHierarchyStrikeAndTaskMarkers()
    {
        const QString raw = QStringLiteral(
            "# Nivel 1\n## Nivel 2\n### Nivel 3\n"
            "~~obsoleto~~\n- [x] terminado\n- [ ] pendiente");
        const QString safe = suprai::ui::internal::safeMarkdownToStyledText(raw);
        QVERIFY(safe.contains(QStringLiteral("font-size:26px")));
        QVERIFY(safe.contains(QStringLiteral("font-size:23px")));
        QVERIFY(safe.contains(QStringLiteral("font-size:20px")));
        QVERIFY(safe.contains(QStringLiteral("<s>obsoleto</s>")));
        QVERIFY(safe.contains(QStringLiteral("☑ terminado")));
        QVERIFY(safe.contains(QStringLiteral("☐ pendiente")));
        QVERIFY(!safe.contains(QStringLiteral("<script")));
        QVERIFY(!safe.contains(QStringLiteral("<img")));
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
