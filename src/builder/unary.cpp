#include "program/resolver.h"
#include "series/parallelopseries.h"

template <typename RealType> struct FuncSgn { RealType operator()(RealType a) const { return (RealType(0) < a) - (a < RealType(0)); } };
template <typename RealType> struct FuncAbs { RealType operator()(RealType a) const { return std::abs(a); } };
template <typename RealType> struct FuncSquare { RealType operator()(RealType a) const { return a * a; } };
template <typename RealType> struct FuncSqrt { RealType operator()(RealType a) const { return std::sqrt(a); } };
template <typename RealType> struct FuncExp { RealType operator()(RealType a) const { return std::exp(a); } };
template <typename RealType> struct FuncLog { RealType operator()(RealType a) const { return std::log(a); } };

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
    declUnaryOp<FuncSquare>(context, resolver, "square");
    declUnaryOp<FuncSqrt>(context, resolver, "sqrt");
    declUnaryOp<FuncExp>(context, resolver, "exp");
    declUnaryOp<FuncLog>(context, resolver, "log");
});
