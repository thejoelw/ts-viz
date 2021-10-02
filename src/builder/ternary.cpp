#include "program/resolver.h"
#include "series/type/parallelopseries.h"

template <typename RealType> struct FuncCond { RealType operator()(RealType a, RealType b, RealType c) const { return a ? b : c; } };

template <template <typename> typename Operator, typename RealType>
void declTernaryOp(app::AppContext &context, program::Resolver &resolver, const char *funcName) {
    resolver.decl(funcName, [](RealType a, RealType b, RealType c) -> RealType { return Operator<RealType>()(a, b, c); });
    resolver.decl(funcName, [&context](RealType a, RealType b, series::DataSeries<RealType> *c){
        auto op = [a, b](RealType c) {return Operator<RealType>()(a, b, c);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>>(context, op, *c);
    });
    resolver.decl(funcName, [&context](RealType a, series::DataSeries<RealType> *b, RealType c){
        auto op = [a, c](RealType b) {return Operator<RealType>()(a, b, c);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>>(context, op, *b);
    });
    resolver.decl(funcName, [&context](RealType a, series::DataSeries<RealType> *b, series::DataSeries<RealType> *c){
        auto op = [a](RealType b, RealType c) {return Operator<RealType>()(a, b, c);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *b, *c);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, RealType b, RealType c){
        auto op = [b, c](RealType a) {return Operator<RealType>()(a, b, c);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>>(context, op, *a);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, RealType b, series::DataSeries<RealType> *c){
        auto op = [b](RealType a, RealType c) {return Operator<RealType>()(a, b, c);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *a, *c);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, series::DataSeries<RealType> *b, RealType c){
        auto op = [c](RealType a, RealType b) {return Operator<RealType>()(a, b, c);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *a, *b);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, series::DataSeries<RealType> *b, series::DataSeries<RealType> *c){
        return new series::ParallelOpSeries<RealType, Operator<RealType>, series::DataSeries<RealType>, series::DataSeries<RealType>, series::DataSeries<RealType>>(context, Operator<RealType>(), *a, *b, *c);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declTernaryOp<FuncCond, float>(context, resolver, "cond");
    declTernaryOp<FuncCond, double>(context, resolver, "cond");
});
