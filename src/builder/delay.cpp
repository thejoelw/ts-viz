#include "program/resolver.h"
#include "series/type/delayedseries.h"

template <typename RealType>
void declDelay(app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("delay", [&context](series::DataSeries<RealType> *a, std::int64_t delay){
        return new series::DelayedSeries<RealType>(context, *a, delay);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declDelay<float>(context, resolver);
    declDelay<double>(context, resolver);
});
