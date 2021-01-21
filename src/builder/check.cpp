#include "program/resolver.h"
#include "program/programmanager.h"

template <typename RealType>
void declCheck(app::AppContext &context, program::Resolver &resolver) {
    (void) context;

    resolver.decl("check_eq", [](RealType a, RealType b){
        bool isEq;
        if constexpr (std::is_floating_point<RealType>::value) {
            RealType dist = std::fabs(a - b);
            isEq = std::isnan(dist)
                    ? std::isnan(a) == std::isnan(b)
                    : dist <= std::max(static_cast<RealType>(1.0), std::fabs(a) + std::fabs(b)) * static_cast<RealType>(1e-6);
        } else {
            isEq = a == b;
        }

        if (isEq) {
            return std::monostate();
        } else {
            throw program::Resolver::AssertionFailureException("check_eq(" + std::to_string(a) + ", " + std::to_string(b) + ")");
        }
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declCheck<float>(context, resolver);
    declCheck<double>(context, resolver);
    declCheck<std::int64_t>(context, resolver);
});
