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
    QString text;
};

struct Turn {
    QString id;
    QString sessionId;
    QString inputId;
    QString parentTurnId;
};

struct Run {
    QString id;
    QString turnId;
    int generation = 1;
};

} // namespace suprai::domain
