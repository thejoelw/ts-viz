#include "program/resolver.h"
#include "series/type/parallelopseries.h"

template <typename RealType> struct FuncSgn {
    RealType operator()(RealType a) const {
        static_assert((static_cast<RealType>(0) < NAN) == (NAN < static_cast<RealType>(0)), "NaN behavior is unexpected");
        return (static_cast<RealType>(0) < a) - (a < static_cast<RealType>(0));
    }
};

template <typename RealType> struct FuncAbs { RealType operator()(RealType a) const { return std::abs(a); } };
template <typename RealType> struct FuncInv { RealType operator()(RealType a) const { return RealType(1.0) / a; } };
template <typename RealType> struct FuncSquare { RealType operator()(RealType a) const { return a * a; } };
template <typename RealType> struct FuncSqrt { RealType operator()(RealType a) const { return std::sqrt(a); } };
template <typename RealType> struct FuncCbrt { RealType operator()(RealType a) const { return std::cbrt(a); } };
template <typename RealType> struct FuncExp { RealType operator()(RealType a) const { return std::exp(a); } };
template <typename RealType> struct FuncLog { RealType operator()(RealType a) const { return std::log(a); } };
template <typename RealType> struct FuncExpm1 { RealType operator()(RealType a) const { return std::expm1(a); } };
template <typename RealType> struct FuncLog1p { RealType operator()(RealType a) const { return std::log1p(a); } };
template <typename RealType> struct FuncSigmoid { RealType operator()(RealType a) const { return RealType(1.0) / (RealType(1.0) + std::exp(-a)); } };
template <typename RealType> struct FuncSin { RealType operator()(RealType a) const { return std::sin(a); } };
template <typename RealType> struct FuncCos { RealType operator()(RealType a) const { return std::cos(a); } };
template <typename RealType> struct FuncTan { RealType operator()(RealType a) const { return std::tan(a); } };
template <typename RealType> struct FuncAsin { RealType operator()(RealType a) const { return std::asin(a); } };
template <typename RealType> struct FuncAcos { RealType operator()(RealType a) const { return std::acos(a); } };
template <typename RealType> struct FuncAtan { RealType operator()(RealType a) const { return std::atan(a); } };
template <typename RealType> struct FuncSinh { RealType operator()(RealType a) const { return std::sinh(a); } };
template <typename RealType> struct FuncCosh { RealType operator()(RealType a) const { return std::cosh(a); } };
template <typename RealType> struct FuncTanh { RealType operator()(RealType a) const { return std::tanh(a); } };
template <typename RealType> struct FuncAsinh { RealType operator()(RealType a) const { return std::asinh(a); } };
template <typename RealType> struct FuncAcosh { RealType operator()(RealType a) const { return std::acosh(a); } };
template <typename RealType> struct FuncAtanh { RealType operator()(RealType a) const { return std::atanh(a); } };
template <typename RealType> struct FuncErf { RealType operator()(RealType a) const { return std::erf(a); } };
template <typename RealType> struct FuncErfc { RealType operator()(RealType a) const { return std::erfc(a); } };
template <typename RealType> struct FuncLgamma { RealType operator()(RealType a) const { return std::lgamma(a); } };
template <typename RealType> struct FuncTgamma { RealType operator()(RealType a) const { return std::tgamma(a); } };
template <typename RealType> struct FuncFloor { RealType operator()(RealType a) const { return std::floor(a); } };
template <typename RealType> struct FuncCeil { RealType operator()(RealType a) const { return std::ceil(a); } };
template <typename RealType> struct FuncRound { RealType operator()(RealType a) const { return std::round(a); } };
template <typename RealType> struct FuncNot { RealType operator()(RealType a) const { return !a; } };
template <typename RealType> struct FuncIsNan { RealType operator()(RealType a) const { return std::isnan(a); } };
template <typename RealType> struct FuncIsNum { RealType operator()(RealType a) const { return !std::isnan(a); } };

template <template <typename> typename Operator>
void declUnaryOp(app::AppContext &context, program::Resolver &resolver, const char *funcName) {
    resolver.decl(funcName, Operator<float>());
    resolver.decl(funcName, Operator<double>());
    resolver.decl(funcName, [&context](series::DataSeries<float> *a){
        return new series::ParallelOpSeries<float, Operator<float>, series::DataSeries<float>>(context, Operator<float>(), *a);
    });
    resolver.decl(funcName, [&context](series::DataSeries<double> *a){
        return new series::ParallelOpSeries<double, Operator<double>, series::DataSeries<double>>(context, Operator<double>(), *a);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declUnaryOp<FuncSgn>(context, resolver, "sgn");
    declUnaryOp<FuncAbs>(context, resolver, "abs");
    declUnaryOp<FuncInv>(context, resolver, "inv");
    declUnaryOp<FuncSquare>(context, resolver, "square");
    declUnaryOp<FuncSqrt>(context, resolver, "sqrt");
    declUnaryOp<FuncCbrt>(context, resolver, "cbrt");
    declUnaryOp<FuncExp>(context, resolver, "exp");
    declUnaryOp<FuncLog>(context, resolver, "log");
    declUnaryOp<FuncExpm1>(context, resolver, "expm1");
    declUnaryOp<FuncLog1p>(context, resolver, "log1p");
    declUnaryOp<FuncSigmoid>(context, resolver, "sigmoid");
    declUnaryOp<FuncSin>(context, resolver, "sin");
    declUnaryOp<FuncCos>(context, resolver, "cos");
    declUnaryOp<FuncTan>(context, resolver, "tan");
    declUnaryOp<FuncAsin>(context, resolver, "asin");
    declUnaryOp<FuncAcos>(context, resolver, "acos");
    declUnaryOp<FuncAtan>(context, resolver, "atan");
    declUnaryOp<FuncSinh>(context, resolver, "sinh");
    declUnaryOp<FuncCosh>(context, resolver, "cosh");
    declUnaryOp<FuncTanh>(context, resolver, "tanh");
    declUnaryOp<FuncAsinh>(context, resolver, "asinh");
    declUnaryOp<FuncAcosh>(context, resolver, "acosh");
    declUnaryOp<FuncAtanh>(context, resolver, "atanh");
    declUnaryOp<FuncErf>(context, resolver, "erf");
    declUnaryOp<FuncErfc>(context, resolver, "erfc");
    declUnaryOp<FuncLgamma>(context, resolver, "lgamma");
    declUnaryOp<FuncTgamma>(context, resolver, "tgamma");
    declUnaryOp<FuncFloor>(context, resolver, "floor");
    declUnaryOp<FuncCeil>(context, resolver, "ceil");
    declUnaryOp<FuncRound>(context, resolver, "round");
    declUnaryOp<FuncNot>(context, resolver, "not");
    declUnaryOp<FuncIsNan>(context, resolver, "is_nan");
    declUnaryOp<FuncIsNum>(context, resolver, "is_num");
});
