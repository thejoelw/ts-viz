#include "program/resolver.h"
#include "series/type/scannedseries.h"

template <typename RealType> struct DecayingPlus {
    DecayingPlus(RealType decayRate)
        : mul1(static_cast<RealType>(1) - decayRate)
        , mul2(decayRate)
    {}

    RealType operator()(RealType a, RealType b) const { return a * mul1 + b * mul2; }

private:
    RealType mul1;
    RealType mul2;
};

template <typename RealType> struct DecayingPlusVarying {
    RealType operator()(RealType a, RealType b, RealType decayRate) const { return a * (RealType(1.0) - decayRate) + b * decayRate; }
};

template <typename RealType> struct LogSumExp {
    RealType operator()(RealType a, RealType b) const {
        RealType max = std::max(a, b);
        a = std::exp(a - max);
        b = std::exp(b - max);
        return std::log(a + b) + max;
    }
};

template <typename RealType> struct ClampedSum {
    RealType operator()(RealType a, RealType b) const {
        return std::max(RealType(0), a + b);
    }
};

template <typename RealType> struct FuncFwdFillZero {
    RealType operator()(RealType a, RealType b) const { return b == static_cast<RealType>(0.0) ? a : b; }
};

template <typename RealType> struct FuncMonotonify {
    RealType operator()(RealType a, RealType b) const { return std::max(a, b); }
};

template <typename RealType> struct FuncScanIf {
    RealType operator()(RealType prev, RealType cond, RealType _else) const { return cond && std::isfinite(cond) ? prev : _else; }
};

template <template <typename> typename Operator> struct FuncSafeOp {
    template <typename RealType>
    struct type : private Operator<RealType> {
        template <typename... ArgTypes>
        type(ArgTypes... args)
            : Operator<RealType>(args...)
        {}

        template <typename... ArgTypes>
        RealType operator()(ArgTypes... args) const {
            return Operator<RealType>::operator()((std::isfinite(args) ? args : RealType(0.0))...);
        }
    };
};

template <template <typename> typename Operator>
void declScanOp(app::AppContext &context, program::Resolver &resolver, const char *funcName, double initialValue) {
    resolver.decl(funcName, [&context, initialValue](series::DataSeries<float> *a) {
        return new series::ScannedSeries<float, Operator<float>, series::DataSeries<float>>(context, Operator<float>(), initialValue, *a);
    });
    resolver.decl(funcName, [&context, initialValue](series::DataSeries<double> *a) {
        return new series::ScannedSeries<double, Operator<double>, series::DataSeries<double>>(context, Operator<double>(), initialValue, *a);
    });
}

template <template <typename> typename Operator, template <typename> typename OperatorVarying>
void declScanOpP1(app::AppContext &context, program::Resolver &resolver, const char *funcName, double initialValue) {
    resolver.decl(funcName, [&context, initialValue](series::DataSeries<float> *a, float param_0) {
        return new series::ScannedSeries<float, Operator<float>, series::DataSeries<float>>(context, Operator<float>(param_0), initialValue, *a);
    });
    resolver.decl(funcName, [&context, initialValue](series::DataSeries<double> *a, double param_0) {
        return new series::ScannedSeries<double, Operator<double>, series::DataSeries<double>>(context, Operator<double>(param_0), initialValue, *a);
    });

    resolver.decl(funcName, [&context, initialValue](series::DataSeries<float> *a, series::DataSeries<float> *b) {
        return new series::ScannedSeries<float, OperatorVarying<float>, series::DataSeries<float>, series::DataSeries<float>>(context, OperatorVarying<float>(), initialValue, *a, *b);
    });
    resolver.decl(funcName, [&context, initialValue](series::DataSeries<double> *a, series::DataSeries<double> *b) {
        return new series::ScannedSeries<double, OperatorVarying<double>, series::DataSeries<double>, series::DataSeries<double>>(context, OperatorVarying<double>(), initialValue, *a, *b);
    });
}


template <template <typename> typename Operator, typename RealType>
void declScanTernaryOp(app::AppContext &context, program::Resolver &resolver, const char *funcName) {
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, RealType b, RealType initialValue){
        auto op = [b](RealType prev, RealType a) {return Operator<RealType>()(prev, a, b);};
        return new series::ScannedSeries<RealType, decltype(op), series::DataSeries<RealType>>(context, op, initialValue, *a);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, series::DataSeries<RealType> *b, RealType initialValue){
        return new series::ScannedSeries<RealType, Operator<RealType>, series::DataSeries<RealType>, series::DataSeries<RealType>>(context, Operator<RealType>(), initialValue, *a, *b);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declScanOp<FuncSafeOp<std::plus>::type>(context, resolver, "cum_sum", 0.0);
    declScanOp<FuncSafeOp<ClampedSum>::type>(context, resolver, "cum_sum_clamped", 0.0);
    declScanOpP1<FuncSafeOp<DecayingPlus>::type, FuncSafeOp<DecayingPlusVarying>::type>(context, resolver, "decaying_sum", 0.0);
    declScanOp<FuncSafeOp<std::multiplies>::type>(context, resolver, "cum_prod", 1.0);
    declScanOp<FuncSafeOp<LogSumExp>::type>(context, resolver, "cum_log_sum_exp", 0.0);
    declScanOp<FuncSafeOp<FuncFwdFillZero>::type>(context, resolver, "fwd_fill_zero", 0.0);
    declScanOp<FuncSafeOp<FuncMonotonify>::type>(context, resolver, "monotonify", -INFINITY);
    declScanTernaryOp<FuncScanIf, float>(context, resolver, "scan_if");
    declScanTernaryOp<FuncScanIf, double>(context, resolver, "scan_if");
});
