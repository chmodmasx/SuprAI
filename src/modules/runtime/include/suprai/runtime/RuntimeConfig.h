#pragma once

#include <QString>

namespace suprai::runtime {

struct AgentRuntimeConfig {
    QString model;
    QString systemPrompt;
};

} // namespace suprai::runtime
