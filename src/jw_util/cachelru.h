#ifndef JWUTIL_CACHELRU_H
#define JWUTIL_CACHELRU_H

#include <unordered_map>
#include <cassert>

namespace jw_util
{

template <typename KeyType, typename ValueType, unsigned int num_buckets, typename Hasher = std::hash<KeyType>>
class CacheLRU
{
private:
    struct ForgetNode;

public:
    class Result
    {
        friend class CacheLRU;

    public:
        ValueType *get_value() const {return &bucket->first;}
        bool is_valid() const {return valid;}

    private:
        std::pair<ValueType, ForgetNode *> *bucket;
        bool valid;
    };

    CacheLRU()
        : map(num_buckets)
    {
        map.max_load_factor(1.0f);

        forget_connector.prev = &forget_connector;
        forget_connector.next = &forget_connector;

#if JWUTIL_CACHELRU_FORGET_POOL_ON_HEAP
        forget_pool = new ForgetNode[num_buckets];
#endif
        forget_pool_back = forget_pool;
    }

    CacheLRU(const CacheLRU &other)
        : CacheLRU()
    {
        operator=(other);
    }

    ~CacheLRU()
    {
#if JWUTIL_CACHELRU_FORGET_POOL_ON_HEAP
        delete[] forget_pool;
#endif
    }

    CacheLRU &operator=(const CacheLRU &other)
    {
        // I'm lazy, so I've decreed you can only copy empty caches
        assert(other.map.empty());
        assert(other.forget_connector.prev == &other.forget_connector);
        assert(other.forget_connector.next == &other.forget_connector);
        assert(other.forget_available == 0);
        assert(other.forget_pool_back == other.forget_pool);

        map.clear();
        forget_connector.prev = &forget_connector;
        forget_connector.next = &forget_connector;
        forget_available = 0;
        forget_pool_back = forget_pool;

        return *this;
    }

    Result access(const KeyType &key)
    {
        float next_load_factor = (map.size() + 2.0f) / map.bucket_count();
        if (next_load_factor > map.max_load_factor())
        {
            map.erase(forget_get_front()->val);
            forget_shift();
        }

        std::pair<KeyType, std::pair<ValueType, ForgetNode *>> insert;
        insert.first = key;
        std::pair<typename MapType::iterator, bool> element = map.insert(std::move(insert));

        Result res;
        res.bucket = &element.first->second;
        res.valid = !element.second;

        ForgetNode *node;
        if (res.valid)
        {
            // Found element in cache
            node = res.bucket->second;
            forget_erase(node);
        }
        else
        {
            // Did not find, inserted element into cache
            node = forget_alloc();
            node->val = element.first;
            element.first->second.second = node;
        }
        forget_push_back(node);

        return res;
    }

    unsigned int get_bucket_id(const Result &result) const
    {
        assert(result.bucket->second >= forget_pool);
        assert(result.bucket->second < forget_pool_back);

        unsigned int id = result.bucket->second - forget_pool;
        assert(is_bucket_id_valid(id));
        return id;
    }

    bool is_bucket_id_valid(unsigned int id) const
    {
        return id < num_buckets && id < (forget_pool_back - forget_pool);
    }

    const KeyType &lookup_bucket(unsigned int id) const
    {
        assert(is_bucket_id_valid(id));
        return forget_pool[id].val->first;
    }

private:
    typedef std::unordered_map<KeyType, std::pair<ValueType, ForgetNode *>, Hasher> MapType;

    struct ListNode
    {
        ListNode *prev;
        ListNode *next;
    };

    struct ForgetNode : public ListNode
    {
        typename MapType::const_iterator val;
    };

    ForgetNode *forget_alloc()
    {
        ForgetNode *node;
        if (forget_available)
        {
            node = forget_available;
            forget_available = 0;
        }
        else
        {
            assert(forget_pool_back < forget_pool + num_buckets);
            node = forget_pool_back;
            forget_pool_back++;
        }
        return node;
    }

    ForgetNode *forget_get_front() const
    {
        assert(forget_connector.next != &forget_connector);
        return static_cast<ForgetNode *>(forget_connector.next);
    }

    void forget_shift()
    {
        assert(forget_available == 0);

        forget_available = forget_get_front();
        forget_connector.next = forget_connector.next->next;
    }

    void forget_erase(ListNode *node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void forget_push_back(ListNode *node)
    {
        forget_connector.prev->next = node;
        node->prev = forget_connector.prev;
        node->next = &forget_connector;
        forget_connector.prev = node;
    }

    MapType map;

#if JWUTIL_CACHELRU_FORGET_POOL_ON_HEAP
    ForgetNode *forget_pool;
#else
    ForgetNode forget_pool[num_buckets];
#endif

    ForgetNode *forget_available = 0;
    ForgetNode *forget_pool_back;

    // forget_connector.next is the first element of the list (will be shifted)
    // forget_connector.prev is the last element of the list (will be pushed_back to)
    ListNode forget_connector;
};

}

#endif // JWUTIL_CACHELRU_H
