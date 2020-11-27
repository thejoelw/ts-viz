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

    static constexpr float margin = 16.0f;
    static int corner = 0;
    ImVec2 window_pos = ImVec2((corner & 1) ? ImGui::GetIO().DisplaySize.x - margin : margin, (corner & 2) ? ImGui::GetIO().DisplaySize.y - margin : margin);
    ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.3f);
    if (ImGui::Begin("Debug", 0, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav)) {
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Mouse Position: (%.1f,%.1f)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Top-left", NULL, corner == 0)) {corner = 0;}
            if (ImGui::MenuItem("Top-right", NULL, corner == 1)) {corner = 1;}
            if (ImGui::MenuItem("Bottom-left", NULL, corner == 2)) {corner = 2;}
            if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) {corner = 3;}
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    /*
    // 1. Show a simple window.
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
    {
        static float f = 0.0f;
        static int counter = 0;
        ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }

    // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
    if (show_demo_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
        ImGui::ShowDemoWindow(&show_demo_window);
    }
    */
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
