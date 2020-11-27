#ifndef JWUTIL_INTERVALSET_H
#define JWUTIL_INTERVALSET_H

#include <set>
#include <cassert>

namespace jw_util
{

template <typename Type>
class IntervalSet
{
public:
    struct Interval
    {
        Type offset;
        mutable Type limit;

        bool operator<(const Interval &interval) const
        {
            return offset < interval.offset;
        }

        bool operator<(Type val) const
        {
            return offset < val;
        }
    };

    typedef std::set<Interval> Set;

    void insert(Type offset, Type limit)
    {
        Interval interval;
        interval.offset = offset;
        interval.limit = limit;

        insert(interval);
    }

    void insert(Interval interval)
    {
        typename Set::iterator next = set.upper_bound(interval);
        typename Set::iterator prev = next;

        if (prev != set.begin() && (--prev)->limit >= interval.offset)
        {
            if (interval.limit > prev->limit)
            {
                prev->limit = interval.limit;
            }
        }
        else
        {
            prev = set.insert(next, interval);
        }

        while (next != set.end() && prev->limit >= next->offset)
        {
#ifndef NDEBUG
            typename Set::iterator next2 = prev;
            next2++;
            assert(next == next2);
#endif

            if (next->limit > prev->limit)
            {
                prev->limit = next->limit;
            }

            next = set.erase(next);
        }
    }

    void remove(Type offset, Type limit)
    {
        typename Set::iterator next = set.upper_bound(offset);
        typename Set::iterator prev = next;
        if (prev != set.begin() && (--prev)->limit > offset) {
            prev->limit = offset;
        }

        while (next != set.end() && next->offset < limit) {
            if (next->limit > limit) {
                next->offset = limit;
                break;
            } else {
                next = set.erase(next);
            }
        }
    }

    bool intersects(Type offset, Type limit)
    {
        typename Set::iterator i = set.upper_bound(offset);
        if (i != set.end() && i->offset < limit) {
            return true;
        }
        if (i != set.begin() && (--i)->limit > offset) {
            return true;
        }
        return false;
    }

    void merge(Type max_gap)
    {
        typename Set::iterator prev = set.begin();
        if (prev == set.end()) {return;}

        typename Set::iterator next = prev;
        next++;

        while (next != set.end())
        {
#ifndef NDEBUG
            typename Set::iterator next2 = prev;
            next2++;
            assert(next == next2);
#endif

            if (prev->limit + max_gap >= next->offset)
            {
                prev->limit = next->limit;
                next = set.erase(next);
            }
            else
            {
                prev++;
                next++;
            }
        }
    }

    void assert_increasing()
    {
        Type prev;

        typename Set::iterator i = set.cbegin();
        while (i != set.cend())
        {
            if (i != set.cbegin())
            {
                assert(prev < i->offset);
            }
            assert(i->offset < i->limit);
            prev = i->limit;

            i++;
        }
    }

    void clear()
    {
        set.clear();
    }

    Set &get_set() {return set;}
    const Set &get_set() const {return set;}
    typename Set::size_type size() const {return set.size();}
    bool empty() const {return set.empty();}

protected:
    Set set;

#ifdef INTERVALSET_EXPAND_LAST_CHECK
    // TODO: Implement this
    bool valid_last = false;
    Set::iterator last;
#endif
};

}

#endif // JWUTIL_INTERVALSET_H
