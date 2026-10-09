#include "SafeMarkdownRenderer.h"

#include <QFont>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextList>
#include <QTextListFormat>

namespace suprai::ui::internal {

QString safeMarkdownToStyledText(const QString &markdown)
{
    // Read-only Markdown AST projection; no layout or image-resource fetch.
    QTextDocument doc;
    doc.setMarkdown(markdown, QTextDocument::MarkdownNoHTML);

    QString result;
    bool first = true;
    for (QTextBlock block = doc.begin(); block.isValid(); block = block.next()) {
        if (!first) result += QStringLiteral("<br/>");
        first = false;

        if (const auto *list = block.textList()) {
            if (list->format().style() == QTextListFormat::ListDecimal) {
                result += QString::number(list->itemNumber(block) + 1);
                result += QStringLiteral(". ");
            } else {
                result += QStringLiteral("• ");
            }
        }

        QString previousHref;
        const bool heading = block.blockFormat().headingLevel() > 0;
        if (heading) result += QStringLiteral("<b>");

        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid()) continue;
            const QTextCharFormat format = fragment.charFormat();
            if (format.isImageFormat()) {
                result += QStringLiteral("[imagen omitida]");
                continue;
            }

            QString text = fragment.text();
            text.replace(QChar::ObjectReplacementCharacter,
                         QStringLiteral("[imagen omitida]"));
            text = text.toHtmlEscaped();

            const bool bold = format.fontWeight() >= QFont::Bold;
            const bool italic = format.fontItalic();
            const bool code = format.fontFixedPitch();
            if (bold) result += QStringLiteral("<b>");
            if (italic) result += QStringLiteral("<i>");
            if (code) result += QStringLiteral("<font face=\"monospace\">");

            // Links are inert, but their targets stay visible and copyable.
            result += text;
            const QString href = format.isAnchor() ? format.anchorHref() : QString();
            if (!href.isEmpty() && href != fragment.text() && href != previousHref) {
                result += QStringLiteral(" (");
                result += href.toHtmlEscaped();
                result += QStringLiteral(")");
            }
            previousHref = href;

            if (code) result += QStringLiteral("</font>");
            if (italic) result += QStringLiteral("</i>");
            if (bold) result += QStringLiteral("</b>");
        }
        if (heading) result += QStringLiteral("</b>");
    }
    return result;
}

} // namespace suprai::ui::internal
