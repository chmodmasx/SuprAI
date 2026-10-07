#include <suprai/providers/OpenAIProviderFactory.h>
#include <suprai/providers/Provider.h>
#include <suprai/runtime/AgentRuntime.h>
#include <suprai/runtime/RuntimeApplicationEvent.h>
#include <suprai/runtime/RuntimeFactory.h>

#include <QHash>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QVector>

#include <algorithm>

class FakeOpenAIServer final : public QObject
{
    Q_OBJECT

public:
    explicit FakeOpenAIServer(QObject *parent = nullptr)
        : QObject(parent)
    {
        connect(&m_server, &QTcpServer::newConnection, this, &FakeOpenAIServer::acceptConnections);
    }

    bool start()
    {
        return m_server.listen(QHostAddress::LocalHost, 0);
    }

    QString baseUrl() const
    {
        return QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort());
    }

    const QList<QByteArray> &requestBodies() const
    {
        return m_requestBodies;
    }

private:
    void acceptConnections()
    {
        while (m_server.hasPendingConnections()) {
            auto *socket = m_server.nextPendingConnection();
            m_buffers.insert(socket, {});

            connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                auto &buffer = m_buffers[socket];
                buffer += socket->readAll();

                const auto headerEnd = buffer.indexOf("\r\n\r\n");
                if (headerEnd < 0) {
                    return;
                }

                const QByteArray headers = buffer.left(headerEnd);
                int contentLength = 0;
                for (const auto &line : headers.split('\n')) {
                    const QByteArray trimmed = line.trimmed();
                    if (trimmed.toLower().startsWith("content-length:")) {
                        contentLength = trimmed.mid(QByteArray("content-length:").size()).trimmed().toInt();
                        break;
                    }
                }

                const int bodyStart = headerEnd + 4;
                if (buffer.size() - bodyStart < contentLength) {
                    return;
                }

                const QByteArray body = buffer.mid(bodyStart, contentLength);
                m_requestBodies.push_back(body);

                const int requestNumber = m_requestBodies.size();
                const QByteArray answer = requestNumber == 1
                    ? QByteArray("Respuesta uno")
                    : QByteArray("Respuesta dos");

                QByteArray response;
                response += "HTTP/1.1 200 OK\r\n";
                response += "Content-Type: text/event-stream\r\n";
                response += "Cache-Control: no-cache\r\n";
                response += "Connection: close\r\n\r\n";
                response += "data: {\"choices\":[{\"delta\":{\"reasoning_content\":\"SECRET_REASONING\"}}]}\n\n";
                response += "data: {\"choices\":[{\"delta\":{\"content\":\"" + answer + "\"}}]}\n\n";
                response += "data: [DONE]\n\n";

                socket->write(response);
                socket->disconnectFromHost();
                buffer.clear();
            });

            connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
                m_buffers.remove(socket);
                socket->deleteLater();
            });
        }
    }

    QTcpServer m_server;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QList<QByteArray> m_requestBodies;
};

class NativeRuntimeTest final : public QObject
{
    Q_OBJECT

private slots:
    void chatAdapterCapabilitiesAreExplicit()
    {
        auto *provider = suprai::providers::createOpenAIChatProvider({
            .baseUrl = QStringLiteral("http://127.0.0.1:1/v1"),
            .apiKey = QStringLiteral("no-key"),
        });

        const auto capabilities = provider->capabilities(QStringLiteral("test-model"));

        QVERIFY(capabilities.chatCompletions == suprai::providers::CapabilitySupport::Supported);
        QVERIFY(capabilities.responses == suprai::providers::CapabilitySupport::Unsupported);
        QVERIFY(capabilities.toolCalling == suprai::providers::CapabilitySupport::Unsupported);
        QVERIFY(capabilities.imageInput == suprai::providers::CapabilitySupport::Unsupported);
        QVERIFY(capabilities.reasoningOutput == suprai::providers::CapabilitySupport::Unknown);
        QVERIFY(capabilities.inputTokenCounting == suprai::providers::CapabilitySupport::Unsupported);

        delete provider;
    }

    void runtimeCapabilitiesPreserveUnknown()
    {
        auto *provider = suprai::providers::createOpenAIChatProvider({
            .baseUrl = QStringLiteral("http://127.0.0.1:1/v1"),
            .apiKey = QStringLiteral("no-key"),
        });

        auto *runtime = suprai::runtime::createNativeRuntime(
            {
                .model = QStringLiteral("test-model"),
                .systemPrompt = {},
            },
            provider);

        bool seen = false;
        suprai::runtime::RuntimeCapabilities capabilities;

        connect(runtime, &suprai::runtime::AgentRuntime::eventOccurred,
                this, [&](const suprai::runtime::RuntimeApplicationEvent &event) {
            if (const auto *changed =
                    suprai::runtime::eventPayload<
                        suprai::runtime::RuntimeCapabilitiesChanged>(event)) {
                capabilities = changed->capabilities;
                seen = true;
            }
        });

        runtime->start();

        QVERIFY(seen);
        QCOMPARE(capabilities.textGeneration, suprai::runtime::CapabilityState::Supported);
        QCOMPARE(capabilities.toolCalling, suprai::runtime::CapabilityState::Unsupported);
        QCOMPARE(capabilities.imageInput, suprai::runtime::CapabilityState::Unsupported);
        QCOMPARE(capabilities.reasoningOutput, suprai::runtime::CapabilityState::Unknown);
        QCOMPARE(
            capabilities.exactInputTokenCounting,
            suprai::runtime::CapabilityState::Unsupported);

        runtime->shutdown();
        delete runtime;
    }

    void reasoningIsEphemeralAcrossTurns()
    {
        FakeOpenAIServer server;
        QVERIFY(server.start());

        auto *provider = suprai::providers::createOpenAIChatProvider({
            .baseUrl = server.baseUrl(),
            .apiKey = QStringLiteral("no-key"),
        });

        auto *runtime = suprai::runtime::createNativeRuntime(
            {
                .model = QStringLiteral("test-model"),
                .systemPrompt = QStringLiteral("Test system prompt"),
            },
            provider);

        QVector<suprai::runtime::RuntimeApplicationEvent> events;
        connect(runtime, &suprai::runtime::AgentRuntime::eventOccurred,
                this, [&events](const suprai::runtime::RuntimeApplicationEvent &event) {
                    events.push_back(event);
                });

        const auto completedCount = [&events]() {
            return std::count_if(
                events.cbegin(),
                events.cend(),
                [](const auto &event) {
                    return suprai::runtime::eventPayload<
                        suprai::runtime::ConversationItemCompleted>(event) != nullptr;
                });
        };

        const auto errorCount = [&events]() {
            return std::count_if(
                events.cbegin(),
                events.cend(),
                [](const auto &event) {
                    return suprai::runtime::eventPayload<
                        suprai::runtime::RuntimeError>(event) != nullptr;
                });
        };

        runtime->start();
        runtime->submitPrompt(QStringLiteral("primer mensaje"));

        QTRY_COMPARE_WITH_TIMEOUT(completedCount(), 1, 3000);
        QCOMPARE(errorCount(), 0);
        QCOMPARE(server.requestBodies().size(), 1);

        runtime->submitPrompt(QStringLiteral("segundo mensaje"));

        QTRY_COMPARE_WITH_TIMEOUT(completedCount(), 2, 3000);
        QCOMPARE(errorCount(), 0);
        QCOMPARE(server.requestBodies().size(), 2);

        const QByteArray secondRequest = server.requestBodies().at(1);

        QVERIFY2(secondRequest.contains("Respuesta uno"),
                 "The canonical assistant answer must be replayed into the next request.");
        QVERIFY2(!secondRequest.contains("SECRET_REASONING"),
                 "Raw reasoning_content must remain ephemeral and absent from future prompts.");
        QVERIFY(secondRequest.contains("primer mensaje"));
        QVERIFY(secondRequest.contains("segundo mensaje"));

        runtime->shutdown();
        delete runtime;
    }
};

QTEST_MAIN(NativeRuntimeTest)
#include "tst_native_runtime.moc"
