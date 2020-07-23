#pragma once

#include "jw_util/hash.h"

#include "series/series.h"

namespace series {

template <typename ElementType, typename OperatorType>
class StaticSeries : public Series<ElementType> {
public:
    StaticSeries(tf::Taskflow &taskflow, OperatorType op)
        : Series<ElementType>(taskflow.emplace([this, op](){
            if (this->requestedBegin < this->requestedEnd) {
                op(this->data);
            }
        }))
        , op(op)
    {}

    void request(std::size_t begin, std::size_t end) {
        if (begin < this->requestedBegin) {
            this->requestedBegin = begin;
        }
        if (end > this->requestedEnd) {
            this->requestedEnd = end;
        }
    }

private:
    OperatorType op;
};

}
