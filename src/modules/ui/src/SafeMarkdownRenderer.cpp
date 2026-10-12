#include "SafeMarkdownRenderer.h"

#include <QFont>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextList>
#include <QTextListFormat>
#include <QUrl>

namespace suprai::ui::internal {

bool isAllowedExternalUrl(const QString &uri)
{
    if (uri.isEmpty() || uri.size() > 2048) return false;
    for (const QChar ch : uri) {
        if (ch.isSpace() || ch.isNull() || ch.category() == QChar::Other_Control) {
            return false;
        }
    }
    const QUrl url(uri, QUrl::StrictMode);
    return url.isValid() && !url.host().isEmpty() && url.userInfo().isEmpty()
        && (url.scheme() == QStringLiteral("https")
            || url.scheme() == QStringLiteral("http"));
}

QString safeMarkdownToStyledText(const QString &markdown)
{
    // Read-only Markdown AST projection; no layout or image-resource fetch.
    QTextDocument doc;
    doc.setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub
                                  | QTextDocument::MarkdownNoHTML);

    QString result;
    bool first = true;
    for (QTextBlock block = doc.begin(); block.isValid(); block = block.next()) {
        if (!first) result += QStringLiteral("<br/>");
        first = false;

        if (const auto *list = block.textList()) {
            switch (block.blockFormat().marker()) {
            case QTextBlockFormat::MarkerType::Checked:
                result += QStringLiteral("☑ ");
                break;
            case QTextBlockFormat::MarkerType::Unchecked:
                result += QStringLiteral("☐ ");
                break;
            case QTextBlockFormat::MarkerType::NoMarker:
                if (list->format().style() == QTextListFormat::ListDecimal) {
                    result += QString::number(list->itemNumber(block) + 1);
                    result += QStringLiteral(". ");
                } else {
                    result += QStringLiteral("• ");
                }
                break;
            }
        }

        QString previousHref;
        // All markup here is generated from fixed constants, never from
        // untrusted source HTML. Sizes are intentionally compact in chat.
        const int headingLevel = block.blockFormat().headingLevel();
        constexpr int headingSizes[] = {0, 26, 23, 20, 18, 16, 14};
        if (headingLevel >= 1 && headingLevel <= 6) {
            result += QStringLiteral("<span style=\"font-size:%1px;font-weight:bold\">")
                          .arg(headingSizes[headingLevel]);
        }

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
            const bool strike = format.fontStrikeOut();
            const bool code = format.fontFixedPitch();
            if (bold) result += QStringLiteral("<b>");
            if (italic) result += QStringLiteral("<i>");
            if (strike) result += QStringLiteral("<s>");
            if (code) result += QStringLiteral("<font face=\"monospace\">");

            // Only generated, strictly validated HTTP(S) anchors are active.
            // No model HTML or image URLs reach the QML text surface.
            const QString href = format.isAnchor() ? format.anchorHref() : QString();
            if (isAllowedExternalUrl(href)) {
                result += QStringLiteral("<a href=\"");
                result += href.toHtmlEscaped();
                result += QStringLiteral("\">");
                result += text;
                result += QStringLiteral("</a>");
            } else {
                result += text;
            }
            // Make the destination visible even when the label differs.
            if (!href.isEmpty() && href != fragment.text() && href != previousHref) {
                result += QStringLiteral(" (");
                result += href.toHtmlEscaped();
                result += QStringLiteral(")");
            }
            previousHref = href;

            if (code) result += QStringLiteral("</font>");
            if (strike) result += QStringLiteral("</s>");
            if (italic) result += QStringLiteral("</i>");
            if (bold) result += QStringLiteral("</b>");
        }
        if (headingLevel >= 1 && headingLevel <= 6) {
            result += QStringLiteral("</span>");
        }
    }
    return result;
}

} // namespace suprai::ui::internal
