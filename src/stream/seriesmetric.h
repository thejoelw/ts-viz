#pragma once

#include <string>

namespace stream {

// A metric should have getValue called with a small number of values (possibly very distant).

class SeriesMetric {
public:
    class ValuePoller {
    public:
        virtual std::pair<bool, double> get() = 0;
    };

    SeriesMetric(const std::string &key)
        : key(key)
    {}

    virtual ValuePoller *makePoller(std::size_t index) = 0;
    virtual void releasePoller(ValuePoller *poller) = 0;

    const std::string &getKey() const {
        return key;
    }

protected:
    std::string key;
};

}
