#pragma once

#include <QString>

namespace suprai::ui::internal {

// Returns only generated, allowlisted Qt StyledText tags. No model-supplied
// HTML, anchors, image tags or resource URLs survive this boundary.
QString safeMarkdownToStyledText(const QString &markdown);

} // namespace suprai::ui::internal
