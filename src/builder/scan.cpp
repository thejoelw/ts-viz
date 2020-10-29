#include "program/resolver.h"
#include "series/scannedseries.h"

template <typename RealType> struct FuncFwdFillZero { RealType operator()(RealType a, RealType b) const { return b == static_cast<RealType>(0.0) ? a : b; } };

template <template <typename> typename Operator> struct FuncSafeBinaryOp {
    template <typename RealType>
    struct type : private Operator<RealType> {
        RealType operator()(RealType a, RealType b) const {
            if (std::isnan(a) || !std::isfinite(a)) { a = static_cast<RealType>(0.0); }
            if (std::isnan(b) || !std::isfinite(b)) { b = static_cast<RealType>(0.0); }
            return Operator<RealType>::operator()(a, b);
        }
    };
};

template <template <typename> typename Operator>
void declScanOp(app::AppContext &context, program::Resolver &resolver, const char *funcName, double initialValue) {
    resolver.decl(funcName, [&context, initialValue](series::DataSeries<float> *a){
        return new series::ScannedSeries<float, Operator<float>, series::DataSeries<float>>(context, Operator<float>(), initialValue, *a);
    });
    resolver.decl(funcName, [&context, initialValue](series::DataSeries<double> *a){
        return new series::ScannedSeries<double, Operator<double>, series::DataSeries<double>>(context, Operator<double>(), initialValue, *a);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declScanOp<FuncSafeBinaryOp<std::plus>::type>(context, resolver, "cumsum", 0.0);
    declScanOp<FuncSafeBinaryOp<std::multiplies>::type>(context, resolver, "cumprod", 1.0);
    declScanOp<FuncSafeBinaryOp<FuncFwdFillZero>::type>(context, resolver, "fwd_fill_zero", 0.0);
});
