# ==============================================================
# EngineCore Static Library
# ==============================================================
file(GLOB_RECURSE ENGINECORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

add_library(EngineCore STATIC ${ENGINECORE_SOURCES} "include/Core/componentcontext.h" "include/Editor/editor.h" "src/Editor/editor.cpp" "include/Core/persistentTag.h" "src/Physics/BroadCollision.cpp" "src/Physics/EntityForceProxy.cpp")

# --- Include Paths ---
target_include_directories(EngineCore
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Extern/glfw/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Extern/glm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Extern/stb
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Extern/Fmod/include
)

# Set ENABLE_EDITOR macro based on build config
target_compile_definitions(EngineCore PUBLIC
    $<$<CONFIG:Debug>:ENABLE_EDITOR=1>
    $<$<CONFIG:RelWithDebInfo>:ENABLE_EDITOR=1>
    $<$<CONFIG:Release>:ENABLE_EDITOR=1>
    $<$<CONFIG:ReleaseInstaller>:ENABLE_EDITOR=0> # installer
    $<$<CONFIG:MinSizeRel>:ENABLE_EDITOR=0>
)

# ImGuizmo only in editor-enabled configs
target_include_directories(EngineCore PUBLIC
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>:${CMAKE_SOURCE_DIR}/Extern/ImGuizmo>
)

target_sources(EngineCore PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>:${CMAKE_SOURCE_DIR}/Extern/ImGuizmo/ImGuizmo.cpp>
)

# --- Link Dependencies ---
target_link_libraries(EngineCore    
    PUBLIC glad
    PUBLIC lua
    PRIVATE glfw
    PRIVATE
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Extern/Fmod/lib/fmodL_vc.lib>
        $<$<NOT:$<CONFIG:Debug>>:${CMAKE_SOURCE_DIR}/Extern/Fmod/lib/fmod_vc.lib>
    # Link imgui only for editor builds
    PUBLIC
        $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>:imgui>
)

# --- FreeType Integration ---
set(FREETYPE_DIR ${CMAKE_SOURCE_DIR}/Extern/freetype)

target_include_directories(EngineCore PUBLIC
    ${FREETYPE_DIR}/include
)

target_link_directories(EngineCore PRIVATE
    ${FREETYPE_DIR}/lib
)

target_link_libraries(EngineCore PRIVATE
    ${FREETYPE_DIR}/lib/freetype.lib
)

# --- Compiler Warnings ---
if (MSVC)
    target_compile_options(EngineCore PRIVATE /W4)
else()
    target_compile_options(EngineCore PRIVATE -Wall -Wextra -Wpedantic)
endif()