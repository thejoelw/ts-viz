#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS

#include "window.h"

#include <unistd.h>
#include "log.h"

#include "app/appcontext.h"
#include "app/tickercontext.h"
#include "app/quitexception.h"
#include "app/options.h"
#include "stream/drawingmanager.h"

namespace app {

Window::Window(AppContext &context)
    : TickableBase(context)
{
    dimensions.width = 0;
    dimensions.height = 0;

#if ENABLE_GUI
    SPDLOG_DEBUG("GLFW version: {}", glfwGetVersionString());

    glfwSetErrorCallback(&Window::errorCallback);

    // Initialize GLFW
    if (!glfwInit()) {
        throw Exception("glfwInit() error");
    }

#ifdef WINDOW_GLFW_DEBUG_CONTEXT
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create GLFW window
    // http://www.glfw.org/docs/latest/window.html
    glfwWindow = glfwCreateWindow(static_cast<int>(800), static_cast<int>(600), app::Options::getInstance().title.data(), nullptr, firstWindow);
    if (!glfwWindow) {
        throw Exception("glfwCreateWindow() error");
    }

    if (!firstWindow) {
        firstWindow = glfwWindow;
    }

    glfwSetWindowUserPointer(glfwWindow, this);
    glfwMakeContextCurrent(glfwWindow);

    // GLEW causes a GL_INVALID_ENUM error if this isn't set:
    glewExperimental = GL_TRUE;

    // Initialize GLEW
    GLenum glew_init = glewInit();
    if (glew_init != GLEW_OK) {
        throw Exception(fmt::format("glfwInit() error: {}", glewGetErrorString(glew_init)));
    }

    // For some reason, glewInit() causes a GL_INVALID_ENUM error.
    // https://www.opengl.org/wiki/OpenGL_Loading_Library
    // Apparently it's harmless, but this is to clear it.
    glGetError();

    SPDLOG_DEBUG("GLEW version: {}", glewGetString(GLEW_VERSION));
    SPDLOG_DEBUG("OpenGL version: {}", glGetString(GL_VERSION));

    // Set GLFW options
    glfwSetInputMode(glfwWindow, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(glfwWindow, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);
    setMouseVisible(true);

    glfwSetCharCallback(glfwWindow, &Window::charCallback);
#endif
}

Window::~Window() {
#if ENABLE_GUI
    // Cursor options:
    // GLFW_CURSOR_NORMAL, GLFW_CURSOR_HIDDEN, or GLFW_CURSOR_DISABLED
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (glfwWindow) {
        glfwDestroyWindow(glfwWindow);
    }
    glfwTerminate();
#endif
}

void Window::tick(TickerContext &tickerContext) {
    (void) tickerContext;

    updateFrame();
    pollEvents();
}

void Window::updateFrame() {
    if (shouldRender()) {
        glFlush();
#if ENABLE_GUI
        glfwSwapBuffers(glfwWindow);
#endif
    }
}

void Window::pollEvents() {
#if ENABLE_GUI
    glfwPollEvents();

    needsRender = glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED);

    if (shouldRender()) {
        Dimensions newDims;
        glfwGetFramebufferSize(glfwWindow, &newDims.width, &newDims.height);

        if (newDims != dimensions) {
            SPDLOG_DEBUG("Resizing to {} x {}", newDims.width, newDims.height);
            glViewport(0, 0, newDims.width, newDims.height);

            dimensions = newDims;
        }
    }
#endif

    if (shouldClose()) {
        throw QuitException();
    }
}

bool Window::shouldRender() const {
    return needsRender;
}

bool Window::shouldClose() const {
#if ENABLE_GUI
    return glfwWindowShouldClose(glfwWindow);
#else
    return false;
#endif
}

bool Window::isKeyPressed(int key) const {
#if ENABLE_GUI
    return glfwGetKey(glfwWindow, key) == GLFW_PRESS;
#else
    return false;
#endif
}

bool Window::isMouseButtonPressed(int mouseButton) const {
#if ENABLE_GUI
    return glfwGetMouseButton(glfwWindow, mouseButton) == GLFW_PRESS;
#else
    return false;
#endif
}

glm::vec2 Window::getMousePosition() const {
#if ENABLE_GUI
    double x;
    double y;
    glfwGetCursorPos(glfwWindow, &x, &y);
    return glm::vec2(4.0 * x / dimensions.width - 1.0, 4.0 * y / dimensions.height - 1.0);
#else
    return glm::vec2(0.0f, 0.0f);
#endif
}

void Window::setMouseVisible(bool visible) {
#if ENABLE_GUI
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
#endif
}

void Window::errorCallback(int code, const char *str) {
    // We don't have a window reference here, so just call a singleton logger

#if ENABLE_GUI
    switch (code)
    {
    case GLFW_NOT_INITIALIZED:
        SPDLOG_ERROR("GLFW has not been initialized.");
        break;
    case GLFW_NO_CURRENT_CONTEXT:
        SPDLOG_ERROR("No context is current for this thread.");
        break;
    case GLFW_INVALID_ENUM:
        SPDLOG_ERROR("One of the enum parameters for the function was given an invalid enum.");
        break;
    case GLFW_INVALID_VALUE:
        SPDLOG_ERROR("One of the parameters for the function was given an invalid value.");
        break;
    case GLFW_OUT_OF_MEMORY:
        SPDLOG_ERROR("A memory allocation failed.");
        break;
    case GLFW_API_UNAVAILABLE:
        SPDLOG_ERROR("GLFW could not find support for the requested client API on the system.");
        break;
    case GLFW_VERSION_UNAVAILABLE:
        SPDLOG_ERROR("The requested client API version is not available.");
        break;
    case GLFW_PLATFORM_ERROR:
        SPDLOG_ERROR("A platform-specific error occurred that does not match any of the more specific categories.");
        break;
    case GLFW_FORMAT_UNAVAILABLE:
        SPDLOG_ERROR("The clipboard did not contain data in the requested format.");
        break;
    }
#endif

    SPDLOG_ERROR("GLFW error {}: {}", code, str);
}

void Window::charCallback(GLFWwindow *glfwWindow, unsigned int codepoint) {
    if (codepoint >= 0x20 && codepoint <= 0x7E) {
        Window &window = *static_cast<Window *>(glfwGetWindowUserPointer(glfwWindow));
        window.context.get<stream::DrawingManager>().resetDrawing(codepoint);
    }
}

GLFWwindow *Window::firstWindow = nullptr;

}

#endif
