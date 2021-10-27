#pragma once

#include <string>

#include "program/progobj.h"

namespace program {

class Variable {
public:
    Variable(const std::string &key, const ProgObj value)
        : key(key)
        , value(value)
    {}

    const std::string &getKey() const {
        return key;
    }
    const ProgObj getValue() const {
        return value;
    }

protected:
    std::string key;
    const ProgObj value;
};

}
