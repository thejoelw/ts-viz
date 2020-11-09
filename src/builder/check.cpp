#include "program/resolver.h"
#include "program/programmanager.h"

template <typename RealType>
bool isEq(RealType a, RealType b) {
    RealType dist = std::fabs(a - b);
    bool valid = std::isnan(dist)
            ? std::isnan(a) == std::isnan(b)
            : dist <= std::max(static_cast<RealType>(1.0), std::fabs(a) + std::fabs(b)) * static_cast<RealType>(1e-6);
    return valid;
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("check_eq", [](float a, float b){
        if (isEq(a, b)) {
            return std::monostate();
        } else {
            throw program::Resolver::AssertionFailureException("check_eq(" + std::to_string(a) + ", " + std::to_string(b) + ")");
        }
    });

    resolver.decl("check_eq", [](double a, double b){
        if (isEq(a, b)) {
            return std::monostate();
        } else {
            throw program::Resolver::AssertionFailureException("check_eq(" + std::to_string(a) + ", " + std::to_string(b) + ")");
        }
    });
});
