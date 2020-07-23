#ifndef JWUTIL_POOL_H
#define JWUTIL_POOL_H

#include <vector>

namespace jw_util
{

template <typename Type, std::size_t blockSize = 1024 * 64>
class Pool {
private:
    static constexpr unsigned int floorLog2(unsigned long long x) {
        return sizeof(unsigned long long) * CHAR_BIT - 1 - __builtin_clzll(x);
    }

    static constexpr std::size_t tryNumItems = blockSize < sizeof(Type) ? 1 : blockSize / sizeof(Type);
    static constexpr unsigned int numItemsRoundLog2 = floorLog2(tryNumItems * 1.41421356237309504880168872420969808);
    static constexpr std::size_t numItems = static_cast<std::size_t>(1) << numItemsRoundLog2;

public:
    ~Pool() {
        std::sort(freed.begin(), freed.end());

        for (std::size_t i = 0; i < size; i++) {
            std::size_t blockIndex = i / numItems;
            Type *ptr = static_cast<Type *>(blocks[blockIndex]) + (i % numItems);
            if (!std::binary_search(freed.cbegin(), freed.cend(), ptr)) {
                ptr->Type::~Type();
            }
        }

        std::vector<void *>::const_iterator i = blocks.cbegin();
        while (i != blocks.cend()) {
            ::free(*i);
            i++;
        }
    }

    template <typename... ArgTypes>
    Type *alloc(ArgTypes &&... args) {
        if (freed.empty()) {
            std::size_t blockIndex = size / numItems;
            if (blockIndex == blocks.size()) {
                blocks.push_back(::malloc(numItems * sizeof(Type)));
            }
            Type *ptr = static_cast<Type *>(blocks[blockIndex]) + (size % numItems);
            size++;
            return new(ptr) Type(std::forward<ArgTypes>(args)...);
        } else {
            Type *ptr = freed.back();
            freed.pop_back();
            return new (ptr) Type(std::forward<ArgTypes>(args)...);
        }
    }

    void free(const Type *type) {
        assert(type);
        type->Type::~Type();
        freed.push_back(const_cast<Type *>(type));
    }

private:
    std::vector<void *> blocks;
    std::size_t size = 0;
    std::vector<Type *> freed;
};

}

#endif // JWUTIL_POOL_H
