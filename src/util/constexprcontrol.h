#pragma once

#include <utility>
#include <tuple>

#include "util/tag.h"

namespace util {

template <typename FuncType, std::size_t... is>
auto constexprFlatFor(std::index_sequence<is...>, FuncType func) {
    return std::tuple_cat(func(Tag<std::size_t, is>())...);
}

}
