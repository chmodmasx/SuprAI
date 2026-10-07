#pragma once

#include "providers/Provider.h"
#include "providers/SseDecoder.h"

#include <QByteArray>
#include <QPointer>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

namespace suprai::providers {

struct OpenAIChatProviderConfig {
    QString baseUrl;
    QString apiKey;
};

class OpenAIChatProvider final : public Provider
{
    Q_OBJECT

public:
    explicit OpenAIChatProvider(OpenAIChatProviderConfig config, QObject *parent = nullptr);

    bool isBusy() const override;

public slots:
    void generate(const ProviderRequest &request) override;
    void cancel() override;

private:
    void handleReadyRead();
    void handleFinished();
    void consumeEvent(const QByteArray &payload);
    void finishSuccess();
    void finishFailure(const QString &message);
    QString endpoint() const;

    OpenAIChatProviderConfig m_config;
    QNetworkAccessManager *m_network = nullptr;
    QPointer<QNetworkReply> m_reply;
    SseDecoder m_decoder;
    QByteArray m_errorPreview;
    bool m_terminalEmitted = false;
    bool m_cancelRequested = false;
};

} // namespace suprai::providers
