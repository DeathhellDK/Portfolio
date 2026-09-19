# Single, consistent source list (lowercase paths)
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/components/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/include/*.h

    ${CMAKE_CURRENT_SOURCE_DIR}/systems/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/systems/include/*.h

    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/include/*.h

    ${CMAKE_CURRENT_SOURCE_DIR}/debugTools/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debugTools/include/*.h

    ${CMAKE_CURRENT_SOURCE_DIR}/prefabs/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/prefabs/include/*.h
)

add_executable(Taken ${GAME_SOURCES} "include/UI/miniMap.h" "src/UI/miniMap.cpp")

if (MSVC)
  target_link_options(Taken PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:/SUBSYSTEM:CONSOLE>
    $<$<OR:$<CONFIG:Release>,$<CONFIG:ReleaseInstaller>,$<CONFIG:MinSizeRel>>:/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup>
  )
endif()

target_include_directories(Taken PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../EngineCore/include
    ${CMAKE_CURRENT_SOURCE_DIR}/prefabs/include
    ${CMAKE_CURRENT_SOURCE_DIR}/components/include
    ${CMAKE_CURRENT_SOURCE_DIR}/systems/include
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/include
    ${CMAKE_CURRENT_SOURCE_DIR}/debugTools/include
)

set(GAME_SOURCE_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")

target_compile_definitions(Taken PRIVATE
    $<$<CONFIG:Debug>:ENABLE_EDITOR=1>
    $<$<CONFIG:RelWithDebInfo>:ENABLE_EDITOR=1>
    $<$<CONFIG:Release>:ENABLE_EDITOR=1>
    $<$<CONFIG:ReleaseInstaller>:ENABLE_EDITOR=0> # installer
    $<$<CONFIG:MinSizeRel>:ENABLE_EDITOR=0>

    # Debug + RelWithDebInfo  load assets directly from source tree
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>:ASSET_ROOT_DIR="${GAME_SOURCE_ASSETS_DIR}">

    # Release + MinSizeRel  load assets from packaged folder next to .exe
    $<$<OR:$<CONFIG:ReleaseInstaller>,$<CONFIG:MinSizeRel>>:ASSET_ROOT_DIR="assets">

    # Shader path
    SHADER_ROOT_DIR="shaders"
)

target_link_libraries(Taken PRIVATE EngineCore)
target_compile_features(Taken PRIVATE cxx_std_17)

# Organize in IDEs
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GAME_SOURCES})

# --- Asset Copy Commands ---
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Assets  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


add_custom_command(TARGET Taken POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Extern/Fmod/bin/fmodL.dll>
    $<$<NOT:$<CONFIG:Debug>>:${CMAKE_SOURCE_DIR}/Extern/Fmod/bin/fmod.dll>
    $<TARGET_FILE_DIR:Taken>
)

set(FREETYPE_DIR ${CMAKE_SOURCE_DIR}/Extern/freetype)
add_custom_command(TARGET Taken POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${FREETYPE_DIR}/bin/freetype.dll
          $<TARGET_FILE_DIR:Taken>/)

add_custom_command(TARGET Taken POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:Taken>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:Taken>/shaders
)

add_custom_command(TARGET Taken POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Scripts
            $<TARGET_FILE_DIR:Taken>/Scripts
)

if (MSVC)
  target_compile_options(Taken PRIVATE /W4 /FS)
else()
  target_compile_options(Taken PRIVATE -Wall -Wextra -Wpedantic)
endif()
