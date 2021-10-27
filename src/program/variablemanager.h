#pragma once

#include <unordered_map>

#include "app/appcontext.h"
#include "program/variable.h"

namespace program {

class VariableManager {
public:
    VariableManager(app::AppContext &context);

    void clearVariables();
    void addVariable(Variable *var);
    ProgObj lookup(const std::string &key);

private:
    std::unordered_map<std::string, ProgObj> vars;
};

}
