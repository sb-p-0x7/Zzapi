#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <cstdio>

#include "app.h"

// // =============================================================================
// // Platform-specific Korean font paths
// // =============================================================================
// static const char* GetKoreanFontPath()
// {
// #if defined(__APPLE__)
//     static const char* candidates[] = {
//         "/System/Library/Fonts/Supplemental/AppleSDGothicNeo.ttc",
//         "/System/Library/Fonts/AppleSDGothicNeo.ttc",
//         "/Library/Fonts/AppleGothic.ttf",
//         nullptr
//     };
// #elif defined(_WIN32)
//     static const char* candidates[] = {
//         "C:\\Windows\\Fonts\\malgun.ttf",
//         "C:\\Windows\\Fonts\\gulim.ttc",
//         nullptr
//     };
// #else
//     static const char* candidates[] = {
//         "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
//         "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
//         "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
//         nullptr
//     };
// #endif
//     for (int i = 0; candidates[i] != nullptr; i++)
//     {
//         FILE* f = fopen(candidates[i], "rb");
//         if (f) { fclose(f); return candidates[i]; }
//     }
//     return nullptr;
// }

// =============================================================================
// GLFW error callback
// =============================================================================
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "[GLFW Error %d] %s\n", error, description);
}

// =============================================================================
// main - handles only the pure GLFW/ImGui boilerplate
// =============================================================================
int main(int, char**)
{
    // -- Initialize GLFW --
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        fprintf(stderr, "GLFW init failed!\n");
        return 1;
    }

    // -- OpenGL version hints (platform-specific) --
#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // -- Create the window --
    GLFWwindow* window = glfwCreateWindow(1280, 720, "PizzaFactory", nullptr, nullptr);
    if (window == nullptr)
    {
        fprintf(stderr, "Window creation failed!\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // -- Set up the ImGui context --
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 4.0f;
    style.GrabRounding      = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowPadding     = ImVec2(12, 12);
    style.FramePadding      = ImVec2(8, 4);

    // -- Initialize the ImGui backends --
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // // -- Load a Korean font --
    // {
    //     const char* fontPath = GetKoreanFontPath();
    //     if (fontPath)
    //     {
    //         ImFontConfig fontConfig;
    //         fontConfig.MergeMode  = false;
    //         fontConfig.PixelSnapH = true;

    //         static const ImWchar ranges[] =
    //         {
    //             0x0020, 0x00FF, // Basic Latin + Latin Supplement
    //             0x0100, 0x024F, // Latin Extended-A/B
    //             0x2000, 0x206F, // General Punctuation
    //             0x2100, 0x214F, // Letterlike Symbols
    //             0x2190, 0x21FF, // Arrows
    //             0x2200, 0x22FF, // Mathematical Operators
    //             0x2300, 0x23FF, // Misc Technical
    //             0x2500, 0x257F, // Box Drawing
    //             0x2580, 0x259F, // Block Elements
    //             0x25A0, 0x25FF, // Geometric Shapes
    //             0x2600, 0x26FF, // Misc Symbols
    //             0x2700, 0x27BF, // Dingbats
    //             0x3000, 0x30FF, // CJK Symbols, Hiragana, Katakana
    //             0x3130, 0x318F, // Hangul Compatibility Jamo
    //             0xAC00, 0xD7A3, // Hangul Syllables
    //             0xFF00, 0xFFEF, // Halfwidth and Fullwidth Forms
    //             0,
    //         };

    //         io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, &fontConfig, ranges);
    //         fprintf(stdout, "Loaded font: %s\n", fontPath);
    //     }
    //     else
    //     {
    //         fprintf(stderr, "Font not found; using default font.\n");
    //         io.Fonts->AddFontDefault();
    //     }
    // }

    io.Fonts->AddFontDefault(); // Explicit Font Path 

    // -- Initialize the app --
    App app;
    app.Init();

    // -- Main loop --
    ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED))
        {
            glfwWaitEvents();
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // -- Update the app (MVC rendering) --
        app.Update();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        );
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // -- Cleanup --
    app.Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
