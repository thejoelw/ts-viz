#pragma once

#include "graphics/gl.h"

#include "app/appcontext.h"
#include "app/tickercontext.h"

namespace render {

class ImguiRenderer : public app::TickerContext::TickableBase<ImguiRenderer, app::TickerContext::ScopedCaller<ImguiRenderer>> {
public:
    ImguiRenderer(app::AppContext &context);
    ~ImguiRenderer();

    void tickOpen(app::TickerContext &tickerContext);
    void tickClose(app::TickerContext &tickerContext);

private:
#if ENABLE_GUI
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void charCallback(GLFWwindow *window, unsigned int c);
#endif
};

}
