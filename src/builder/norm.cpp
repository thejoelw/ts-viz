#include "program/resolver.h"
#include "series/type/normalizedseries.h"

template <typename RealType>
void declNorm(app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("norm", [&context](series::DataSeries<RealType> *a, std::int64_t size, bool zeroOutside) {
        return new series::NormalizedSeries<RealType, series::DataSeries<RealType>>(context, *a, size, zeroOutside);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declNorm<float>(context, resolver);
    declNorm<double>(context, resolver);
});
