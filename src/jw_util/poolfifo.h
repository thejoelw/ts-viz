#ifndef JWUTIL_POOLFIFO_H
#define JWUTIL_POOLFIFO_H

#include <assert.h>

namespace jw_util
{

// Thread-safe, if only one thread calls alloc and one thread calls free

template <typename Type>
class PoolFIFO
{
public:
    PoolFIFO()
        : alloc_cur(new Type[alloc_size])
        , alloc_end(alloc_cur + alloc_size)
        , free_cur(alloc_cur)
        , free_end(alloc_end)
    {}

    Type *alloc()
    {
        if (alloc_cur == alloc_end)
        {
            alloc_cur = new Type[alloc_size];
            alloc_end = alloc_cur + alloc_size;
        }
        return alloc_cur++;
    }

    void free(const Type *type)
    {
        if (free_cur == free_end)
        {
            const Type *free_start = free_end - alloc_size;
            delete[] free_start;

            free_cur = type;
            free_end = free_cur + alloc_size;
        }

        assert(free_cur == type);
        free_cur++;
    }

private:
    static constexpr unsigned int alloc_bytes = 65536;
    static constexpr unsigned int alloc_size = alloc_bytes > sizeof(Type) ? alloc_bytes / sizeof(Type) : 1;

    Type *alloc_cur;
    Type *alloc_end;

    // Padding so alloc_* and free_* are on different cache lines
    char _padding[64];

    const Type *free_cur;
    const Type *free_end;
};

}

#endif // JWUTIL_POOLFIFO_H
