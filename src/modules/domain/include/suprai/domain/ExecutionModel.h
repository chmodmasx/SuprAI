#pragma once

#include <QString>

namespace suprai::domain {

struct Session {
    QString id;
    QString parentSessionId;
};

struct Input {
    QString id;
    QString sessionId;
    int sequence = 0;
    QString text;
};

struct Turn {
    QString id;
    QString sessionId;
    QString inputId;
    QString parentTurnId;
    int sequence = 0;
};

struct Run {
    QString id;
    QString turnId;
    int generation = 1;
};

} // namespace suprai::domain
