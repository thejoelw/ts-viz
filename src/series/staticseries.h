#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"

namespace series {

template <typename ElementType, typename OperatorType>
class StaticSeries : public DataSeries<ElementType> {
public:
    StaticSeries(app::AppContext &context, OperatorType op, std::size_t width)
        : DataSeries<ElementType>(context)
        , op(op)
        , width(width)
    {}

    void propogateRequest() override {
        if (this->requestedEnd > width) {
            this->requestedEnd = width;
        }
        if (this->requestedBegin < this->requestedEnd) {
            this->requestedBegin = 0;
            this->requestedEnd = width;
        }
    }

    void computeInto(ElementType *dst, std::size_t begin, std::size_t end) override {
        assert(begin == 0);
        assert(end == width);

        op(dst, begin, end);
    }

private:
    OperatorType op;
    std::size_t width;
};

}
