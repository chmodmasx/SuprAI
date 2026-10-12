#include "MarkdownStructures.h"
#include "SafeMarkdownRenderer.h"

#include <QRegularExpression>
#include <QVariantMap>

namespace suprai::ui::internal {
namespace {

constexpr qsizetype kMaxColumns = 10;
constexpr qsizetype kMaxRows = 60;

QVariantMap prose(const QString &raw)
{
    return {{QStringLiteral("kind"), QStringLiteral("text")},
            {QStringLiteral("raw"), raw},
            {QStringLiteral("formatted"), safeMarkdownToStyledText(raw)}};
}

// Pipes escaped with a backslash, or inside an inline code span, are not
// cell delimiters. Preserve escapes for the safe Markdown parser.
QStringList splitCells(const QString &line)
{
    const QString input = line.trimmed();
    QStringList cells;
    QString current;
    QList<qsizetype> pipePositions;
    qsizetype codeTicks = 0;

    for (qsizetype i = 0; i < input.size(); ++i) {
        const QChar ch = input.at(i);
        if (ch == QLatin1Char('\\') && i + 1 < input.size()
            && input.at(i + 1) == QLatin1Char('|')) {
            current += ch;
            current += input.at(++i);
            continue;
        }
        if (ch == QChar(0x60)) {
            qsizetype count = 1;
            while (i + count < input.size()
                   && input.at(i + count) == QChar(0x60)) ++count;
            if (codeTicks == 0) codeTicks = count;
            else if (codeTicks == count) codeTicks = 0;
            current += input.mid(i, count);
            i += count - 1;
            continue;
        }
        if (ch == QLatin1Char('|') && codeTicks == 0) {
            pipePositions.append(i);
            cells.append(current.trimmed());
            current.clear();
            continue;
        }
        current += ch;
    }
    if (pipePositions.isEmpty()) return {};
    cells.append(current.trimmed());
    if (pipePositions.first() == 0) cells.removeFirst();
    if (pipePositions.last() == input.size() - 1) cells.removeLast();
    return cells;
}

bool parseTable(const QStringList &lines, qsizetype first,
                QVariantMap &result, qsizetype &after)
{
    if (first + 1 >= lines.size()) return false;
    const QStringList headers = splitCells(lines.at(first));
    const QStringList separators = splitCells(lines.at(first + 1));
    if (headers.size() < 2 || headers.size() > kMaxColumns
        || separators.size() != headers.size()) return false;

    static const QRegularExpression rule(QStringLiteral("^:?-{3,}:?$"));
    QVariantList alignment;
    for (const QString &separator : separators) {
        if (!rule.match(separator).hasMatch()) return false;
        const bool left = separator.startsWith(QLatin1Char(':'));
        const bool right = separator.endsWith(QLatin1Char(':'));
        alignment.append(left && right ? QStringLiteral("center")
                        : right ? QStringLiteral("right") : QStringLiteral("left"));
    }

    QVariantList headerCells;
    for (const QString &raw : headers) {
        headerCells.append(QVariantMap{{QStringLiteral("raw"), raw},
            {QStringLiteral("formatted"), safeMarkdownToStyledText(raw)}});
    }

    QVariantList rows;
    QStringList rawLines{lines.at(first), lines.at(first + 1)};
    qsizetype i = first + 2;
    for (; i < lines.size(); ++i) {
        if (lines.at(i).trimmed().isEmpty()) break;
        const QStringList values = splitCells(lines.at(i));
        if (values.isEmpty()) break;
        if (rows.size() >= kMaxRows || values.size() > headers.size()) {
            return false; // fall back to source text, never silently truncate
        }
        QVariantList row;
        for (qsizetype col = 0; col < headers.size(); ++col) {
            const QString value = col < values.size() ? values.at(col) : QString();
            row.append(QVariantMap{{QStringLiteral("raw"), value},
                {QStringLiteral("formatted"), safeMarkdownToStyledText(value)}});
        }
        // QVariantList::append(QVariantList) concatenates cells; wrap the
        // row explicitly so QML receives a nested list of table rows.
        rows.append(QVariant::fromValue(row));
        rawLines.append(lines.at(i));
    }
    result = {{QStringLiteral("kind"), QStringLiteral("table")},
              {QStringLiteral("raw"), rawLines.join(QLatin1Char('\n'))},
              {QStringLiteral("headers"), headerCells},
              {QStringLiteral("rows"), rows},
              {QStringLiteral("alignment"), alignment}};
    after = i;
    return true;
}

} // namespace

void appendMarkdownStructures(const QStringList &lines, QVariantList &blocks)
{
    static const QRegularExpression quoteLine(QStringLiteral("^ {0,3}> ?(.*)$"));
    QStringList proseLines;
    const auto flush = [&]() {
        while (!proseLines.isEmpty() && proseLines.first().trimmed().isEmpty())
            proseLines.removeFirst();
        while (!proseLines.isEmpty() && proseLines.last().trimmed().isEmpty())
            proseLines.removeLast();
        if (!proseLines.isEmpty()) {
            blocks.append(prose(proseLines.join(QLatin1Char('\n'))));
            proseLines.clear();
        }
    };

    for (qsizetype i = 0; i < lines.size();) {
        QVariantMap table;
        qsizetype after = i;
        if (parseTable(lines, i, table, after)) {
            flush();
            blocks.append(table);
            i = after;
            continue;
        }
        const auto quote = quoteLine.match(lines.at(i));
        if (quote.hasMatch()) {
            flush();
            QStringList quoted, original;
            do {
                quoted.append(quoteLine.match(lines.at(i)).captured(1));
                original.append(lines.at(i));
                ++i;
            } while (i < lines.size() && quoteLine.match(lines.at(i)).hasMatch());
            blocks.append(QVariantMap{
                {QStringLiteral("kind"), QStringLiteral("quote")},
                {QStringLiteral("raw"), original.join(QLatin1Char('\n'))},
                {QStringLiteral("formatted"),
                    safeMarkdownToStyledText(quoted.join(QLatin1Char('\n')))}
            });
            continue;
        }
        proseLines.append(lines.at(i++));
    }
    flush();
}

} // namespace suprai::ui::internal
