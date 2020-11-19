#include "program/resolver.h"
#include "series/type/inputseries.h"

#include "defs/INPUT_SERIES_ELEMENT_TYPE.h"

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("input", [&context](const std::string &name){return new series::InputSeries<INPUT_SERIES_ELEMENT_TYPE>(context, name);});
});
