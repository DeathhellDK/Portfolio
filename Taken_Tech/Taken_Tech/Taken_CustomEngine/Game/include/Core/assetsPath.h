    #pragma once
/**
* @file     assetsPath.h
* @author   Woh Kye Le
* @email    w.kyele
* @date     2025-11-15
*
* @brief     Utility helpers for resolving absolute asset and shader paths
* 
* 
* This header provides two lightweight helper functions, `AssetPath` and
* `ShaderPath`, which expand a relative path (e.g., `"scene/level01.json"`)
* into a full OS path using compile-time root directories defined by CMake.
*
* These helpers allow the engine to load JSON files, textures, and shaders
* directly from the source tree without requiring post-build copies or
* symbolic links. When CMake does not define `ASSET_ROOT_DIR` or
* `SHADER_ROOT_DIR`, the functions fall back to `"Assets"` and `"shaders"`
* 
* e.g LoadConfig(cfg, AssetPath("config.json"));
* shader.LoadFromFile(
*     ShaderPath("default.vert"),
*     ShaderPath("default.frag")
* );
*
* @version 1.0
* @copyright
* Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include <string>

// Fallbacks if someone builds without the CMake definitions
#ifndef ASSET_ROOT_DIR
#define ASSET_ROOT_DIR "assets"
#endif

#ifndef SHADER_ROOT_DIR
#define SHADER_ROOT_DIR "shaders"
#endif

/**
 * @brief Builds an absolute path to a file inside the asset directory.
 *
 * @param rel Relative path inside the Assets folder
 *            (e.g., `"scene/labyrinth.json"`).
 * @return Full concatenated path (e.g.,
 *         `"C:/.../Game/Assets/scene/labyrinth.json"`).
 */
inline std::string AssetPath(const std::string& rel){
    // Example: rel = "scene/labyrinth.json"
    // -> "C:/.../Game/Assets/scene/labyrinth.json"
    return std::string(ASSET_ROOT_DIR) + "/" + rel;
}


/**
 * @brief Builds an absolute path to a file inside the shader directory.
 *
 * @param rel Relative shader filename (e.g., `"shader.vert"`).
 * @return Full concatenated path to the shader file.
 */
inline std::string ShaderPath(const std::string& rel){
    // Example: "shader.vert"
    // -> "C:/.../Game/shaders/shader.vert"
    return std::string(SHADER_ROOT_DIR) + "/" + rel;
}