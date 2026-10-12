#pragma once

#include <QStringList>
#include <QVariantList>

namespace suprai::ui::internal {

// Converts only bounded prose slices to safe structural UI segments.
// Does not run resource loaders or produce unsanitized HTML.
void appendMarkdownStructures(const QStringList &lines, QVariantList &blocks);

} // namespace suprai::ui::internal
