#include "providers/SseDecoder.h"

namespace suprai::providers {

QVector<QByteArray> SseDecoder::feed(const QByteArray &chunk)
{
    QVector<QByteArray> events;
    m_buffer += chunk;

    while (true) {
        const auto newline = m_buffer.indexOf('\n');
        if (newline < 0) {
            break;
        }

        QByteArray line = m_buffer.left(newline);
        m_buffer.remove(0, newline + 1);

        if (line.endsWith('\r')) {
            line.chop(1);
        }

        if (!line.startsWith("data:")) {
            continue;
        }

        line.remove(0, 5);
        events.push_back(line.trimmed());
    }

    return events;
}

void SseDecoder::reset()
{
    m_buffer.clear();
}

} // namespace suprai::providers
