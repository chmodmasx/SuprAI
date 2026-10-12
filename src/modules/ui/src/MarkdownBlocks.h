#pragma once

#include <QString>
#include <QVariantList>

namespace suprai::ui::internal {

// Fenced code is represented structurally rather than through resource-bearing
// rich text. Both parsers cap input at 65,536 UTF-16 units and 80 blocks.
QVariantList safeMarkdownBlocks(const QString &markdown);
// During streaming, an open fence is projected as a temporary code block.
// The final version always uses the strict completed-message parser.
QVariantList safeStreamingMarkdownBlocks(const QString &markdown);

} // namespace suprai::ui::internal
