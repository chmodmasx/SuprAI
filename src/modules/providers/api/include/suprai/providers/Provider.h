#pragma once

#include <QObject>
#include <QString>
#include <QVector>

namespace suprai::providers {

struct ProviderMessage {
    QString role;
    QString content;
};

struct ProviderRequest {
    QString model;
    QVector<ProviderMessage> messages;
};

enum class CapabilitySupport {
    Unknown,
    Supported,
    Unsupported
};

struct ProviderCapabilities {
    CapabilitySupport chatCompletions = CapabilitySupport::Unknown;
    CapabilitySupport responses = CapabilitySupport::Unknown;
    CapabilitySupport toolCalling = CapabilitySupport::Unknown;
    CapabilitySupport imageInput = CapabilitySupport::Unknown;
    CapabilitySupport reasoningOutput = CapabilitySupport::Unknown;
    CapabilitySupport inputTokenCounting = CapabilitySupport::Unknown;
};

class Provider : public QObject
{
    Q_OBJECT

public:
    explicit Provider(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~Provider() override = default;

    virtual bool isBusy() const = 0;
    virtual ProviderCapabilities capabilities(const QString &model) const = 0;

public slots:
    virtual void generate(const ProviderRequest &request) = 0;
    virtual void cancel() = 0;

signals:
    void textDelta(const QString &delta);
    void reasoningDelta(const QString &delta);
    void completed();
    void failed(const QString &message);
    void cancelled();
};

} // namespace suprai::providers
