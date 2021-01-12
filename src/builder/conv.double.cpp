#include "conv.h"

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declConv<double>(context, resolver);
});
