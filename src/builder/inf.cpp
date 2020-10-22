#include "program/resolver.h"
#include "series/infcompseries.h"

template <typename RealType>
auto sequence(app::AppContext &context, RealType scale) {
    auto op = [scale](RealType *dst, std::size_t begin, std::size_t end) {
        for (std::size_t i = begin; i < end; i++) {
            dst[i - begin] = i * scale;
        }
    };

    return new series::InfCompSeries<RealType, decltype(op)>(context, op);
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("seq", [&context](float scale){return sequence(context, scale);});
    resolver.decl("seq", [&context](double scale){return sequence(context, scale);});
});
