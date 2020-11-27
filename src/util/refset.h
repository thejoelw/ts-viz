#pragma once

#include <cassert>
#include <unordered_set>

namespace util {

template <typename Type, typename ContainerType = std::unordered_set<Type *>>
class RefSet {
public:
    typedef typename ContainerType::iterator iterator;
    typedef typename ContainerType::const_iterator const_iterator;

    RefSet() {}

    void add(Type *inst) {
        bool inserted = data.insert(inst).second;
        assert(inserted);
    }

    void remove(Type *inst) {
        bool removed = data.erase(inst);
        assert(removed);
    }

    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }
    const_iterator cbegin() const { return data.cbegin(); }
    const_iterator cend() const { return data.cend(); }

    template <typename... ArgTypes>
    class Invoker {
    public:
        Invoker() = delete;

        template <void (Type::*Method)(ArgTypes...)>
        static void call(const RefSet &refSet, ArgTypes... args) {
            typename ContainerType::const_iterator i = refSet.data.cbegin();
            while (i != refSet.data.cend()) {
                ((*i)->*Method)(std::forward<ArgTypes>(args)...);
                i++;
            }
        }
    };

private:
    ContainerType data;
};

}
