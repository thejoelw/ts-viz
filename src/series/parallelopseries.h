#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"

namespace series {

template <typename ElementType, typename OperatorType, typename... ArgTypes>
class ParallelOpSeries : public DataSeries<ElementType> {
public:
    ParallelOpSeries(app::AppContext &context, OperatorType op, ArgTypes &... args)
        : DataSeries<ElementType>(context)
        , op(op)
        , args(args...)
    {
        (this->dependsOn(args), ...);
    }

    void propogateRequest() override {
        std::apply([this](auto &... x){(x.request(this->requestedBegin, this->requestedEnd), ...);}, args);
    }

    void computeInto(ElementType *dst, std::size_t begin, std::size_t end) override {
        auto sources = std::apply([begin, end](auto &... x){std::make_tuple(x.getData(begin, end).first...);}, args);

        for (std::size_t i = begin; i < end; i++) {
            *dst++ = std::apply([this](auto *&... s){return op(*s++...);}, sources);
        }
    }

private:
    OperatorType op;

    std::tuple<ArgTypes &...> args;
};

}
