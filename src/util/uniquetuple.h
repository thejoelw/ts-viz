#pragma once

#include <tuple>

namespace util {

template <typename Out, typename In>
struct BuildUniqueTuple;
template <typename... Out, typename InCar, typename... InCdr>
struct BuildUniqueTuple<std::tuple<Out...>, std::tuple<InCar, InCdr...>> {
    using type = typename std::conditional<
        (std::is_same<Out, InCar>::value || ...),
        typename BuildUniqueTuple<std::tuple<Out...>, std::tuple<InCdr...>>::type,
        typename BuildUniqueTuple<std::tuple<Out..., InCar>, std::tuple<InCdr...>>::type
    >::type;
};
template <typename Out>
struct BuildUniqueTuple<Out, std::tuple<>> {
    using type = Out;
};
static_assert(std::is_same<
        BuildUniqueTuple<std::tuple<>, std::tuple<int, char, bool>>::type,
        std::tuple<int, char, bool>
    >::value, "BuildUniqueTuple test 1 failed!");
static_assert(std::is_same<
        BuildUniqueTuple<std::tuple<>, std::tuple<int, char, int, bool>>::type,
        std::tuple<int, char, bool>
    >::value, "BuildUniqueTuple test 2 failed!");
static_assert(std::is_same<
        BuildUniqueTuple<std::tuple<>, std::tuple<int, char, int, int, bool>>::type,
        std::tuple<int, char, bool>
    >::value, "BuildUniqueTuple test 3 failed!");
static_assert(std::is_same<
        BuildUniqueTuple<std::tuple<>, std::tuple<int, char, int, char, float, int, int, bool, bool>>::type,
        std::tuple<int, char, float, bool>
    >::value, "BuildUniqueTuple test 4 failed!");

}
