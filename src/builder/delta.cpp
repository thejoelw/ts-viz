#include "program/resolver.h"
#include "series/type/deltaseries.h"

template <template <typename> typename Operator>
void declDeltaOp(app::AppContext &context, program::Resolver &resolver, const char *funcName) {
    resolver.decl(funcName, [&context](series::DataSeries<float> *a){
        return new series::DeltaSeries<float, Operator<float>, series::DataSeries<float> &>(context, Operator<float>(), *a);
    });
    resolver.decl(funcName, [&context](series::DataSeries<double> *a){
        return new series::DeltaSeries<double, Operator<double>, series::DataSeries<double> &>(context, Operator<double>(), *a);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declDeltaOp<std::minus>(context, resolver, "sub_delta");
    declDeltaOp<std::divides>(context, resolver, "div_delta");
});
