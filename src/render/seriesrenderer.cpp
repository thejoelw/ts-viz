#include "seriesrenderer.h"

#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS
#include "app/window.h"
#endif

namespace render {

SeriesRenderer::SeriesRenderer(app::AppContext &context, const std::string &name)
    : context(context)
    , name(name)
{
#if ENABLE_GRAPHICS
    context.get<app::Window>();
#endif
}

}
