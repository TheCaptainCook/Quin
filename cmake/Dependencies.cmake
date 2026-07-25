include(FetchContent)

# ------------------------------------------------------------------------------
# 0. Vulkan SDK (Optional — enables real GPU detection)
# ------------------------------------------------------------------------------
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    message(STATUS "Vulkan SDK found: ${Vulkan_LIBRARY}")
else()
    message(STATUS "Vulkan SDK not found — GPU backend will run in simulated mode.")
endif()

# ------------------------------------------------------------------------------
# 1. spdlog (Fast C++ Logging Library)
# ------------------------------------------------------------------------------
message(STATUS "Fetching spdlog...")
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spdlog)

# ------------------------------------------------------------------------------
# 2. Catch2 (C++ Unit Testing Framework)
# ------------------------------------------------------------------------------
if(BUILD_TESTING)
    message(STATUS "Fetching Catch2...")
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# ------------------------------------------------------------------------------
# 3. SDL2 (Cross-platform Windowing & Input)
# ------------------------------------------------------------------------------
message(STATUS "Fetching SDL2...")
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.1
    GIT_SHALLOW    TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# ------------------------------------------------------------------------------
# 4. Dear ImGui (Immediate Mode Graphical User Interface)
# ------------------------------------------------------------------------------
message(STATUS "Fetching Dear ImGui...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# Create an imgui library target since ImGui doesn't supply CMakeLists.txt natively
if(NOT TARGET imgui::imgui)
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )

    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )

    if(TARGET SDL2::SDL2-static)
        target_link_libraries(imgui PUBLIC SDL2::SDL2-static)
    elseif(TARGET SDL2::SDL2)
        target_link_libraries(imgui PUBLIC SDL2::SDL2)
    endif()

    add_library(imgui::imgui ALIAS imgui)
endif()
