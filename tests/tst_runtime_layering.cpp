#include "AgentEngine.h"
#include "RuntimeOrchestrator.h"

#include <suprai/providers/Provider.h>

#include <QSignalSpy>
#include <QTest>
#include <QVector>

class FakeProvider final : public suprai::providers::Provider
{
public:
    using Provider::Provider;

    bool isBusy() const override
    {
        return m_busy;
    }

    suprai::providers::ProviderCapabilities capabilities(const QString &) const override
    {
        return {};
    }

    const QVector<suprai::providers::ProviderRequest> &requests() const
    {
        return m_requests;
    }

public:
    void generate(const suprai::providers::ProviderRequest &request) override
    {
        m_busy = true;
        m_requests.push_back(request);

        emit reasoningDelta(QStringLiteral("PRIVATE_REASONING"));

        const QString answer = m_requests.size() == 1
            ? QStringLiteral("respuesta uno")
            : QStringLiteral("respuesta dos");

        emit textDelta(answer);
        m_busy = false;
        emit completed();
    }

    void cancel() override
    {
        if (!m_busy) {
            return;
        }

        m_busy = false;
        emit cancelled();
    }

private:
    bool m_busy = false;
    QVector<suprai::providers::ProviderRequest> m_requests;
};

class RuntimeLayeringTest final : public QObject
{
    Q_OBJECT

private slots:
    void executionIdentitiesTrackConversationLineage()
    {
        using namespace suprai::runtime::internal;

        auto *provider = new FakeProvider;
        AgentEngine engine(provider);
        RuntimeOrchestrator orchestrator(
            {
                .model = QStringLiteral("test-model"),
                .systemPrompt = QStringLiteral("system"),
            },
            &engine);

        const QString initialSessionId = orchestrator.session().id;
        QVERIFY(initialSessionId.startsWith(QStringLiteral("session_")));

        orchestrator.start();
        orchestrator.submitPrompt(QStringLiteral("primero"));

        QCOMPARE(orchestrator.inputs().size(), 1);
        QCOMPARE(orchestrator.turns().size(), 1);
        QCOMPARE(orchestrator.runs().size(), 1);
        QCOMPARE(orchestrator.history().size(), 2);

        const auto firstInput = orchestrator.inputs().at(0);
        const auto firstTurn = orchestrator.turns().at(0);
        const auto firstRun = orchestrator.runs().at(0);

        QCOMPARE(firstInput.sessionId, initialSessionId);
        QCOMPARE(firstTurn.sessionId, initialSessionId);
        QCOMPARE(firstTurn.inputId, firstInput.id);
        QVERIFY(firstTurn.parentTurnId.isEmpty());
        QCOMPARE(firstRun.turnId, firstTurn.id);
        QCOMPARE(firstRun.generation, 1);
        QCOMPARE(orchestrator.history().at(0).turnId, firstTurn.id);
        QCOMPARE(orchestrator.history().at(1).turnId, firstTurn.id);

        orchestrator.submitPrompt(QStringLiteral("segundo"));

        QCOMPARE(orchestrator.inputs().size(), 2);
        QCOMPARE(orchestrator.turns().size(), 2);
        QCOMPARE(orchestrator.runs().size(), 2);
        QCOMPARE(orchestrator.history().size(), 4);

        const auto secondTurn = orchestrator.turns().at(1);
        QCOMPARE(secondTurn.parentTurnId, firstTurn.id);
        QCOMPARE(orchestrator.history().at(2).turnId, secondTurn.id);
        QCOMPARE(orchestrator.history().at(3).turnId, secondTurn.id);

        orchestrator.resetSession();

        QVERIFY(orchestrator.session().id != initialSessionId);
        QCOMPARE(orchestrator.inputs().size(), 0);
        QCOMPARE(orchestrator.turns().size(), 0);
        QCOMPARE(orchestrator.runs().size(), 0);
        QCOMPARE(orchestrator.history().size(), 0);
    }

    void orchestratorOwnsHistoryWhileEngineOwnsExecution()
    {
        using namespace suprai::runtime::internal;

        auto *provider = new FakeProvider;
        AgentEngine engine(provider);
        RuntimeOrchestrator orchestrator(
            {
                .model = QStringLiteral("test-model"),
                .systemPrompt = QStringLiteral("system"),
            },
            &engine);

        QSignalSpy completed(&orchestrator, &RuntimeOrchestrator::assistantMessageCompleted);
        QSignalSpy reasoning(&orchestrator, &RuntimeOrchestrator::reasoningActiveChanged);

        orchestrator.start();
        orchestrator.submitPrompt(QStringLiteral("primero"));

        QCOMPARE(completed.size(), 1);
        QCOMPARE(provider->requests().size(), 1);
        QCOMPARE(reasoning.size(), 2);
        QCOMPARE(reasoning.at(0).at(0).toBool(), true);
        QCOMPARE(reasoning.at(1).at(0).toBool(), false);

        orchestrator.submitPrompt(QStringLiteral("segundo"));

        QCOMPARE(completed.size(), 2);
        QCOMPARE(provider->requests().size(), 2);

        const auto &secondRequest = provider->requests().at(1);

        bool hasFirstUser = false;
        bool hasFirstAnswer = false;
        bool hasSecondUser = false;
        bool hasPrivateReasoning = false;

        for (const auto &message : secondRequest.messages) {
            hasFirstUser |= message.content.contains(QStringLiteral("primero"));
            hasFirstAnswer |= message.content.contains(QStringLiteral("respuesta uno"));
            hasSecondUser |= message.content.contains(QStringLiteral("segundo"));
            hasPrivateReasoning |= message.content.contains(QStringLiteral("PRIVATE_REASONING"));
        }

        QVERIFY(hasFirstUser);
        QVERIFY(hasFirstAnswer);
        QVERIFY(hasSecondUser);
        QVERIFY2(!hasPrivateReasoning,
                 "AgentEngine reasoning events must not become RuntimeOrchestrator canonical history.");
    }
};

QTEST_MAIN(RuntimeLayeringTest)
#include "tst_runtime_layering.moc"
