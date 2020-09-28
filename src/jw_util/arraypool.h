#ifndef JWUTIL_ARRAYPOOL_H
#define JWUTIL_ARRAYPOOL_H

#include <vector>

namespace jw_util {

template <typename ElementType>
class ArrayPool {
public:
    ArrayPool(std::size_t arraySize)
        : arraySize(arraySize)
    {}

    ~ArrayPool() {
        for (ElementType *arr : arrs) {
            delete[] arr;
        }
    }

    template <typename... ArgTypes>
    ElementType *alloc() {
        if (arrs.empty()) {
            return new ElementType[arraySize];
        } else {
            ElementType *res = arrs.back();
            arrs.pop_back();
            return res;
        }
    }

    void free(const ElementType *arr) {
        arrs.push_back(arr);
    }

private:
    std::size_t arraySize;
    std::vector<ElementType *> arrs;
};

}

#endif // JWUTIL_ARRAYPOOL_H
