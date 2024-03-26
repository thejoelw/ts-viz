#include "program/resolver.h"
#include "program/programmanager.h"
#include "series/dataseries.h"

#include "log.h"

#include "defs/ENABLE_CHUNK_DEBUG.h"

template <typename RealType>
void declInfo(app::AppContext &context, program::Resolver &resolver) {
    (void) context;

    resolver.decl("info", [](const std::string &msg, RealType a) {
        SPDLOG_INFO("Program info: {}: {}", msg, a);
        return a;
    });
}

template <typename RealType>
void declMeta(app::AppContext &context, program::Resolver &resolver) {
    (void) context;

#if ENABLE_CHUNK_DEBUG
    resolver.decl("meta", [](series::DataSeries<RealType> *node, const std::string &name, const std::string &trace) {
        node->addMeta(name, trace);
        return node;
    });
#endif
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declInfo<float>(context, resolver);
    declInfo<double>(context, resolver);
    declInfo<std::int64_t>(context, resolver);

    declMeta<float>(context, resolver);
    declMeta<double>(context, resolver);
});
