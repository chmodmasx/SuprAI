#pragma once

#include <QString>
#include <QStringView>
#include <QUuid>

namespace suprai::domain {

inline QString newEntityId(QStringView prefix)
{
    return prefix.toString()
        + QLatin1Char('_')
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

inline QString newSessionId()
{
    return newEntityId(u"session");
}

inline QString newInputId()
{
    return newEntityId(u"input");
}

inline QString newTurnId()
{
    return newEntityId(u"turn");
}

inline QString newRunId()
{
    return newEntityId(u"run");
}

inline QString newItemId()
{
    return newEntityId(u"item");
}

inline QString newToolInvocationId()
{
    return newEntityId(u"tool");
}

inline QString newAttachmentId()
{
    return newEntityId(u"attachment");
}

} // namespace suprai::domain
