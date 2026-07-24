#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include "core/logging.hpp"
#include "gui/debug_shell.hpp"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // 1. Initialize Logging Framework
    quin::core::init_logging();
    QUIN_LOG_INFO("Starting Quin PS5 Emulator Debug Shell...");

    // 2. Initialize SDL2 Windowing & Graphics Subsystem
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        QUIN_LOG_CRITICAL("Failed to initialize SDL2: {}", SDL_GetError());
        return -1;
    }

    // Configure OpenGL 3.3 Core Profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_OPENGL | 
        SDL_WINDOW_RESIZABLE | 
        SDL_WINDOW_ALLOW_HIGHDPI
    );

    SDL_Window* window = SDL_CreateWindow(
        "Quin PS5 Emulator — Phase 0 Debug Shell",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        window_flags
    );

    if (!window) {
        QUIN_LOG_CRITICAL("Failed to create SDL2 Window: {}", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        QUIN_LOG_CRITICAL("Failed to create OpenGL Context: {}", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable VSync

    QUIN_LOG_INFO("Graphics Context created successfully.");

    // 3. Initialize Dear ImGui Context & Backends
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCKING
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    // Apply Dark Modern Theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    QUIN_LOG_INFO("Dear ImGui initialized successfully.");

    // 4. Instantiate Debug Shell UI
    quin::gui::DebugShell debug_shell;

    // 5. Main Application Event & Render Loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                running = false;
            }
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Render Debug Shell Interface
        debug_shell.render();

        // Rendering Pipeline
        ImGui::Render();
        int display_w, display_h;
        SDL_GL_GetDrawableSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.10f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    QUIN_LOG_INFO("Shutting down Quin Debug Shell...");

    // 6. Clean Shutdown Sequence
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    QUIN_LOG_INFO("Shutdown complete.");
    return 0;
}
