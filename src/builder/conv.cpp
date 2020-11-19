#include "program/resolver.h"
#include "series/type/convseries.h"

template <typename RealType>
void declConv(app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("conv", [&context](series::FiniteCompSeries<RealType> *kernel, series::DataSeries<RealType> *ts){
        return new series::ConvSeries<RealType>(context, *kernel, *ts, 0.0, false);
    });
    resolver.decl("conv", [&context](series::FiniteCompSeries<RealType> *kernel, series::DataSeries<RealType> *ts, RealType derivativeOrder){
        return new series::ConvSeries<RealType>(context, *kernel, *ts, derivativeOrder, false);
    });

    resolver.decl("conv", [&context](series::FiniteCompSeries<RealType> *kernel, series::DataSeries<RealType> *ts, bool backfillZeros){
        return new series::ConvSeries<RealType>(context, *kernel, *ts, 0.0, backfillZeros);
    });
    resolver.decl("conv", [&context](series::FiniteCompSeries<RealType> *kernel, series::DataSeries<RealType> *ts, RealType derivativeOrder, bool backfillZeros){
        return new series::ConvSeries<RealType>(context, *kernel, *ts, derivativeOrder, backfillZeros);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declConv<float>(context, resolver);
    declConv<double>(context, resolver);
});
