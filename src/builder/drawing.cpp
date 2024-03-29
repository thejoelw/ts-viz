#include "program/resolver.h"

#include "render/drawingrenderer.h"

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("drawing", [&context](const std::string &name, bool enabled, program::UncastNumber r, program::UncastNumber g, program::UncastNumber b, program::UncastNumber a) {
        return new render::DrawingRenderer(context, name, enabled, r.value, g.value, b.value, a.value);
    });
});
