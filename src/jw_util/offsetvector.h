#ifndef JWUTIL_OFFSETVECTOR_H
#define JWUTIL_OFFSETVECTOR_H

#include <deque>

namespace jw_util
{

template <typename Type>
class OffsetVector
{
public:
    OffsetVector()
    {}

    Type &operator[](typename std::deque<Type>::size_type i)
    {
        if (items.empty())
        {
            items.emplace_back();
            offset = i;
            return items.front();
        }

        if (i < offset)
        {
            items.insert(items.begin(), offset - i, Type());
            offset = i;
            return items.front();
        }

        i -= offset;
        if (i >= items.size())
        {
            items.resize(i + 1);
        }

        return items[i];
    }

protected:
    typename std::deque<Type>::size_type offset;
    std::deque<Type> items;
};

}

#endif // JWUTIL_OFFSETVECTOR_H
