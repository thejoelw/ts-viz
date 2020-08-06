#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"

namespace series {

template <typename ElementType>
class InputSeries : public DataSeries<ElementType> {
public:
    InputSeries(tf::Taskflow &taskflow, const std::string &)
        : DataSeries<ElementType>(taskflow.emplace([](){}))
    {}

    void propogateRequest() {}

    void set(std::size_t index, ElementType value) {
        this->request(index, index + 1);
        sets.emplace_back(index, value);
    }

    void computeInto(ElementType *dst, std::size_t begin, std::size_t end) override {
        for (auto s : sets) {
            assert(begin <= s.first);
            assert(s.first < end);
            dst[s.first - begin] = s.second;
        }
        sets.clear();
    }

private:
    std::vector<std::pair<std::size_t, ElementType>> sets;
};

}
