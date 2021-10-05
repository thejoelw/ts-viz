#pragma once

namespace util {

template <typename Type>
struct PublicWrapper : public Type {};

template <typename Type>
struct PrivateWrapper {
    Type val;
};

}
