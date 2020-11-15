#pragma once

#include <string>

#include "jw_util/baseexception.h"

namespace app { class AppContext; }

namespace stream {

class SeriesEmitter {
public:
    SeriesEmitter(app::AppContext &context, const std::string &key)
        : key(key)
    {
        (void) context;
    }

    virtual std::pair<bool, double> getValue(std::size_t index) = 0;

    const std::string &getKey() const {
        return key;
    }

protected:
    std::string key;
};

}
