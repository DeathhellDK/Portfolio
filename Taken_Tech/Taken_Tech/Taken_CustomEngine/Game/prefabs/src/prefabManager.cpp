/**
 * @file      prefabManager.cpp
 * @author    Jethro Sung
 * @email     sung.h, w.kyele,t.weiliangterril
 * @co-author Woh Kye Le, Tan Wei Liang Terril
 * @date      2025-11-7
 *
 * @brief     Implementation of the PrefabManager class.
 *
 * This file defines the PrefabManager�s JSON loading logic.
 * It interprets prefab data, supports flexible JSON structures,
 * and populates Prefab objects with mesh, collider, color, and
 * emitter configurations for later instantiation.
 *
 * @version   1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "prefabManager.h"
#include "Core/resourceManager.h"
#include <json.hpp>
#include <fstream>
#include "Input/DebugConsole.hpp"
#include <iomanip>
#include <string>

using json = nlohmann::json;

/**
 * @brief Safely reads a 2D vector from JSON or returns a default value.
 *
 * Accepts loosely formatted arrays and falls back to @p def if missing or invalid.
 *
 * @param j   JSON object containing the key.
 * @param key Field name (e.g., "scale", "offset").
 * @param def Default Vector2 if not found or invalid.
 * @return Parsed Vector2 value.
 */
static inline Vector2 ReadVec2Loose(const json& j, const char* key, Vector2 def = { 0,0 }) {
    if (j.contains(key) && j[key].is_array() && j[key].size() == 2)
        return { j[key][0].get<float>(), j[key][1].get<float>() };
    return def;
}

/**
 * @brief Safely reads a 3D vector from JSON or returns a default value.
 *
 * @param j   JSON object containing the key.
 * @param key Field name (e.g., "color").
 * @param def Default Vector3 if not found or invalid.
 * @return Parsed Vector3 value.
 */
static inline Vector3 ReadVec3Loose(const json& j, const char* key, Vector3 def = { 1,1,1 }) {
    if (j.contains(key) && j[key].is_array() && j[key].size() == 3)
        return { j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>() };
    return def;
}

/**
 * @brief Load all prefabs from a JSON definition file.
 *
 * Supports flexible JSON shapes:
 * - `{ "prefabs": { "Crate": {...}, "Wall": {...} } }`
 * - `{ "Crate": {...}, "Wall": {...} }`
 * - `[ { "name": "Crate", ... }, { "name": "Wall", ... } ]`
 *
 * Each prefab can define:
 * - Mesh index
 * - Scale and color
 * - Collider type and trigger flag
 * - Texture name
 * - Optional particle emitter configuration
 *
 * @param filename Path to the JSON prefab definition file.
 * @return true if at least one prefab was successfully loaded.
 */
bool PrefabManager::LoadPrefabs(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) { DebugConsole::Get().AddFormattedMessage(LogLevel::Warning, "Cannot open prefab file: " + filename, "\n");  return false; }

    json data;
    try { f >> data; }
    catch (const std::exception& e) { auto er = e.what(); std::string convert = er; DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[ERROR] JSON parse failed: " + convert, "\n"); return false; }

    auto load_one = [&](const std::string& name, const json& def) {
        Prefab p;
        p.meshIndex = def.value("meshIndex", 0);
        if (def.contains("scale") && def["scale"].is_array())
            p.scale = { def["scale"][0], def["scale"][1] };
        if (def.contains("color") && def["color"].is_array())
            p.color = { def["color"][0], def["color"][1], def["color"][2] };

        std::string colStr = def.value("colliderType", "Box");
        if (colStr == "Circle") p.collider = ColliderType::Circle;
        else if (colStr == "Triangle") p.collider = ColliderType::Triangle;

        if (def.contains("isTrigger")) {
            if (def["isTrigger"].is_boolean())      p.isTrigger = def["isTrigger"].get<bool>();
            else if (def["isTrigger"].is_number())  p.isTrigger = (def["isTrigger"].get<int>() != 0);
        }

        p.texture = def.value("texture", "");

        if (def.contains("emitter") && def["emitter"].is_object()) {
            const json& em = def["emitter"];
            p.hasEmitter = true;
            p.emitter.enabled = em.value("enabled", true);
            p.emitter.rate = em.value("rate", 80.f);
            p.emitter.particleLife = em.value("particleLife", 0.4f);
            p.emitter.velMin = ReadVec2Loose(em, "velMin", { -40.f,100.f });
            p.emitter.velMax = ReadVec2Loose(em, "velMax", { 40.f,140.f });
            p.emitter.offset = ReadVec2Loose(em, "offset", { 0.f, 0.f });
            p.emitter.sizeStart = em.value("sizeStart", 12.f);
            p.emitter.sizeEnd = em.value("sizeEnd", 3.f);

            // color: [[r,g,b],[r,g,b]]
            if (em.contains("color") && em["color"].is_array() && em["color"].size() == 2) {
                const auto& c0 = em["color"][0];
                const auto& c1 = em["color"][1];
                if (c0.is_array() && c0.size() == 3 && c1.is_array() && c1.size() == 3) {
                    p.emitter.colorStart = { c0[0].get<float>(), c0[1].get<float>(), c0[2].get<float>() };
                    p.emitter.colorEnd = { c1[0].get<float>(), c1[1].get<float>(), c1[2].get<float>() };
                }
            }

            p.emitter.texture = em.value("texture", "");
            p.emitter.meshIndex = em.value("meshIndex", 1);
        }

        if (def.contains("light") && def["light"].is_object()) {
            const json& l = def["light"];
            p.hasLight = true;
            p.light.enabled = l.value("enabled", true);
            if ((l.contains("glow") && l["glow"].is_object()) || (l.contains("source") && l["source"].is_object())) {
                if (l.contains("glow") && l["glow"].is_object()) {
                    const json& g = l["glow"];
                    p.light.glow.enabled = g.value("enabled", true);
                    p.light.glow.radius = g.value("radius", 180.0f);
                    p.light.glow.intensity = g.value("intensity", 1.0f);
                    p.light.glow.opacity = g.value("opacity", 0.35f);
                    p.light.glow.color = ReadVec3Loose(g, "color", { 1.f, 1.f, 1.f });
                    p.light.glow.offset = ReadVec2Loose(g, "offset", { 0.f, 0.f });
                    p.light.glow.softness = g.value("softness", 2.0f);
                }
                if (l.contains("source") && l["source"].is_object()) {
                    const json& s = l["source"];
                    p.light.source.enabled = s.value("enabled", true);
                    p.light.source.radius = s.value("radius", 180.0f);
                    p.light.source.intensity = s.value("intensity", 1.0f);
                    p.light.source.opacity = s.value("opacity", 0.35f);
                    p.light.source.color = ReadVec3Loose(s, "color", { 1.f, 1.f, 1.f });
                    p.light.source.offset = ReadVec2Loose(s, "offset", { 0.f, 0.f });
                    p.light.source.attenuation = s.value("attenuation", 1.0f);
                }
            }
            else {
                const int type = l.value("type", 1);
                const float radius = l.value("radius", 180.0f);
                const float intensity = l.value("intensity", 1.0f);
                const float opacity = l.value("opacity", 0.35f);
                const Vector3 color = ReadVec3Loose(l, "color", { 1.f, 1.f, 1.f });
                const Vector2 offset = ReadVec2Loose(l, "offset", { 0.f, 0.f });
                const float attenuation = l.value("attenuation", 1.0f);
                const float softness = l.value("softness", 2.0f);

                if (type == 0 || type == 2) {
                    p.light.glow.enabled = true;
                    p.light.glow.radius = radius;
                    p.light.glow.intensity = intensity;
                    p.light.glow.opacity = opacity;
                    p.light.glow.color = color;
                    p.light.glow.offset = offset;
                    p.light.glow.softness = softness;
                }
                else {
                    p.light.source.enabled = true;
                    p.light.source.radius = radius;
                    p.light.source.intensity = intensity;
                    p.light.source.opacity = opacity;
                    p.light.source.color = color;
                    p.light.source.offset = offset;
                    p.light.source.attenuation = attenuation;
                }
            }

            if (l.contains("ember") && l["ember"].is_object()) {
                const json& em = l["ember"];
                p.light.ember.enabled = em.value("enabled", false);
                p.light.ember.rate = em.value("rate", 20.0f);
                p.light.ember.particleLife = em.value("particleLife", 0.8f);
                p.light.ember.velMin = ReadVec2Loose(em, "velMin", { -20.f, 30.f });
                p.light.ember.velMax = ReadVec2Loose(em, "velMax", { 20.f, 60.f });
                p.light.ember.colorStart = ReadVec3Loose(em, "colorStart", { 1.0f, 0.8f, 0.5f });
                p.light.ember.colorEnd = ReadVec3Loose(em, "colorEnd", { 0.3f, 0.1f, 0.0f });
                p.light.ember.sizeStart = em.value("sizeStart", 6.0f);
                p.light.ember.sizeEnd = em.value("sizeEnd", 1.0f);
                p.light.ember.additive = em.value("additive", true);
                p.light.ember.offset = ReadVec2Loose(em, "offset", { 0.0f, 0.0f });
            }

            if (l.contains("flicker") && l["flicker"].is_object()) {
                const json& fl = l["flicker"];
                p.light.flicker.enabled = fl.value("enabled", false);
                p.light.flicker.intensityMin = fl.value("intensityMin", 0.8f);
                p.light.flicker.intensityMax = fl.value("intensityMax", 1.0f);
                p.light.flicker.speed = fl.value("speed", 5.0f);
            }
        }

        prefabs[name] = p;
        DebugConsole::Get().Info("[PrefabManager] Loaded prefab: " + name);
        };

    DebugConsole::Get().Info("[PrefabManager] Reading: " + filename);
    size_t before = prefabs.size();

    if (data.is_object() && data.contains("prefabs") && data["prefabs"].is_object()) {
        for (auto& [name, def] : data["prefabs"].items())
            load_one(name, def);
    }
    else if (data.is_object()) {
        // tolerant: accept top-level object of named prefabs
        for (auto& [name, def] : data.items()) {
            if (def.is_object()) load_one(name, def);
        }
    }
    else if (data.is_array()) {
        // tolerant: array of { "name": "...", "def": { ... } } or array of { "name": "...", ... }
        for (auto& e : data) {
            if (e.contains("name") && e["name"].is_string())
                load_one(e["name"].get<std::string>(), e);
        }
    }
    else {
        DebugConsole::Get().Error("[PrefabManager] Unrecognized prefab JSON shape.");
    }

    DebugConsole::Get().Info("[PrefabManager] Loaded " + std::to_string(prefabs.size() - before) + " prefabs (total " + std::to_string(prefabs.size()) + ")");
    return !prefabs.empty();
}

/**
 * @brief Saves the current set of prefabs back to a JSON file.
 *
 * Serializes all loaded prefabs, including their scale, color, collider,
 * texture, and particle emitter settings into the specified file.
 *
 * @param filename Output JSON file path.
 * @return true if the file was written successfully.
 */
bool PrefabManager::SavePrefabs(const std::string& filename) const
{
    json data;
    json prefabsObj = json::object();

    for (const auto& [name, p] : prefabs)
    {
        json def;

        // Basic fields
        def["meshIndex"] = p.meshIndex;
        def["scale"] = { p.scale.x,  p.scale.y };
        def["color"] = { p.color.x,  p.color.y,  p.color.z };

        // Collider type -> string
        std::string colStr = "Box";
        switch (p.collider)
        {
        case ColliderType::Box:      colStr = "Box";      break;
        case ColliderType::Circle:   colStr = "Circle";   break;
        case ColliderType::Triangle: colStr = "Triangle"; break;
        default:                     break;
        }
        def["colliderType"] = colStr;
        def["isTrigger"] = p.isTrigger;

        if (!p.texture.empty())
            def["texture"] = p.texture;

        // Optional emitter
        if (p.hasEmitter)
        {
            json em;
            em["enabled"] = p.emitter.enabled;
            em["rate"] = p.emitter.rate;
            em["particleLife"] = p.emitter.particleLife;
            em["velMin"] = { p.emitter.velMin.x,   p.emitter.velMin.y };
            em["velMax"] = { p.emitter.velMax.x,   p.emitter.velMax.y };
            em["offset"] = { p.emitter.offset.x,   p.emitter.offset.y };
            em["sizeStart"] = p.emitter.sizeStart;
            em["sizeEnd"] = p.emitter.sizeEnd;

            em["color"] = {
                json::array({ p.emitter.colorStart.x, p.emitter.colorStart.y, p.emitter.colorStart.z }),
                json::array({ p.emitter.colorEnd.x,   p.emitter.colorEnd.y,   p.emitter.colorEnd.z   })
            };

            if (!p.emitter.texture.empty())
                em["texture"] = p.emitter.texture;
            em["meshIndex"] = p.emitter.meshIndex;

            def["emitter"] = em;
        }

        // Optional light
        if (p.hasLight)
        {
            json l;
            l["enabled"] = p.light.enabled;

            json g;
            g["enabled"] = p.light.glow.enabled;
            g["radius"] = p.light.glow.radius;
            g["intensity"] = p.light.glow.intensity;
            g["opacity"] = p.light.glow.opacity;
            g["color"] = { p.light.glow.color.x, p.light.glow.color.y, p.light.glow.color.z };
            g["offset"] = { p.light.glow.offset.x, p.light.glow.offset.y };
            g["softness"] = p.light.glow.softness;
            l["glow"] = g;

            json s;
            s["enabled"] = p.light.source.enabled;
            s["radius"] = p.light.source.radius;
            s["intensity"] = p.light.source.intensity;
            s["opacity"] = p.light.source.opacity;
            s["color"] = { p.light.source.color.x, p.light.source.color.y, p.light.source.color.z };
            s["offset"] = { p.light.source.offset.x, p.light.source.offset.y };
            s["attenuation"] = p.light.source.attenuation;
            l["source"] = s;

            json ember;
            ember["enabled"] = p.light.ember.enabled;
            ember["rate"] = p.light.ember.rate;
            ember["particleLife"] = p.light.ember.particleLife;
            ember["velMin"] = { p.light.ember.velMin.x, p.light.ember.velMin.y };
            ember["velMax"] = { p.light.ember.velMax.x, p.light.ember.velMax.y };
            ember["colorStart"] = { p.light.ember.colorStart.x, p.light.ember.colorStart.y, p.light.ember.colorStart.z };
            ember["colorEnd"] = { p.light.ember.colorEnd.x, p.light.ember.colorEnd.y, p.light.ember.colorEnd.z };
            ember["sizeStart"] = p.light.ember.sizeStart;
            ember["sizeEnd"] = p.light.ember.sizeEnd;
            ember["additive"] = p.light.ember.additive;
            ember["offset"] = { p.light.ember.offset.x, p.light.ember.offset.y };
            l["ember"] = ember;

            json flicker;
            flicker["enabled"] = p.light.flicker.enabled;
            flicker["intensityMin"] = p.light.flicker.intensityMin;
            flicker["intensityMax"] = p.light.flicker.intensityMax;
            flicker["speed"] = p.light.flicker.speed;
            l["flicker"] = flicker;

            def["light"] = l;
        }

        prefabsObj[name] = def;
    }

    data["prefabs"] = prefabsObj;

    std::ofstream f(filename);
    if (!f.is_open())
    {
        std::cerr << "[ERROR] Cannot open prefab file for writing: " << filename << "\n";
        return false;
    }

    f << std::setw(2) << data << std::endl;
    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[INFO] Saved ", prefabs.size(), " prefabs to ", filename, "\n");
    return true;
}

/**
 * @brief Retrieve a prefab by its name.
 *
 * Performs a lookup in the internal prefab map and returns the corresponding
 * Prefab pointer. Returns nullptr if no prefab with that name exists.
 *
 * @param name Logical prefab name.
 * @return Pointer to the Prefab definition, or nullptr if not found.
 */
const Prefab* PrefabManager::Get(const std::string& name) const {
    auto it = prefabs.find(name);
    return (it != prefabs.end()) ? &it->second : nullptr;
}

/**
 * @brief Returns a list of all registered prefab names.
 *
 * @return std::vector<std::string> List of names.
 */
std::vector<std::string> PrefabManager::GetNames() const {
    std::vector<std::string> names;
    names.reserve(prefabs.size());
    for (auto const& [name, _] : prefabs) {
        names.push_back(name);
    }
    return names;
}
