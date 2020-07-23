#ifndef JWUTIL_ERASABLEUINTQUEUE_H
#define JWUTIL_ERASABLEUINTQUEUE_H

#include <assert.h>
#include <type_traits>
#include <vector>
#include <list>

namespace jw_util
{

template <typename Type>
class ErasableUIntQueue
{
    static_assert(std::is_integral<Type>::value && std::is_unsigned<Type>::value, "ErasableQueue<Type>: Type must be of an unsigned integral type");

    typedef std::vector<typename std::list<Type>::const_iterator> MapType;

public:
    ErasableUIntQueue()
    {}

    void set_value_limit(Type limit)
    {
#ifndef NDEBUG
        if (limit < map.size())
        {
            typename MapType::const_iterator i = map.cbegin() + limit;
            while (i != map.cend())
            {
                assert(*i == list.cend());
                i++;
            }
        }
#endif

        map.resize(limit, list.cend());
    }

    bool has(Type val) const
    {
        assert(val < map.size());
        return map[val] != list.cend();
    }

    template <bool replace = true>
    void push(Type val)
    {
        if (replace)
        {
            if (has(val))
            {
                list.erase(map[val]);
            }
        }
        else
        {
            if (has(val)) {return;}
        }

        list.push_back(val);
        map[val] = std::prev(list.cend());
    }

    /*
    void move(Type old_val, Type new_val)
    {
        if (has(new_val))
        {
            list.erase(map[new_val]);
        }

        if (has(old_val))
        {
            *map[old_val] = new_val;
        }

        map[new_val] = map[old_val];
        map[old_val] = list.cend();
    }
    */

    void erase(Type val)
    {
        if (has(val))
        {
            list.erase(map[val]);
            map[val] = list.cend();
        }
    }

    Type pop()
    {
        assert(!list.empty());
        Type front = list.front();
        map[front] = list.cend();
        list.pop_front();
        return front;
    }

    bool empty() const {return list.empty();}
    unsigned int size() const {return list.size();}

    Type get_value_limit() const {return map.size();}

private:
    // The map of values to list iterators, or list.cend() if the value isn't in the list
    MapType map;

    // The actual fifo queue
    std::list<Type> list;
};

}

#endif // JWUTIL_ERASABLEUINTQUEUE_H
