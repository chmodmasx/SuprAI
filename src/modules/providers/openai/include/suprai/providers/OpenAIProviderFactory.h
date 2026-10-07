#pragma once

#include <QString>

class QObject;

namespace suprai::providers {

class Provider;

struct OpenAIProviderConfig {
    QString baseUrl;
    QString apiKey;
};

Provider *createOpenAIChatProvider(
    OpenAIProviderConfig config,
    QObject *parent = nullptr);

} // namespace suprai::providers
