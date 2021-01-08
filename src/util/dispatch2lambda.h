#pragma once

#include <utility>

#include "util/tag.h"

namespace util {

template <typename ValueType, std::size_t count>
class DispatchToLambda {
public:
    template <typename ReturnType = void, typename FuncType>
    static ReturnType call(ValueType value, FuncType func) {
        assert(value < count);
        return call<ReturnType, FuncType>(std::forward<ValueType>(value), std::forward<FuncType>(func), std::make_index_sequence<count>{});
    }

private:
    template <typename ReturnType, typename FuncType, std::size_t... is>
    static ReturnType call(ValueType value, FuncType func, std::index_sequence<is...>) {
        static constexpr ReturnType (*ptrs[count])(FuncType) = { &dispatch<ReturnType, FuncType, is>... };
        return ptrs[value](std::forward<FuncType>(func));
    }

    template <typename ReturnType, typename FuncType, ValueType value>
    static ReturnType dispatch(FuncType func) {
        return func(Tag<ValueType, value>());
    }
};

}
