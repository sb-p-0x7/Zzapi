cmake_minimum_required(VERSION 3.20)
project(PizzaFactory VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Dependencies via FetchContent (auto-downloaded - no separate install needed)
# =============================================================================
include(FetchContent)

# --- GLFW (window creation and input handling) ---
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# --- Dear ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# ImGui source files (core + GLFW/OpenGL3 backends)
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Build ImGui as a static library
add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw)

# =============================================================================
# Platform-specific OpenGL link settings
# =============================================================================
if(APPLE)
    # macOS: use the OpenGL framework
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
    target_link_libraries(imgui_lib PUBLIC
        ${OPENGL_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
    )
    # Suppress macOS OpenGL deprecation warnings
    target_compile_definitions(imgui_lib PUBLIC GL_SILENCE_DEPRECATION)
elseif(WIN32)
    # Windows: link opengl32
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui_lib PUBLIC OpenGL::GL)
else()
    # Linux (optional support)
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui_lib PUBLIC OpenGL::GL)
endif()

# =============================================================================
# Main executable
# =============================================================================
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(${PROJECT_NAME} ${APP_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui_lib)

# MSVC: the sources are UTF-8 (no BOM), so without this they would be misread as the
# system code page (e.g. CP949), corrupting symbol literals or causing compile errors.
# /utf-8 pins both the source and execution encodings to UTF-8. (Not needed on MinGW/Clang/GCC, which default to UTF-8.)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# =============================================================================
# Post-build message
# =============================================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "========================================="
    COMMAND ${CMAKE_COMMAND} -E echo " PizzaFactory build complete!"
    COMMAND ${CMAKE_COMMAND} -E echo " Run: $<TARGET_FILE:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E echo "========================================="
)
