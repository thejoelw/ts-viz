#pragma once

#include <string>

#include "jw_util/baseexception.h"

namespace app { class AppContext; }

namespace stream {

// An emitter should have getValue called with an incremental, monotonic argument.

class SeriesEmitter {
public:
    SeriesEmitter(const std::string &key)
        : key(key)
    {}

    virtual std::pair<bool, double> getValue(std::size_t index) = 0;

    const std::string &getKey() const {
        return key;
    }

protected:
    std::string key;
};

}
