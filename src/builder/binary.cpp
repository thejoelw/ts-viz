#include "program/resolver.h"
#include "series/type/parallelopseries.h"

template <typename RealType> struct FuncMod { RealType operator()(RealType a, RealType b) const { return std::fmod(a, b); } };
template <typename RealType> struct FuncPow { RealType operator()(RealType a, RealType b) const { return std::pow(a, b); } };
template <typename RealType> struct FuncMinimum { RealType operator()(RealType a, RealType b) const { return std::fmin(a, b); } };
template <typename RealType> struct FuncMaximum { RealType operator()(RealType a, RealType b) const { return std::fmax(a, b); } };
template <typename RealType> struct FuncShrink { RealType operator()(RealType a, RealType b) const { return a > 0 ? (a <= b ? 0 : a - b) : a < 0 ? (a >= -b ? 0 : a + b) : a; } };
template <typename RealType> struct FuncClamp { RealType operator()(RealType a, RealType b) const { return std::isnan(a) ? a : std::fmax(-b, std::fmin(a, b)); } };
template <typename RealType> struct FuncNanTo { RealType operator()(RealType a, RealType b) const { return std::isnan(a) ? b : a; } };

template <template <typename> typename Operator>
void declBinaryOp(app::AppContext &context, program::Resolver &resolver, const char *funcName) {
    resolver.decl(funcName, [](float a, float b) -> float { return Operator<float>()(a, b); });
    resolver.decl(funcName, [](float a, double b) -> double { return Operator<double>()(a, b); });
    resolver.decl(funcName, [](double a, float b) -> double { return Operator<double>()(a, b); });
    resolver.decl(funcName, [](double a, double b) -> double { return Operator<double>()(a, b); });
    resolver.decl(funcName, [&context](series::DataSeries<float> *a, float b){
        auto op = [b](float a) {return Operator<float>()(a, b);};
        return new series::ParallelOpSeries<float, decltype(op), series::DataSeries<float>>(context, op, *a);
    });
    resolver.decl(funcName, [&context](series::DataSeries<double> *a, double b){
        auto op = [b](double a) {return Operator<double>()(a, b);};
        return new series::ParallelOpSeries<double, decltype(op), series::DataSeries<double>>(context, op, *a);
    });
    resolver.decl(funcName, [&context](float a, series::DataSeries<float> *b){
        auto op = [a](float b) {return Operator<float>()(a, b);};
        return new series::ParallelOpSeries<float, decltype(op), series::DataSeries<float>>(context, op, *b);
    });
    resolver.decl(funcName, [&context](double a, series::DataSeries<double> *b){
        auto op = [a](double b) {return Operator<double>()(a, b);};
        return new series::ParallelOpSeries<double, decltype(op), series::DataSeries<double>>(context, op, *b);
    });
    resolver.decl(funcName, [&context](series::DataSeries<float> *a, series::DataSeries<float> *b){
        return new series::ParallelOpSeries<float, Operator<float>, series::DataSeries<float>, series::DataSeries<float>>(context, Operator<float>(), *a, *b);
    });
    resolver.decl(funcName, [&context](series::DataSeries<double> *a, series::DataSeries<double> *b){
        return new series::ParallelOpSeries<double, Operator<double>, series::DataSeries<double>, series::DataSeries<double>>(context, Operator<double>(), *a, *b);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declBinaryOp<std::plus>(context, resolver, "add");
    declBinaryOp<std::minus>(context, resolver, "sub");
    declBinaryOp<std::multiplies>(context, resolver, "mul");
    declBinaryOp<std::divides>(context, resolver, "div");
    declBinaryOp<FuncMod>(context, resolver, "mod");
    declBinaryOp<FuncPow>(context, resolver, "pow");
    declBinaryOp<std::less>(context, resolver, "lt");
    declBinaryOp<std::less_equal>(context, resolver, "lte");
    declBinaryOp<std::greater>(context, resolver, "gt");
    declBinaryOp<std::greater_equal>(context, resolver, "gte");
    declBinaryOp<std::equal_to>(context, resolver, "eq");
    declBinaryOp<std::not_equal_to>(context, resolver, "neq");
    declBinaryOp<FuncMinimum>(context, resolver, "min");
    declBinaryOp<FuncMaximum>(context, resolver, "max");
    declBinaryOp<FuncShrink>(context, resolver, "shrink");
    declBinaryOp<FuncClamp>(context, resolver, "clamp");
    declBinaryOp<FuncNanTo>(context, resolver, "nan_to");
});
