#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS

#include "seriesrenderer.h"

#include "app/window.h"

namespace render {

SeriesRenderer::SeriesRenderer(app::AppContext &context, const std::string &name)
    : context(context)
    , name(name)
{
    context.get<app::Window>();
}

}

#endif
