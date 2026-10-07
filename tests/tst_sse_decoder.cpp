#include "SseDecoder.h"

#include <QTest>

class SseDecoderTest final : public QObject
{
    Q_OBJECT

private slots:
    void handlesSplitFrames()
    {
        suprai::providers::internal::SseDecoder decoder;

        auto events = decoder.feed("data: {\"a\":");
        QCOMPARE(events.size(), 0);

        events = decoder.feed("1}\n\ndata: [DONE]\n");
        QCOMPARE(events.size(), 2);
        QCOMPARE(events.at(0), QByteArray("{\"a\":1}"));
        QCOMPARE(events.at(1), QByteArray("[DONE]"));
    }

    void ignoresNonDataLines()
    {
        suprai::providers::internal::SseDecoder decoder;
        const auto events = decoder.feed(": ping\nevent: message\ndata: hello\n");
        QCOMPARE(events.size(), 1);
        QCOMPARE(events.first(), QByteArray("hello"));
    }
};

QTEST_MAIN(SseDecoderTest)
#include "tst_sse_decoder.moc"
