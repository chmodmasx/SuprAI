#pragma once

#include <QByteArray>
#include <QVector>

namespace suprai::providers::internal {

class SseDecoder
{
public:
    QVector<QByteArray> feed(const QByteArray &chunk);
    void reset();

private:
    QByteArray m_buffer;
};

} // namespace suprai::providers::internal
