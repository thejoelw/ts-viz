#pragma once

#include <memory>
#include <string>

#include "jw_util/baseexception.h"

#include "app/tickercontext.h"
#include "graphics/gl.h"
#include "graphics/glm.h"
#include <glm/vec2.hpp>

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

    Window(AppContext &context);
    ~Window();

    Dimensions dimensions;

    void tick(TickerContext &tickerContext);
    void updateFrame();
    void pollEvents();

    bool shouldRender() const;
    bool shouldClose() const;
    bool isKeyPressed(int key) const;
    bool isMouseButtonPressed(int mouseButton) const;
    glm::vec2 getMousePosition() const;

    void setMouseVisible(bool visible);

    GLFWwindow *getGlfwWindow() {
        return glfwWindow;
    }

private:
    static GLFWwindow *firstWindow;
    GLFWwindow *glfwWindow = nullptr;

    bool needsRender = ENABLE_GUI;

    static void errorCallback(int code, const char *str);
    static void charCallback(GLFWwindow *glfwWindow, unsigned int codepoint);
};

}
