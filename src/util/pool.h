#pragma once

#include "jw_util/pool.h"

namespace app { class AppContext; }

namespace util {

template <typename Type>
class Pool : public jw_util::Pool<Type> {
public:
    Pool(app::AppContext &context) {
        // Assume Type's constructor uses a dependency Dep.

        // Normal case:
        // context.get<Type>() results in a class order of [Dep, Type].
        // During destruction, Type will be destructed first, and Dep second.
        // This is good.

        // Pool case:
        // context.get<util::Pool<Type>>().alloc() results in a class order of [util::Pool<Type>, Dep].
        // This is bad because Type's destructor will throw an error if it needs Dep and can't get it.
        // This is different because the pool's destructor finishing without getting Dep.

        // To fix this, we ask Type to construct any dependencies here:
        initializeDependencies<Type>(context, 0);
    }

private:
    template <typename Type2>
    static auto initializeDependencies(app::AppContext &context, int) -> decltype(Type2::initializeDependencies(context), void()) {
        Type2::initializeDependencies(context);
    }

    template <typename Type2>
    static void initializeDependencies(app::AppContext &context, long) {
        (void) context;
    }
};

}
