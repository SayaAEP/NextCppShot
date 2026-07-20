cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(NextCppShot VERSION 0.7.0)

IF( MSVC )
    SET ( CMAKE_EXE_LINKER_FLAGS /MANIFEST:NO )
ENDIF()

add_definitions(-DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0500 -D_WIN32_IE=0x0300)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    IF( MSVC )
        add_compile_options(/W4)
        add_link_options(/OPT:REF /OPT:ICF /LTCG /INCREMENTAL:NO /SUBSYSTEM:WINDOWS)
    ELSE()
        add_compile_options(-O2 -s -mwindows -Wall)
    ENDIF()
endif()

# =============================================================================
# Dear ImGui Configuration
# =============================================================================
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui")

# ImGui core sources
set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
)

# ImGui backend sources (all 7 renderers)
set(IMGUI_BACKEND_SOURCES
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx9.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx10.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx12.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    # "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)

# =============================================================================
# Main Executable Configuration
# =============================================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/resources.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/res/resources.rc
    @ONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/src/version.h
    @ONLY)

# Source files - src/ only, exclude gui/ (handled separately)
file(GLOB_RECURSE SOURCES
    src/*.cpp
    res/*.rc
)

# Exclude the gui/ folder from the glob (handled explicitly below)
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/.*")

# Explicit GUI sources — guarantees correct build order and prevents Glob issues
set(GUI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Gui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/renderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/renderer_dx9.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/renderer_dx10.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/renderer_dx11.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/renderer_dx12.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/renderer_opengl2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/renderer_opengl3.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/renderer_vulkan.cpp"
)
list(APPEND SOURCES ${GUI_SOURCES})

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/res/resources.rc)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src)
target_include_directories(${PROJECT_NAME} PRIVATE "${IMGUI_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${IMGUI_DIR}/backends")

# Add ImGui sources directly to target
target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})

IF( DEFINED LINK_UNICOWS )
    target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/lib/libunicows.a)
ENDIF()

# Link Windows + all DX (9/10/11/12) + OpenGL libraries
# Vulkan is reserved for future use (stub renderer doesn't link to vulkan-1 yet)
target_link_libraries(${PROJECT_NAME} gdiplus advapi32 gdi32 user32 kernel32 comctl32
                                            d3d9 d3d10 d3d11 d3d12 dxgi winmm opengl32 glu32)

set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

IF( MSVC )
    SET ( CMAKE_EXE_LINKER_FLAGS /MANIFEST:NO )
ELSE()
    target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
ENDIF()
