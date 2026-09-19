#pragma once
/**
* @file     config.h
* @author   Jethro Sung
* @email    sung.h
* @date     2025-09-29
*
* @brief    Declares the Config structure and configuration loading function.
*
* This file defines a simple configuration structure (`Config`) to hold global
* game settings such as window dimensions, camera parameters, audio levels,
* default mesh colors, and system enable/disable toggles.
* It also declares a function to load configuration data from a JSON file.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include <string>
#include <unordered_map>
#include <vector>
#include "Math/vect2.h"
#include "Math/vect3.h"

/**
 * @struct Config
 * @brief Holds global configuration settings for the application.
 *
 * The Config struct contains settings for:
 * - **Window**: width and height
 * - **Camera**: zoom factor and camera position
 * - **Audio**: master/music/sfx volume levels (in decibels)
 * - **Rendering**: default per-vertex colors for meshes
 * - **System toggles**: enable or disable specific systems such as movement, collision, rendering, and animation
 */
struct Config {
    // Window settings
    int width = 1280;
    int height = 720;
	bool fullscreen = true;

    // Camera settings
    float zoom = 1.0f;
    Vector2 camPos = { 0.f, 0.f };

    // --- Audio settings ---
    float masterVolume = 0.0f;
    float musicVolume = -6.0f;
    float sfxVolume = 0.0f;

    // sound registry + default background music name
    std::unordered_map<std::string, std::string> sounds;
    std::string bgmName;
    

    // Default colors for meshes
    std::vector<Vector3> colors;

    // System toggles
    bool enableMovement = true;
    bool enableCollision = true;
    bool enableRender = true;
    bool enableAnimation = true;
    bool enableEmitter = true;
    bool enableLight = true;
};

/**
 * @brief Loads configuration data from a JSON file into a Config structure.
 *
 * The JSON file should follow this general format:
 * @code
 * {
 *   "window": {
 *     "width": 1400,
 *     "height": 700
 *   },
 *   "audio": {
 *     "masterVolume": 0.8,
 *     "musicVolume": -6.0,
 *     "sfxVolume": 0.0
 *   },
 *   "camera": {
 *     "zoom": 1.0,
 *     "pos": [0, 0]
 *   },
 *   "systems": {
 *     "movement": true,
 *     "collision": true,
 *     "render": true,
 *     "animation": true
 *   }
 * }
 * @endcode
 *
 * @param[out] cfg       Reference to a Config struct that will be filled with loaded data.
 * @param[in]  filename  Path to the JSON configuration file.
 * @return true if the file was loaded successfully, false otherwise.
 */
bool LoadConfig(Config& cfg, const std::string& filename);