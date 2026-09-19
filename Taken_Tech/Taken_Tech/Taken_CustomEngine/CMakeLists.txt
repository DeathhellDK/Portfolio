# ==============================================================
# Project Settings
# ==============================================================
cmake_minimum_required(VERSION 3.15)
project(gam-proj)

# --- Multi-config setups (MSVC / Visual Studio) ---
if (MSVC)
    # Add a custom config for installer builds
    set(CMAKE_CONFIGURATION_TYPES
        "Debug;Release;RelWithDebInfo;MinSizeRel;ReleaseInstaller"
        CACHE STRING "" FORCE)

    # Make sure all the per-config flags exist for ReleaseInstaller,
    # by copying them from the normal Release config.
    foreach(VAR
        CMAKE_C_FLAGS
        CMAKE_CXX_FLAGS
        CMAKE_EXE_LINKER_FLAGS
        CMAKE_SHARED_LINKER_FLAGS
        CMAKE_MODULE_LINKER_FLAGS
        CMAKE_STATIC_LINKER_FLAGS
    )
        if (DEFINED ${VAR}_RELEASE AND NOT DEFINED ${VAR}_RELEASEINSTALLER)
            set(${VAR}_RELEASEINSTALLER "${${VAR}_RELEASE}" CACHE STRING "" FORCE)
        endif()
    endforeach()
endif()

# --- Output directories for all build configurations ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
foreach(OUTPUTCONFIG DEBUG RELEASE RELWITHDEBINFO MINSIZEREL RELEASEINSTALLER)
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_SOURCE_DIR}/bin/${OUTPUTCONFIG})
endforeach()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================
# External Dependencies
# ==============================================================

# GLFW
add_subdirectory(Extern/glfw EXCLUDE_FROM_ALL)

# GLAD
add_library(glad Extern/glad/src/glad.c)
target_include_directories(glad PUBLIC Extern/glad/include)

# stb (header-only)
include_directories(Extern/stb)

# nlohmann
include_directories(${CMAKE_SOURCE_DIR}/Extern/nlohmann)

# ImGui (built once here)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Extern/imgui)

# ==============================================================
# Lua (embedded scripting)
# ==============================================================

file(GLOB LUA_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/Extern/lua/src/*.c
)

# Remove the standalone executables (they contain main())
list(REMOVE_ITEM LUA_SOURCES
    ${CMAKE_SOURCE_DIR}/Extern/lua/src/lua.c
    ${CMAKE_SOURCE_DIR}/Extern/lua/src/luac.c
)

add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${CMAKE_SOURCE_DIR}/Extern/lua/src)

if (MSVC)
    target_compile_definitions(lua PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()


add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC glfw glad)

if (MSVC)
    target_compile_options(imgui PRIVATE /W4)
else()
    target_compile_options(imgui PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ==============================================================
# Project Targets
# ==============================================================

# EngineCore library
add_subdirectory(EngineCore)

# Game executable
add_subdirectory(Game)