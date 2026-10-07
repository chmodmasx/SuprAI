#pragma once

#include <QString>
#include <QUuid>

namespace suprai::domain {

enum class ConversationRole {
    User,
    Assistant,
    System
};

struct ConversationItem {
    QString id;
    ConversationRole role = ConversationRole::Assistant;
    QString text;
    bool streaming = false;
};

inline QString newItemId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

inline QString roleName(ConversationRole role)
{
    switch (role) {
    case ConversationRole::User:
        return QStringLiteral("user");
    case ConversationRole::Assistant:
        return QStringLiteral("assistant");
    case ConversationRole::System:
        return QStringLiteral("system");
    }
    return QStringLiteral("assistant");
}

} // namespace suprai::domain
