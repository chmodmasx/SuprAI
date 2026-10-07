#include "OpenAIChatProvider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <utility>

namespace suprai::providers {

Provider *createOpenAIChatProvider(OpenAIProviderConfig config, QObject *parent)
{
    return new internal::OpenAIChatProvider(std::move(config), parent);
}

} // namespace suprai::providers

namespace suprai::providers::internal {

OpenAIChatProvider::OpenAIChatProvider(
    suprai::providers::OpenAIProviderConfig config,
    QObject *parent)
    : Provider(parent)
    , m_config(std::move(config))
    , m_network(new QNetworkAccessManager(this))
{
}

bool OpenAIChatProvider::isBusy() const
{
    return !m_reply.isNull();
}

QString OpenAIChatProvider::endpoint() const
{
    QString base = m_config.baseUrl.trimmed();
    while (base.endsWith('/')) {
        base.chop(1);
    }
    return base + QStringLiteral("/chat/completions");
}

void OpenAIChatProvider::generate(const ProviderRequest &request)
{
    if (isBusy()) {
        emit failed(QStringLiteral("El provider ya está procesando una solicitud."));
        return;
    }

    m_decoder.reset();
    m_errorPreview.clear();
    m_pendingError.clear();
    m_cancelRequested = false;

    QJsonArray messages;
    for (const auto &message : request.messages) {
        messages.append(QJsonObject{
            {QStringLiteral("role"), message.role},
            {QStringLiteral("content"), message.content},
        });
    }

    const QJsonObject body{
        {QStringLiteral("model"), request.model},
        {QStringLiteral("messages"), messages},
        {QStringLiteral("stream"), true},
    };

    QNetworkRequest networkRequest{QUrl(endpoint())};
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    networkRequest.setRawHeader("Accept", "text/event-stream");
    if (!m_config.apiKey.isEmpty()) {
        networkRequest.setRawHeader("Authorization", QByteArray("Bearer ") + m_config.apiKey.toUtf8());
    }

    m_reply = m_network->post(networkRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));

    connect(m_reply, &QNetworkReply::readyRead, this, &OpenAIChatProvider::handleReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &OpenAIChatProvider::handleFinished);
}

void OpenAIChatProvider::cancel()
{
    if (!m_reply) {
        return;
    }

    m_cancelRequested = true;
    m_reply->abort();
}

void OpenAIChatProvider::handleReadyRead()
{
    if (!m_reply) {
        return;
    }

    const QByteArray chunk = m_reply->readAll();
    if (m_errorPreview.size() < 64 * 1024) {
        m_errorPreview += chunk.left((64 * 1024) - m_errorPreview.size());
    }

    const auto events = m_decoder.feed(chunk);
    for (const auto &event : events) {
        consumeEvent(event);
    }
}

void OpenAIChatProvider::consumeEvent(const QByteArray &payload)
{
    if (payload == "[DONE]") {
        return;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return;
    }

    const auto root = document.object();

    if (root.contains(QStringLiteral("error"))) {
        const auto error = root.value(QStringLiteral("error")).toObject();
        m_pendingError = error.value(QStringLiteral("message")).toString(
            QStringLiteral("El endpoint devolvió un error."));
        if (m_reply) {
            m_reply->abort();
        }
        return;
    }

    const auto choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        return;
    }

    const auto delta = choices.first().toObject().value(QStringLiteral("delta")).toObject();

    const auto reasoning = delta.value(QStringLiteral("reasoning_content")).toString();
    if (!reasoning.isEmpty()) {
        emit reasoningDelta(reasoning);
    }

    const auto text = delta.value(QStringLiteral("content")).toString();
    if (!text.isEmpty()) {
        emit textDelta(text);
    }
}

void OpenAIChatProvider::handleFinished()
{
    if (!m_reply) {
        return;
    }

    auto *reply = m_reply.data();
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto networkError = reply->error();
    const QString networkErrorString = reply->errorString();

    QString errorMessage = m_pendingError;
    if (errorMessage.isEmpty() && !m_cancelRequested
        && (networkError != QNetworkReply::NoError || status >= 400)) {
        QString detail = QString::fromUtf8(m_errorPreview).trimmed();
        if (detail.size() > 2000) {
            detail.truncate(2000);
        }

        errorMessage = detail.isEmpty()
            ? QStringLiteral("Error HTTP/provider: %1").arg(networkErrorString)
            : QStringLiteral("Error HTTP/provider: %1").arg(detail);
    }

    m_reply = nullptr;
    reply->deleteLater();

    if (m_cancelRequested) {
        emit cancelled();
    } else if (!errorMessage.isEmpty()) {
        finishFailure(errorMessage);
    } else {
        finishSuccess();
    }
}

void OpenAIChatProvider::finishSuccess()
{
    emit completed();
}

void OpenAIChatProvider::finishFailure(const QString &message)
{
    emit failed(message);
}

} // namespace suprai::providers::internal
