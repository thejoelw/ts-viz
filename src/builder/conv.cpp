#include "program/resolver.h"
#include "series/type/convseries.h"

template <typename RealType>
void declConv(app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("conv", [&context](series::DataSeries<RealType> *kernel, series::DataSeries<RealType> *ts, std::int64_t kernelSize, bool backfillZeros) {
        return new series::ConvSeries<RealType>(context, *kernel, *ts, kernelSize, backfillZeros);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declConv<float>(context, resolver);
    declConv<double>(context, resolver);
});
