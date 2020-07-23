#pragma once

#include <cstdlib>

namespace util {

template <typename KeyType>
struct HashForwarder {
    std::size_t operator()(const KeyType key) const {
        return key.getHash();
    }
};

}
