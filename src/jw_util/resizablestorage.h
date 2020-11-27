#ifndef JWUTIL_RESIZABLESTORAGE_H
#define JWUTIL_RESIZABLESTORAGE_H

#include <cassert>
#include <algorithm>

namespace jw_util
{

template <typename DataType, bool fill_zero = false>
class ResizableStorage
{
public:
    ResizableStorage()
        : data(0)
        , size(0)
    {}

    ResizableStorage(unsigned int init_size)
        : data(new DataType[init_size])
        , size(init_size)
    {
        if (fill_zero)
        {
            std::fill_n(data, init_size, static_cast<DataType>(0));
        }
    }

    template <typename... UpdatePtrs>
    void resize(unsigned int new_size, UpdatePtrs &... ptrs)
    {
        if (new_size <= size) {return;}

        unsigned int new_size_2 = size + (size / 2);
        if (new_size < new_size_2) {new_size = new_size_2;}

        DataType *new_data = new DataType[new_size];
        std::move(data, data + size, new_data);
        if (fill_zero)
        {
            std::fill(new_data + size, new_data + new_size, static_cast<DataType>(0));
        }

        update_ptrs(reinterpret_cast<const char *>(data), reinterpret_cast<char *>(new_data), ptrs...);

        delete[] data;

        data = new_data;
        size = new_size;
    }

    DataType *begin() const {return data;}
    DataType *end() const {return data + size;}

private:
    DataType *data;
    unsigned int size;

    template <typename PtrType, typename... UpdatePtrs>
#ifdef NDEBUG
    static
#endif
    void update_ptrs(const char *old_data, char *new_data, PtrType *&ptr, UpdatePtrs &... rest)
    {
        unsigned int offset = reinterpret_cast<const char *>(ptr) - old_data;
#ifndef NDEBUG
        assert(offset <= size * sizeof(DataType));
#endif
        ptr = reinterpret_cast<PtrType *>(new_data + offset);
        update_ptrs(old_data, new_data, rest...);
    }

    static void update_ptrs(const char *old_data, char *new_data) {}
};

}

#endif // JWUTIL_RESIZABLESTORAGE_H
