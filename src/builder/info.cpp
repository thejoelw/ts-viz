#include "program/resolver.h"
#include "program/programmanager.h"

#include "log.h"

template <typename RealType>
void declInfo(app::AppContext &context, program::Resolver &resolver) {
    (void) context;

    resolver.decl("info", [](const std::string &msg, RealType a) {
        SPDLOG_INFO("Program info: {}: {}", msg, a);
        return a;
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declInfo<float>(context, resolver);
    declInfo<double>(context, resolver);
    declInfo<std::int64_t>(context, resolver);
});
