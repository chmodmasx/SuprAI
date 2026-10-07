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
