#include "AgentEngine.h"
#include "RuntimeOrchestrator.h"

#include <suprai/persistence/PersistencePort.h>
#include <suprai/providers/Provider.h>

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
        if (failNext) {
            m_busy = false;
            emit failed(QStringLiteral("provider-error"));
            return;
        }

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

public:
    bool failNext = false;

private:
    bool m_busy = false;
    QVector<suprai::providers::ProviderRequest> m_requests;
};

class FakePersistencePort final : public suprai::persistence::PersistencePort
{
public:
    using PersistencePort::PersistencePort;

    void persistTurnStart(suprai::persistence::TurnStartWrite request) override
    {
        ++writeCount;
        lastWrite = std::move(request);
    }

    void loadLatestSession() override
    {
        ++loadCount;
        if (!deferLoad) emit latestSessionLoaded(snapshot);
    }

    void loaded()
    {
        emit latestSessionLoaded(snapshot);
    }

    void persistTurnTerminal(suprai::persistence::TurnTerminalWrite request) override
    {
        ++terminalWriteCount;
        lastTerminal = std::move(request);
    }

    void succeed()
    {
        emit turnStartPersisted(lastWrite.requestId);
    }

    void terminalSucceeded()
    {
        emit turnTerminalPersisted(lastTerminal.requestId);
    }

    void terminalFailed()
    {
        emit writeFailed(lastTerminal.requestId, QStringLiteral("disk-full"));
    }

    int writeCount = 0;
    int loadCount = 0;
    bool deferLoad = false;
    suprai::persistence::SessionSnapshot snapshot;
    int terminalWriteCount = 0;
    suprai::persistence::TurnTerminalWrite lastTerminal;
    suprai::persistence::TurnStartWrite lastWrite;
};

class RuntimeLayeringTest final : public QObject
{
    Q_OBJECT

private slots:
    void durableAckPrecedesProviderInference()
    {
        using namespace suprai::runtime::internal;

        auto *provider = new FakeProvider;
        AgentEngine engine(provider);
        FakePersistencePort persistence;

        RuntimeOrchestrator orchestrator(
            {
                .model = QStringLiteral("test-model"),
                .systemPrompt = QStringLiteral("system"),
            },
            &engine,
            &persistence);

        orchestrator.start();
        orchestrator.submitPrompt(QStringLiteral("persistime primero"));

        QCOMPARE(persistence.writeCount, 1);
        QCOMPARE(provider->requests().size(), 0);
        QCOMPARE(orchestrator.history().size(), 0);
        QCOMPARE(orchestrator.inputs().size(), 0);
        QCOMPARE(orchestrator.turns().size(), 0);
        QCOMPARE(orchestrator.runs().size(), 0);

        QCOMPARE(persistence.lastWrite.input.sequence, 1);
        QCOMPARE(persistence.lastWrite.turn.sequence, 1);
        QCOMPARE(persistence.lastWrite.userItem.sequence, 1);
        QCOMPARE(persistence.lastWrite.userItem.turnId, persistence.lastWrite.turn.id);
        QCOMPARE(persistence.lastWrite.run.turnId, persistence.lastWrite.turn.id);

        persistence.succeed();

        QCOMPARE(provider->requests().size(), 1);
        QCOMPARE(persistence.terminalWriteCount, 1);
        QCOMPARE(orchestrator.history().size(), 1);
        QCOMPARE(persistence.lastTerminal.status, QStringLiteral("completed"));
        QVERIFY(persistence.lastTerminal.assistantItem.has_value());

        persistence.terminalSucceeded();
        QCOMPARE(orchestrator.history().size(), 2);
        QCOMPARE(orchestrator.inputs().size(), 1);
        QCOMPARE(orchestrator.turns().size(), 1);
        QCOMPARE(orchestrator.runs().size(), 1);
    }

    void terminalWriteFailureDoesNotPublishCanonicalAnswer()
    {
        using namespace suprai::runtime::internal;
        auto *provider = new FakeProvider;
        AgentEngine engine(provider);
        FakePersistencePort persistence;
        RuntimeOrchestrator orchestrator({.model = QStringLiteral("test")}, &engine, &persistence);
        int completed = 0;
        connect(&orchestrator, &RuntimeOrchestrator::eventOccurred,
                this, [&](const suprai::runtime::RuntimeApplicationEvent &event) {
            if (suprai::runtime::eventPayload<suprai::runtime::ConversationItemCompleted>(event)) {
                ++completed;
            }
        });
        orchestrator.start();
        orchestrator.submitPrompt(QStringLiteral("hola"));
        persistence.succeed();
        QCOMPARE(orchestrator.history().size(), 1);
        QCOMPARE(completed, 0);
        persistence.terminalFailed();
        QCOMPARE(orchestrator.history().size(), 1);
        QCOMPARE(completed, 1); // End UI streaming, do not commit canonical answer.
        QCOMPARE(provider->requests().size(), 1);
    }

    void cancellationBeforeInferenceDurablyClosesRun()
    {
        using namespace suprai::runtime::internal;
        auto *provider = new FakeProvider;
        AgentEngine engine(provider);
        FakePersistencePort persistence;
        RuntimeOrchestrator orchestrator({.model = QStringLiteral("test")}, &engine, &persistence);
        orchestrator.start();
        orchestrator.submitPrompt(QStringLiteral("cancelar"));
        orchestrator.cancelTurn();
        QCOMPARE(provider->requests().size(), 0);
        persistence.succeed();
        QCOMPARE(persistence.terminalWriteCount, 1);
        QCOMPARE(persistence.lastTerminal.status, QStringLiteral("cancelled"));
        QVERIFY(!persistence.lastTerminal.assistantItem.has_value());
        QCOMPARE(provider->requests().size(), 0);
        persistence.terminalSucceeded();
        QCOMPARE(orchestrator.history().size(), 1);
    }

    void providerFailureRequiresDurableTerminalAck()
    {
        using namespace suprai::runtime::internal;
        auto *provider = new FakeProvider;
        provider->failNext = true;
        AgentEngine engine(provider);
        FakePersistencePort persistence;
        RuntimeOrchestrator orchestrator({.model = QStringLiteral("test")}, &engine, &persistence);
        orchestrator.start();
        orchestrator.submitPrompt(QStringLiteral("fallar"));
        persistence.succeed();
        QCOMPARE(persistence.terminalWriteCount, 1);
        QCOMPARE(persistence.lastTerminal.status, QStringLiteral("failed"));
        QCOMPARE(orchestrator.history().size(), 1);
        persistence.terminalSucceeded();
        QCOMPARE(orchestrator.history().size(), 1);
    }

    void startupRestoresLocalHistoryAndLineage()
    {
        using namespace suprai::runtime::internal;
        const suprai::domain::Session session{.id = suprai::domain::newSessionId()};
        const suprai::domain::Input input{
            .id = suprai::domain::newInputId(), .sessionId = session.id,
            .sequence = 1, .text = QStringLiteral("vieja")};
        const suprai::domain::Turn turn{
            .id = suprai::domain::newTurnId(), .sessionId = session.id,
            .inputId = input.id, .sequence = 1};
        const suprai::domain::Run run{
            .id = suprai::domain::newRunId(), .turnId = turn.id,
            .generation = 1, .status = suprai::domain::RunStatus::Completed};
        FakePersistencePort persistence;
        persistence.deferLoad = true;
        persistence.snapshot.found = true;
        persistence.snapshot.session = session;
        persistence.snapshot.inputs = {input};
        persistence.snapshot.turns = {turn};
        persistence.snapshot.runs = {run};
        persistence.snapshot.items = {
            suprai::domain::makeMessageItem(suprai::domain::ConversationRole::User,
                input.text, suprai::domain::ConversationItemState::Completed,
                suprai::domain::newItemId(), turn.id, 1),
            suprai::domain::makeMessageItem(suprai::domain::ConversationRole::Assistant,
                QStringLiteral("guardada"), suprai::domain::ConversationItemState::Completed,
                suprai::domain::newItemId(), turn.id, 2),
        };

        auto *provider = new FakeProvider;
        AgentEngine engine(provider);
        RuntimeOrchestrator orchestrator({.model = QStringLiteral("test")}, &engine, &persistence);
        orchestrator.start();
        QCOMPARE(persistence.loadCount, 1);
        orchestrator.submitPrompt(QStringLiteral("ignorar mientras se restaura"));
        QCOMPARE(persistence.writeCount, 0);
        persistence.loaded();
        QCOMPARE(orchestrator.session().id, session.id);
        QCOMPARE(orchestrator.history().size(), 2);
        QCOMPARE(orchestrator.runs().constLast().status, suprai::domain::RunStatus::Completed);
        orchestrator.submitPrompt(QStringLiteral("nueva"));
        QCOMPARE(persistence.writeCount, 1);
        QCOMPARE(persistence.lastWrite.input.sequence, 2);
        QCOMPARE(persistence.lastWrite.turn.sequence, 2);
        QCOMPARE(persistence.lastWrite.turn.parentTurnId, turn.id);
        QCOMPARE(provider->requests().size(), 0);
        persistence.succeed();
        QCOMPARE(provider->requests().size(), 1);
        QCOMPARE(provider->requests().constLast().messages.size(), 3);
    }

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
        QCOMPARE(firstInput.sequence, 1);
        QCOMPARE(firstTurn.sessionId, initialSessionId);
        QCOMPARE(firstTurn.inputId, firstInput.id);
        QCOMPARE(firstTurn.sequence, 1);
        QVERIFY(firstTurn.parentTurnId.isEmpty());
        QCOMPARE(firstRun.turnId, firstTurn.id);
        QCOMPARE(firstRun.generation, 1);
        QCOMPARE(orchestrator.history().at(0).turnId, firstTurn.id);
        QCOMPARE(orchestrator.history().at(0).sequence, 1);
        QCOMPARE(orchestrator.history().at(1).turnId, firstTurn.id);
        QCOMPARE(orchestrator.history().at(1).sequence, 2);

        orchestrator.submitPrompt(QStringLiteral("segundo"));

        QCOMPARE(orchestrator.inputs().size(), 2);
        QCOMPARE(orchestrator.turns().size(), 2);
        QCOMPARE(orchestrator.runs().size(), 2);
        QCOMPARE(orchestrator.history().size(), 4);

        const auto secondTurn = orchestrator.turns().at(1);
        QCOMPARE(orchestrator.inputs().at(1).sequence, 2);
        QCOMPARE(secondTurn.sequence, 2);
        QCOMPARE(secondTurn.parentTurnId, firstTurn.id);
        QCOMPARE(orchestrator.history().at(2).turnId, secondTurn.id);
        QCOMPARE(orchestrator.history().at(2).sequence, 1);
        QCOMPARE(orchestrator.history().at(3).turnId, secondTurn.id);
        QCOMPARE(orchestrator.history().at(3).sequence, 2);

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

        int completed = 0;
        QVector<bool> reasoning;

        connect(&orchestrator, &RuntimeOrchestrator::eventOccurred,
                this, [&](const suprai::runtime::RuntimeApplicationEvent &event) {
            if (suprai::runtime::eventPayload<suprai::runtime::ConversationItemCompleted>(event)) {
                ++completed;
            } else if (const auto *active =
                           suprai::runtime::eventPayload<suprai::runtime::ReasoningActiveChanged>(event)) {
                reasoning.push_back(active->active);
            }
        });

        orchestrator.start();
        orchestrator.submitPrompt(QStringLiteral("primero"));

        QCOMPARE(completed, 1);
        QCOMPARE(provider->requests().size(), 1);
        QCOMPARE(reasoning.size(), 2);
        QCOMPARE(reasoning.at(0), true);
        QCOMPARE(reasoning.at(1), false);

        orchestrator.submitPrompt(QStringLiteral("segundo"));

        QCOMPARE(completed, 2);
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
