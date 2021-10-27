#include "variablemanager.h"

#include "program/programmanager.h"

namespace program {

VariableManager::VariableManager(app::AppContext &context) {
    (void) context;
}

void VariableManager::clearVariables() {
    vars.clear();
}

void VariableManager::addVariable(Variable *var) {
    bool inserted = vars.emplace(var->getKey(), var->getValue()).second;
    if (!inserted) {
        throw ProgramManager::InvalidProgramException("Variable with name " + var->getKey() + " already exists!");
    }
}

ProgObj VariableManager::lookup(const std::string &key) {
    std::unordered_map<std::string, ProgObj>::const_iterator found = vars.find(key);
    if (found == vars.cend()) {
        throw ProgramManager::InvalidProgramException("No variable with name " + key + " exists!");
    }
    return found->second;
}

}
