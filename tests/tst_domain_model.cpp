#include <suprai/domain/ConversationItem.h>
#include <suprai/domain/ExecutionModel.h>
#include <suprai/domain/Identifiers.h>

#include <QTest>

class DomainModelTest final : public QObject
{
    Q_OBJECT

private slots:
    void entityIdsCarrySemanticPrefixes()
    {
        QVERIFY(suprai::domain::newSessionId().startsWith(QStringLiteral("session_")));
        QVERIFY(suprai::domain::newInputId().startsWith(QStringLiteral("input_")));
        QVERIFY(suprai::domain::newTurnId().startsWith(QStringLiteral("turn_")));
        QVERIFY(suprai::domain::newRunId().startsWith(QStringLiteral("run_")));
        QVERIFY(suprai::domain::newItemId().startsWith(QStringLiteral("item_")));
        QVERIFY(suprai::domain::newToolInvocationId().startsWith(QStringLiteral("tool_")));
        QVERIFY(suprai::domain::newAttachmentId().startsWith(QStringLiteral("attachment_")));
    }

    void conversationKindsAreDerivedFromTypedContent()
    {
        using namespace suprai::domain;

        const ConversationItem message = makeMessageItem(
            ConversationRole::User,
            QStringLiteral("hola"));

        const ConversationItem reasoning{
            .id = newItemId(),
            .state = ConversationItemState::Completed,
            .content = ReasoningSummaryContent{
                .summary = QStringLiteral("resultado resumido"),
            },
        };

        const ConversationItem toolCall{
            .id = newItemId(),
            .state = ConversationItemState::Pending,
            .content = ToolCallContent{
                .toolInvocationId = newToolInvocationId(),
                .name = QStringLiteral("read_file"),
                .arguments = {},
            },
        };

        QCOMPARE(itemKind(message), ConversationItemKind::Message);
        QCOMPARE(itemKind(reasoning), ConversationItemKind::ReasoningSummary);
        QCOMPARE(itemKind(toolCall), ConversationItemKind::ToolCall);

        QVERIFY(messageContent(message) != nullptr);
        QVERIFY(messageContent(reasoning) == nullptr);
        QCOMPARE(itemRole(message).value(), ConversationRole::User);
        QVERIFY(!itemRole(toolCall).has_value());
    }

    void executionIdentityRelationsAreExplicit()
    {
        using namespace suprai::domain;

        const Session session{
            .id = newSessionId(),
            .parentSessionId = {},
        };

        const Input input{
            .id = newInputId(),
            .sessionId = session.id,
            .text = QStringLiteral("hacé algo"),
        };

        const Turn turn{
            .id = newTurnId(),
            .sessionId = session.id,
            .inputId = input.id,
            .parentTurnId = {},
        };

        const Run run{
            .id = newRunId(),
            .turnId = turn.id,
            .generation = 1,
        };

        QCOMPARE(input.sessionId, session.id);
        QCOMPARE(turn.sessionId, session.id);
        QCOMPARE(turn.inputId, input.id);
        QCOMPARE(run.turnId, turn.id);
        QCOMPARE(run.generation, 1);
    }
};

QTEST_MAIN(DomainModelTest)
#include "tst_domain_model.moc"
