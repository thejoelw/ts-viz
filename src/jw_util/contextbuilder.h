#ifndef JWUTIL_CONTEXTBUILDER_H
#define JWUTIL_CONTEXTBUILDER_H

#include <vector>
#include <algorithm>

namespace jw_util {

template <typename ContextType>
class ContextBuilder {
public:
    template <typename Type>
    void registerConstructor() {
#ifndef NDEBUG
        typename std::vector<void (*)(ContextType &context)>::const_iterator pos = std::find(getters.cbegin(), getters.cend(), &getter<Type>);
        assert(pos == getters.cend());
#endif

        getters.push_back(&getter<Type>);
    }

    template <typename Type>
    void removeConstructor() {
        typename std::vector<void (*)(ContextType &context)>::iterator pos = std::find(getters.begin(), getters.end(), &getter<Type>);
        assert(pos != getters.end());

        // Can't erase it here because we could be iterating
        *pos = &noop;
    }

    void buildAll(ContextType &context) {
        // Remove any noops
        getters.erase(std::remove(getters.begin(), getters.end(), &noop), getters.end());

        // Can't use an iterator here because one of the calls could register another constructor
        for (std::size_t i = 0; i < getters.size(); i++) {
            (*getters[i])(context);
        }
    }

    std::size_t getSize() const {
        return getters.size() - std::count(getters.cbegin(), getters.cend(), &noop);
    }

private:
    std::vector<void (*)(ContextType &context)> getters;

    template <typename Type>
    static void getter(ContextType &context) {
        context.template get<Type>();
    }

    static void noop(ContextType &context) {
        (void) context;
    }
};

}

#endif // JWUTIL_CONTEXTBUILDER_H
