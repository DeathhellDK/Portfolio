/**
 * @file     config.cpp
 * @author   Jethro Sung
 * @email    sung.h,t.weiliangterril
 * @co-author Tan Wei Liang Terril
 * @date     2025-09-29
 *
 * @brief    Implements configuration file loading from JSON into the Config struct.
 *
 * This file provides parsing utilities and the implementation of the
 * `LoadConfig` function, which reads game configuration data from a JSON file.
 * Supported fields include window size, camera parameters, audio levels,
 * mesh colors, system toggles, and sound file loading via ResourceManager.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "Core/config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <algorithm>
#include "Core/resourceManager.h"
#include <json.hpp>
#include "Input/DebugConsole.hpp"
using json = nlohmann::json;

/**
 * @brief Loads configuration settings from a JSON file into the given Config structure.
 *
 * This function reads a JSON file line by line, matching key names and filling in fields of the
 * provided `Config` structure. It also loads audio files referenced by `"sound_*"` keys through
 * the `ResourceManager`.
 *
 * Supported JSON keys include:
 * - `"width"`, `"height"` (window size)
 * - `"zoom"`, `"pos"` (camera zoom and position)
 * - `"masterVolume"`, `"musicVolume"`, `"sfxVolume"` (audio levels)
 * - `"movement"`, `"collision"`, `"render"`, `"animation"` (system toggles)
 * - Arrays representing mesh colors
 * - `"sound_<name>"` for loading sound assets.
 *
 * @param[out] cfg       Reference to a Config struct that will be filled.
 * @param[in]  filename  Path to the JSON configuration file.
 * @return true if the configuration file was successfully opened and parsed, false otherwise.
 */
bool LoadConfig(Config& cfg, const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) {
        DebugConsole::Get().Error("Failed to open config file: " + filename + "\n");
        return false;
    }

    json j;
    try {
        f >> j;
    }
    catch (const std::exception& e) {
        DebugConsole::Get().Error("JSON parse error in " + filename + ": " + e.what() + "\n");
        return false;
    }

    // --- Window ---
    if (j.contains("window")) {
        auto& w = j["window"];
        cfg.width = w.value("width", 1280);
        cfg.height = w.value("height", 720);
        cfg.fullscreen = w.value("fullscreen", true);
    }

    // --- Camera ---
    if (j.contains("camera")) {
        auto& c = j["camera"];
        cfg.zoom = c.value("zoom", 1.0f);
        if (c.contains("pos"))
            cfg.camPos = { c["pos"][0].get<float>(), c["pos"][1].get<float>() };
    }

    // --- Audio ---
    if (j.contains("audio")) {
        auto& a = j["audio"];
        cfg.masterVolume = a.value("masterVolume", 0.0f);
        cfg.musicVolume = a.value("musicVolume", 0.0f);
        cfg.sfxVolume = a.value("sfxVolume", 0.0f);

        // Handle single BGM string field
        if (a.contains("sound_bgm") && a["sound_bgm"].is_string()) {
            cfg.bgmName = a["sound_bgm"].get<std::string>();
            cfg.sounds["bgm"] = "Assets/" + cfg.bgmName;
        }

        // Optionally load all "sound_*" keys automatically
        for (auto& [key, val] : a.items()) {
            if (key.rfind("sound_", 0) == 0 && val.is_string())
                cfg.sounds[key] = "Assets/" + val.get<std::string>();
        }
    }

    // --- Colors ---
    if (j.contains("colors")) {
        for (auto& c : j["colors"]) {
            if (c.size() == 3)
                cfg.colors.emplace_back(c[0], c[1], c[2]);
        }
    }

    // --- Systems ---
    if (j.contains("systems")) {
        auto& s = j["systems"];
        cfg.enableMovement = s.value("movement", true);
        cfg.enableCollision = s.value("collision", true);
        cfg.enableRender = s.value("render", true);
        cfg.enableAnimation = s.value("animation", true);
        cfg.enableEmitter = s.value("emitter", true);
        cfg.enableLight = s.value("light", true);
    }
    return true;
}