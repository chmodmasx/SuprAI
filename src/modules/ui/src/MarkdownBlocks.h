#pragma once

#include <QString>
#include <QVariantList>

namespace suprai::ui::internal {

// Fenced code is represented structurally rather than through QTextDocument's
// resource-bearing rich text. Completed assistant messages only, <= 64Ki UTF-16.
QVariantList safeMarkdownBlocks(const QString &markdown);

} // namespace suprai::ui::internal
