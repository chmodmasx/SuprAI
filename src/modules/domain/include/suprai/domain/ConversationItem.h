#pragma once

#include <suprai/domain/Identifiers.h>

#include <QJsonObject>
#include <QString>

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace suprai::domain {

enum class ConversationItemKind {
    Message,
    ReasoningSummary,
    ToolCall,
    ToolResult,
    Attachment,
    RuntimeAnnotation
};

enum class ConversationItemState {
    Pending,
    Streaming,
    Completed,
    Failed,
    Cancelled
};

enum class ConversationRole {
    User,
    Assistant,
    System,
    Tool,
    Runtime
};

enum class ToolResultStatus {
    Success,
    Error,
    Denied,
    Cancelled,
    SkippedBySteering,
    OutcomeUnknown
};

struct MessageContent {
    ConversationRole role = ConversationRole::Assistant;
    QString text;
};

struct ReasoningSummaryContent {
    QString summary;
};

struct ToolCallContent {
    QString toolInvocationId;
    QString name;
    QJsonObject arguments;
};

struct ToolResultContent {
    QString toolInvocationId;
    ToolResultStatus status = ToolResultStatus::Success;
    QString text;
};

struct AttachmentContent {
    QString attachmentId;
    QString displayName;
    QString mimeType;
};

struct RuntimeAnnotationContent {
    QString code;
    QString text;
};

using ConversationItemContent = std::variant<
    MessageContent,
    ReasoningSummaryContent,
    ToolCallContent,
    ToolResultContent,
    AttachmentContent,
    RuntimeAnnotationContent>;

struct ConversationItem {
    QString id;
    QString turnId;
    int sequence = 0;
    ConversationItemState state = ConversationItemState::Completed;
    ConversationItemContent content = MessageContent{};
};

inline ConversationItemKind itemKind(const ConversationItem &item)
{
    return std::visit(
        [](const auto &content) -> ConversationItemKind {
            using T = std::decay_t<decltype(content)>;

            if constexpr (std::is_same_v<T, MessageContent>) {
                return ConversationItemKind::Message;
            } else if constexpr (std::is_same_v<T, ReasoningSummaryContent>) {
                return ConversationItemKind::ReasoningSummary;
            } else if constexpr (std::is_same_v<T, ToolCallContent>) {
                return ConversationItemKind::ToolCall;
            } else if constexpr (std::is_same_v<T, ToolResultContent>) {
                return ConversationItemKind::ToolResult;
            } else if constexpr (std::is_same_v<T, AttachmentContent>) {
                return ConversationItemKind::Attachment;
            } else {
                return ConversationItemKind::RuntimeAnnotation;
            }
        },
        item.content);
}

inline const MessageContent *messageContent(const ConversationItem &item)
{
    return std::get_if<MessageContent>(&item.content);
}

inline MessageContent *messageContent(ConversationItem &item)
{
    return std::get_if<MessageContent>(&item.content);
}

inline bool isStreaming(const ConversationItem &item)
{
    return item.state == ConversationItemState::Streaming;
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
    case ConversationRole::Tool:
        return QStringLiteral("tool");
    case ConversationRole::Runtime:
        return QStringLiteral("runtime");
    }
    return QStringLiteral("runtime");
}

inline std::optional<ConversationRole> itemRole(const ConversationItem &item)
{
    if (const auto *message = messageContent(item)) {
        return message->role;
    }
    return std::nullopt;
}

inline ConversationItem makeMessageItem(
    ConversationRole role,
    QString text,
    ConversationItemState state = ConversationItemState::Completed,
    QString id = newItemId(),
    QString turnId = {},
    int sequence = 0)
{
    return {
        .id = std::move(id),
        .turnId = std::move(turnId),
        .sequence = sequence,
        .state = state,
        .content = MessageContent{
            .role = role,
            .text = std::move(text),
        },
    };
}

} // namespace suprai::domain
