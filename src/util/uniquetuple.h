#pragma once

#include <tuple>

namespace util {

template <typename Out, typename In>
struct BuildUniqueTuple;

namespace {

template <bool keep, typename Out, typename In>
struct Mover;
template <typename... Out, typename InCar, typename... InCdr>
struct Mover<false, std::tuple<Out...>, std::tuple<InCar, InCdr...>> {
    using type = typename BuildUniqueTuple<std::tuple<Out...>, std::tuple<InCdr...>>::type;
};
template <typename... Out, typename InCar, typename... InCdr>
struct Mover<true, std::tuple<Out...>, std::tuple<InCar, InCdr...>> {
    using type = typename BuildUniqueTuple<std::tuple<Out..., InCar>, std::tuple<InCdr...>>::type;
};

}

template <typename... Out, typename InCar, typename... InCdr>
struct BuildUniqueTuple<std::tuple<Out...>, std::tuple<InCar, InCdr...>> {
    using type = typename Mover<!(std::is_same<Out, InCar>::value || ...), std::tuple<Out...>, std::tuple<InCar, InCdr...>>::type;
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
