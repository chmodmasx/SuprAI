#include "MarkdownBlocks.h"
#include "SafeMarkdownRenderer.h"

#include <QRegularExpression>
#include <QVariantMap>

namespace suprai::ui::internal {
namespace {

QVariantMap prose(const QString &markdown)
{
    return {
        {QStringLiteral("kind"), QStringLiteral("text")},
        {QStringLiteral("formatted"), safeMarkdownToStyledText(markdown)},
        {QStringLiteral("raw"), markdown},
    };
}

QVariantMap code(const QString &body, const QString &language)
{
    return {
        {QStringLiteral("kind"), QStringLiteral("code")},
        {QStringLiteral("raw"), body},
        {QStringLiteral("language"), language},
    };
}

QString languageName(const QString &info)
{
    const QString first = info.trimmed().section(QLatin1Char(' '), 0, 0);
    static const QRegularExpression acceptable(QStringLiteral("^[A-Za-z0-9_+#.\\-]{1,32}$"));
    return acceptable.match(first).hasMatch() ? first : QStringLiteral("Código");
}

} // namespace

QVariantList safeMarkdownBlocks(const QString &markdown)
{
    // Hard ceiling both on render cost and the number of QML delegates.
    constexpr qsizetype kMaxLength = 65536;
    constexpr qsizetype kMaxBlocks = 80;
    if (markdown.size() > kMaxLength) return {};

    const QStringList lines = markdown.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    static const QRegularExpression fence(QStringLiteral("^ {0,3}(\`{3,}|~{3,})(.*)$"));
    QVariantList blocks;
    QStringList proseLines;
    qsizetype index = 0;
    while (index < lines.size()) {
        const auto match = fence.match(lines.at(index));
        if (!match.hasMatch()) {
            proseLines.append(lines.at(index++));
            continue;
        }

        const QString opening = match.captured(1);
        const QString info = match.captured(2);
        // Locate a complete matching closing fence before consuming text.
        qsizetype close = index + 1;
        while (close < lines.size()) {
            const QString candidate = lines.at(close).trimmed();
            if (candidate.size() >= opening.size()
                && candidate.front() == opening.front()
                && candidate.startsWith(opening)) {
                // Check the closing line contains only fence characters.
                bool onlyFence = true;
                for (const QChar c : candidate) {
                    if (c != opening.front()) { onlyFence = false; break; }
                }
                if (onlyFence) break;
            }
            ++close;
        }
        if (close >= lines.size()) {
            proseLines.append(lines.at(index++));
            continue;
        }

        if (!proseLines.isEmpty()) {
            blocks.append(prose(proseLines.join(QLatin1Char('\n'))));
            proseLines.clear();
        }

        QStringList contents;
        for (qsizetype line = index + 1; line < close; ++line) {
            contents.append(lines.at(line));
        }
        blocks.append(code(contents.join(QLatin1Char('\n')), languageName(info)));
        index = close + 1;
        if (blocks.size() >= kMaxBlocks) return {};
    }
    if (!proseLines.isEmpty()) {
        blocks.append(prose(proseLines.join(QLatin1Char('\n'))));
    }
    if (blocks.isEmpty()) blocks.append(prose(markdown));
    return blocks;
}

} // namespace suprai::ui::internal
