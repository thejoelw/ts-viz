#include "program/resolver.h"
#include "series/type/parallelopseries.h"

template <typename RealType> struct FuncTriCond { RealType operator()(RealType a, RealType b, RealType c, RealType d) const { return a < static_cast<RealType>(0.0) ? b : a > static_cast<RealType>(0.0) ? d : c; } };

template <template <typename> typename Operator, typename RealType>
void declQuaternaryOp(app::AppContext &context, program::Resolver &resolver, const char *funcName) {
    resolver.decl(funcName, [](RealType a, RealType b, RealType c, RealType d) -> RealType { return Operator<RealType>()(a, b, c, d); });
    resolver.decl(funcName, [&context](RealType a, RealType b, RealType c, series::DataSeries<RealType> *d){
        auto op = [a, b, c](RealType d) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>>(context, op, *d);
    });
    resolver.decl(funcName, [&context](RealType a, RealType b, series::DataSeries<RealType> *c, RealType d){
        auto op = [a, b, d](RealType c) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>>(context, op, *c);
    });
    resolver.decl(funcName, [&context](RealType a, RealType b, series::DataSeries<RealType> *c, series::DataSeries<RealType> *d){
        auto op = [a, b](RealType c, RealType d) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *c, *d);
    });
    resolver.decl(funcName, [&context](RealType a, series::DataSeries<RealType> *b, RealType c, RealType d){
        auto op = [a, c, d](RealType b) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>>(context, op, *b);
    });
    resolver.decl(funcName, [&context](RealType a, series::DataSeries<RealType> *b, RealType c, series::DataSeries<RealType> *d){
        auto op = [a, c](RealType b, RealType d) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *b, *d);
    });
    resolver.decl(funcName, [&context](RealType a, series::DataSeries<RealType> *b, series::DataSeries<RealType> *c, RealType d){
        auto op = [a, d](RealType b, RealType c) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *b, *c);
    });
    resolver.decl(funcName, [&context](RealType a, series::DataSeries<RealType> *b, series::DataSeries<RealType> *c, series::DataSeries<RealType> *d){
        auto op = [a](RealType b, RealType c, RealType d) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *b, *c, *d);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, RealType b, RealType c, RealType d){
        auto op = [b, c, d](RealType a) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>>(context, op, *a);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, RealType b, RealType c, series::DataSeries<RealType> *d){
        auto op = [b, c](RealType a, RealType d) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *a, *d);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, RealType b, series::DataSeries<RealType> *c, RealType d){
        auto op = [b, d](RealType a, RealType c) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *a, *c);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, RealType b, series::DataSeries<RealType> *c, series::DataSeries<RealType> *d){
        auto op = [b](RealType a, RealType c, RealType d) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *a, *c, *d);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, series::DataSeries<RealType> *b, RealType c, RealType d){
        auto op = [c, d](RealType a, RealType b) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *a, *b);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, series::DataSeries<RealType> *b, RealType c, series::DataSeries<RealType> *d){
        auto op = [c](RealType a, RealType b, RealType d) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *a, *b, *d);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, series::DataSeries<RealType> *b, series::DataSeries<RealType> *c, RealType d){
        auto op = [d](RealType a, RealType b, RealType c) {return Operator<RealType>()(a, b, c, d);};
        return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *a, *b, *c);
    });
    resolver.decl(funcName, [&context](series::DataSeries<RealType> *a, series::DataSeries<RealType> *b, series::DataSeries<RealType> *c, series::DataSeries<RealType> *d){
        return new series::ParallelOpSeries<RealType, Operator<RealType>, series::DataSeries<RealType>, series::DataSeries<RealType>, series::DataSeries<RealType>, series::DataSeries<RealType>>(context, Operator<RealType>(), *a, *b, *c, *d);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declQuaternaryOp<FuncTriCond, float>(context, resolver, "tri_cond");
    declQuaternaryOp<FuncTriCond, double>(context, resolver, "tri_cond");
});
