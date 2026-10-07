#pragma once

#include "SseDecoder.h"

#include <suprai/providers/OpenAIProviderFactory.h>
#include <suprai/providers/Provider.h>

#include <QByteArray>
#include <QPointer>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

namespace suprai::providers::internal {

class OpenAIChatProvider final : public suprai::providers::Provider
{
    Q_OBJECT

public:
    explicit OpenAIChatProvider(
        suprai::providers::OpenAIProviderConfig config,
        QObject *parent = nullptr);

    bool isBusy() const override;
    suprai::providers::ProviderCapabilities capabilities(const QString &model) const override;

public slots:
    void generate(const suprai::providers::ProviderRequest &request) override;
    void cancel() override;

private:
    void handleReadyRead();
    void handleFinished();
    void consumeEvent(const QByteArray &payload);
    void finishSuccess();
    void finishFailure(const QString &message);
    QString endpoint() const;

    suprai::providers::OpenAIProviderConfig m_config;
    QNetworkAccessManager *m_network = nullptr;
    QPointer<QNetworkReply> m_reply;
    SseDecoder m_decoder;
    QByteArray m_errorPreview;
    QString m_pendingError;
    bool m_cancelRequested = false;
};

} // namespace suprai::providers::internal
