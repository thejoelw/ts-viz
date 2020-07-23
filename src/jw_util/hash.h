#ifndef JWUTIL_HASH_H
#define JWUTIL_HASH_H

#include <cstddef>

namespace jw_util
{

template <typename HashType = std::size_t>
class Hash {
public:
    static constexpr HashType combine(HashType hash, HashType val) {
        return hash ^ (val + static_cast<HashType>(0x9e3779b97f4a7c15u) + (hash << 6) + (hash >> 2));
    }
};

}

#endif // JWUTIL_HASH_H
