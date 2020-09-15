#pragma once

#include "series/dataseries.h"

namespace {

template<typename T>
T && vmin(T && val) {
    return std::forward<T>(val);
}
template<typename T0, typename T1, typename... Ts>
auto vmin(T0 && val1, T1 && val2, Ts &&... vs) {
    return (val1 < val2) ?
        vmin(val1, std::forward<Ts>(vs)...) :
        vmin(val2, std::forward<Ts>(vs)...);
}

template<typename T>
T && vmax(T && val) {
    return std::forward<T>(val);
}
template<typename T0, typename T1, typename... Ts>
auto vmax(T0 && val1, T1 && val2, Ts &&... vs) {
    return (val1 < val2) ?
        vmax(val2, std::forward<Ts>(vs)...) :
        vmax(val1, std::forward<Ts>(vs)...);
}

}

namespace series {

template <typename ElementType, typename OperatorType, typename... ArgTypes>
class ParallelOpSeries : public DataSeries<ElementType> {
public:
    ParallelOpSeries(app::AppContext &context, OperatorType op, ArgTypes &... args)
        : DataSeries<ElementType>(context)
        , op(op)
        , args(args...)
    {
        (this->dependsOn(&args), ...);
    }

    void propogateRequest() override {
        std::apply([this](auto &... x){(x.request(this->requestedBegin, this->requestedEnd), ...);}, args);
    }

    std::pair<std::size_t, std::size_t> computeInto(ElementType *dst, std::size_t begin, std::size_t end) override {
        begin = std::apply([begin](auto &... x){return vmax(begin, x.getComputedBegin()...);}, args);
        end = std::apply([end](auto &... x){return vmin(end, x.getComputedEnd()...);}, args);
        if (begin >= end) {
            return std::make_pair(0, 0);
        }
        auto sources = std::apply([begin, end](auto &... x){return std::make_tuple(x.getData(begin, end).first...);}, args);

        for (std::size_t i = begin; i < end; i++) {
            *dst++ = std::apply([this](auto *&... s){return op(*s++...);}, sources);
        }

        return std::make_pair(begin, end);
    }

private:
    OperatorType op;

    std::tuple<ArgTypes &...> args;
};

}
