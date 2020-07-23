#pragma once

#include "jw_util/hash.h"

#include "series/series.h"

namespace series {

template <typename ElementType>
class InputSeries : public Series<ElementType> {
public:
    InputSeries(tf::Taskflow &taskflow, const std::string &)
        : Series<ElementType>(taskflow.emplace([](){}))
    {}

    void request(std::size_t begin, std::size_t end) {
        assert(begin <= end);
        assert(end < this->data.size() + this->dataOffset);
    }

    void set(std::size_t index, ElementType value) {
        this->data.resize(index + 1 - this->dataOffset);
        *this->getData(index) = value;
    }
};

}
