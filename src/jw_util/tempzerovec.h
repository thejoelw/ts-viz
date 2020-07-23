#ifndef JWUTIL_TEMPZEROVEC_H
#define JWUTIL_TEMPZEROVEC_H

#include <vector>

namespace jw_util
{

template <typename DataType>
class TempZeroVec
{
public:
    static TempZeroVec alloc(unsigned int size)
    {
#ifdef TEMPZEROVEC_ASSERT_UNLOCKED
        assert(!locked);
        locked = true;
#endif

        if (data.size() < size)
        {
            data.resize(size, 0);
        }

        return TempZeroVec();
    }

    static void free(const TempZeroVec &vec)
    {
#ifdef TEMPZEROVEC_ASSERT_UNLOCKED
        assert(locked);
        locked = false;
#endif

#ifdef TEMPZEROVEC_ASSERT_ZEROED
        typename std::vector<DataType>::const_iterator i = data.cbegin();
        while (i != data.cend())
        {
            assert(*i == static_cast<DataType>(0));
            i++;
        }
#endif
    }

    DataType &operator[](unsigned int i)
    {
        assert(i < data.size());
        return data[i];
    }

    const DataType &operator[](unsigned int i) const
    {
        assert(i < data.size());
        return data[i];
    }

private:
    TempZeroVec()
    {
    }

    static std::vector<DataType> data;

#ifdef TEMPZEROVEC_ASSERT_UNLOCKED
    static bool locked;
#endif
};

template <typename DataType>
std::vector<DataType> TempZeroVec<DataType>::data;

#ifdef TEMPZEROVEC_ASSERT_UNLOCKED
template <typename DataType>
bool TempZeroVec<DataType>::locked = false;
#endif

}

#endif // JWUTIL_TEMPZEROVEC_H
