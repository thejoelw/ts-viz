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

    std::pair<std::size_t, std::size_t> computeInto(ElementType *dst, std::size_t begin, std::size_t end) override {
        assert(begin == 0);
        assert(end == width);

        op(dst, begin, end);

        return std::make_pair(begin, end);
    }

    std::size_t getStaticWidth() const override {
        return width;
    }

private:
    OperatorType op;
    std::size_t width;
};

}
