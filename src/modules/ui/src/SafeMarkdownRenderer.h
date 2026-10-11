#pragma once

#include <QString>

namespace suprai::ui::internal {

// Input must be an absolute HTTP(S) URL without embedded credentials. External
// activation is always user-initiated and confirmation-gated in the QML UI.
bool isAllowedExternalUrl(const QString &uri);

// Emits ONLY generated/escaped Qt StyledText tags. External resources are not
// fetched. Allowed HTTP(S) links emit an anchor, but never activate themselves.
QString safeMarkdownToStyledText(const QString &markdown);

} // namespace suprai::ui::internal
