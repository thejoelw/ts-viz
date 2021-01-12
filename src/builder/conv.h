#include "program/resolver.h"
#include "series/type/convseries.h"

// Splitting this up into 2 compilation units because it takes so long to compile

template <typename RealType>
void declConv(app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("conv", [&context](series::DataSeries<RealType> *kernel, series::DataSeries<RealType> *ts, std::int64_t kernelSize, bool backfillZeros) {
        return new series::ConvSeries<RealType>(context, *kernel, *ts, kernelSize, backfillZeros);
    });
}
