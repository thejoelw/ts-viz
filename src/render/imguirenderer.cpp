#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS

#include "imguirenderer.h"

#include "graphics/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"

#include "app/window.h"

namespace render {

ImguiRenderer::ImguiRenderer(app::AppContext &context)
    : TickableBase(context)
{
#if ENABLE_GUI
    GLFWwindow *glfwWindow = context.get<app::Window>().getGlfwWindow();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, false);

    prevMouseButtonCallback = glfwSetMouseButtonCallback(glfwWindow, mouseButtonCallback);
    prevScrollCallback = glfwSetScrollCallback(glfwWindow, scrollCallback);
    prevKeyCallback = glfwSetKeyCallback(glfwWindow, keyCallback);
    prevCharCallback = glfwSetCharCallback(glfwWindow, charCallback);

    ImGui_ImplOpenGL3_Init("#version 150");

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
#endif
}

ImguiRenderer::~ImguiRenderer() {
#if ENABLE_GUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif
}

void ImguiRenderer::tickOpen(app::TickerContext &tickerContext) {
#if ENABLE_GUI
    tickerContext.get<app::Window::Ticker>();

    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
#endif
}

void ImguiRenderer::tickClose(app::TickerContext &tickerContext) {
#if ENABLE_GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}

#if ENABLE_GUI
void ImguiRenderer::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (prevMouseButtonCallback && !ImGui::GetIO().WantCaptureMouse) {
        prevMouseButtonCallback(window, button, action, mods);
    }
}

void ImguiRenderer::scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (prevScrollCallback) {
        prevScrollCallback(window, xoffset, yoffset);
    }
}

void ImguiRenderer::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (prevKeyCallback && !ImGui::GetIO().WantCaptureKeyboard) {
        prevKeyCallback(window, key, scancode, action, mods);
    }
}

void ImguiRenderer::charCallback(GLFWwindow *window, unsigned int c) {
    ImGui_ImplGlfw_CharCallback(window, c);
    if (prevCharCallback && !ImGui::GetIO().WantCaptureKeyboard) {
        prevCharCallback(window, c);
    }
}

GLFWmousebuttonfun ImguiRenderer::prevMouseButtonCallback;
GLFWscrollfun ImguiRenderer::prevScrollCallback;
GLFWkeyfun ImguiRenderer::prevKeyCallback;
GLFWcharfun ImguiRenderer::prevCharCallback;
#endif

}

#endif
