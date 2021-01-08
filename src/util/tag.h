#pragma once

namespace util {

template <typename Type, Type _value>
struct Tag {
    static constexpr Type value = _value;
};

}
