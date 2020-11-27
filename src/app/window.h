#pragma once

#include <memory>
#include <string>

#include "jw_util/baseexception.h"

#include "app/tickercontext.h"
#include "graphics/gl.h"

struct GLFWwindow;

namespace app {

class AppContext;

class Window : public TickerContext::TickableBase<Window> {
public:
    class Exception : public jw_util::BaseException {
        friend class Window;

    private:
        Exception(const std::string &msg)
            : BaseException(msg)
        {}
    };

    struct Dimensions {
        signed int width;
        signed int height;

        bool operator==(const Dimensions &other) const {
            return width == other.width && height == other.height;
        }
        bool operator!=(const Dimensions &other) const {
            return width != other.width || height != other.height;
        }
    };

    struct MouseButtonEvent {
        signed int button;
        signed int action;
        signed int mods;
    };

    struct KeyEvent {
        signed int key;
        signed int scancode;
        signed int action;
        signed int mods;
    };

    struct ScrollEvent {

    };

    struct MousePosition {
        double x;
        double y;
    };

    struct MouseMoveEvent {
        double x;
        double y;
    };

    Window(AppContext &context);
    ~Window();

    Dimensions dimensions;

    void tick(TickerContext &tickerContext);
    void updateFrame();
    void pollEvents();

    bool shouldClose() const;
    bool isKeyPressed(int key) const;
    bool isMouseButtonPressed(int mouseButton) const;
    MousePosition getMousePosition() const;

    void setMouseVisible(bool visible);

    GLFWwindow *getGlfwWindow() {
        return glfwWindow;
    }

private:
    static GLFWwindow *firstWindow;
    GLFWwindow *glfwWindow = nullptr;

    static void errorCallback(int code, const char *str);
    static void mouseMoveCallback(GLFWwindow *glfwWindow, double x, double y);
    static void mouseButtonCallback(GLFWwindow *glfwWindow, int button, int action, int mods);
    static void keyCallback(GLFWwindow *glfwWindow, int key, int scancode, int action, int mods);
};

}
