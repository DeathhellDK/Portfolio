
/**
 * @file       loader.cpp
 * @author     Jethro Sung
 * @co_author  Low JianLin,Sng Swee Yong Dillon, Tan Wei Liang Terril
 * @email      sung.h, jianlin.low,sweeyongdillon.sng,t.weiliangterril
 * @date       2025-09-29
 *
 * @brief    Implements entity loading from a JSON-like file into the GameApp.
 *
 * The loader runs during:
 *  - Startup (global assets)
 *  - Scene transitions (labyrinth → level)
 *  - Editor "Play" mode state restoration
 *
 * Load rules summary:
 *  Transforms apply world-space values unless `"local"` is provided
 *  Player entity is unique and persisted
 *  Missing MeshRenderer/Animator fields fall back to defaults
 *  Skips unknown component types gracefully
 *
 * @version 1.0
 * @copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "Core/loader.h"
#include "Core/gameApp.h"
#include "factories.h"
#include "World/doorsystem.h"
#include "World/mapGenerator.h"
#include "Physics/EntityForceProxy.hpp"
#include "Physics/ForceSystem.hpp"
#include "Core/assetsPath.h"
#include "Input/DebugConsole.hpp"
#include "Core/componentcontext.h" 
#include "Light/lightComponent.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {
    /**
     * @brief Compare two floats with an absolute epsilon.
     * @param a First value.
     * @param b Second value.
     * @param eps Absolute tolerance.
     * @return true if the values are within @p eps; false otherwise.
     */
    bool AlmostEqual(float a, float b, float eps = 1e-4f) {
        return std::fabs(a - b) <= eps;
    }

    /**
     * @brief Compare two Vector2 values with an absolute epsilon per component.
     * @param a First vector.
     * @param b Second vector.
     * @param eps Absolute tolerance.
     * @return true if both components are within @p eps; false otherwise.
     */
    bool AlmostEqualVec2(const Vector2& a, const Vector2& b, float eps = 1e-4f) {
        return AlmostEqual(a.x, b.x, eps) && AlmostEqual(a.y, b.y, eps);
    }

    /**
     * @brief Compare two Vector3 values with an absolute epsilon per component.
     * @param a First vector.
     * @param b Second vector.
     * @param eps Absolute tolerance.
     * @return true if all components are within @p eps; false otherwise.
     */
    bool AlmostEqualVec3(const Vector3& a, const Vector3& b, float eps = 1e-4f) {
        return AlmostEqual(a.x, b.x, eps) &&
            AlmostEqual(a.y, b.y, eps) &&
            AlmostEqual(a.z, b.z, eps);
    }

    /**
     * @brief Internal light record representation used for sidecar JSON.
     *
     * This mirrors the LightEffects/LightRecord schema used by the editor-side
     * tooling, and is used to deserialize/serialize `Assets/scene/<stem>_lights.json`.
     */
    struct LightJsonRecord_ {
        std::string name;
        std::string prefabTag;
        Vector2 position{ 0.0f, 0.0f };

        bool enabled = true;

        bool glowEnabled = true;
        float glowRadius = 180.0f;
        Vector3 glowColor{ 1.0f, 0.85f, 0.55f };
        Vector2 glowOffset{ 0.0f, 0.0f };
        float glowIntensity = 1.0f;
        float glowOpacity = 0.35f;
        float glowSoftness = 2.0f;

        bool sourceEnabled = false;
        float sourceRadius = 180.0f;
        Vector3 sourceColor{ 1.0f, 0.85f, 0.55f };
        Vector2 sourceOffset{ 0.0f, 0.0f };
        float sourceIntensity = 1.0f;
        float sourceOpacity = 0.35f;
        float sourceAttenuation = 1.0f;

        bool emberEnabled = false;
        float emberRate = 20.0f;
        float emberParticleLife = 0.8f;
        Vector2 emberVelMin{ -20.0f, 30.0f };
        Vector2 emberVelMax{ 20.0f, 60.0f };
        Vector3 emberColorStart{ 1.0f, 0.8f, 0.5f };
        Vector3 emberColorEnd{ 0.3f, 0.1f, 0.0f };
        float emberSizeStart = 6.0f;
        float emberSizeEnd = 1.0f;
        bool emberAdditive = true;
        Vector2 emberOffset{ 0.0f, 0.0f };

        bool flickerEnabled = false;
        float flickerIntensityMin = 0.8f;
        float flickerIntensityMax = 1.0f;
        float flickerSpeed = 5.0f;
        float flickerTime = 0.0f;
    };

    /**
     * @brief Compute squared distance between two positions.
     * @param a First position.
     * @param b Second position.
     * @return Squared Euclidean distance.
     */
    float DistSq_(const Vector2& a, const Vector2& b) {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    /**
     * @brief Read a light sidecar JSON file into LightJsonRecord_ values.
     *
     * Accepts both the current schema using `"lights"` as well as legacy files
     * using `"objects"` as the array key.
     *
     * @param path Path to the sidecar JSON file.
     * @param outSceneStem Receives the `"scene"` field value (may be empty).
     * @param out Receives the parsed light records (cleared first).
     * @return true if the file was opened and parsed and a recognized array was found; false otherwise.
     */
    bool ReadLightJsonFile_(const std::string& path, std::string& outSceneStem, std::vector<LightJsonRecord_>& out) {
        out.clear();
        outSceneStem.clear();

        std::ifstream f(path);
        if (!f.is_open())
            return false;

        json j;
        try { f >> j; }
        catch (...) { return false; }

        outSceneStem = j.value("scene", std::string{});

        const json* arr = nullptr;
        if (j.contains("lights") && j["lights"].is_array())
            arr = &j["lights"];
        else if (j.contains("objects") && j["objects"].is_array())
            arr = &j["objects"];

        if (!arr)
            return false;

        for (const auto& o : *arr) {
            if (!o.is_object())
                continue;

            LightJsonRecord_ r{};
            r.name = o.value("name", std::string{});
            r.prefabTag = o.value("prefabTag", std::string{});
            r.enabled = o.value("enabled", r.enabled);

            if (o.contains("position") && o["position"].is_array() && o["position"].size() == 2) {
                r.position.x = o["position"][0].get<float>();
                r.position.y = o["position"][1].get<float>();
            }

            if (o.contains("glow") && o["glow"].is_object()) {
                const auto& g = o["glow"];
                r.glowEnabled = g.value("enabled", r.glowEnabled);
                r.glowRadius = g.value("radius", r.glowRadius);
                r.glowIntensity = g.value("intensity", r.glowIntensity);
                r.glowOpacity = g.value("opacity", r.glowOpacity);
                r.glowSoftness = g.value("softness", r.glowSoftness);
                if (g.contains("color") && g["color"].is_array() && g["color"].size() == 3) {
                    r.glowColor.x = g["color"][0].get<float>();
                    r.glowColor.y = g["color"][1].get<float>();
                    r.glowColor.z = g["color"][2].get<float>();
                }
                if (g.contains("offset") && g["offset"].is_array() && g["offset"].size() == 2) {
                    r.glowOffset.x = g["offset"][0].get<float>();
                    r.glowOffset.y = g["offset"][1].get<float>();
                }
            }

            if (o.contains("source") && o["source"].is_object()) {
                const auto& s = o["source"];
                r.sourceEnabled = s.value("enabled", r.sourceEnabled);
                r.sourceRadius = s.value("radius", r.sourceRadius);
                r.sourceIntensity = s.value("intensity", r.sourceIntensity);
                r.sourceOpacity = s.value("opacity", r.sourceOpacity);
                r.sourceAttenuation = s.value("attenuation", r.sourceAttenuation);
                if (s.contains("color") && s["color"].is_array() && s["color"].size() == 3) {
                    r.sourceColor.x = s["color"][0].get<float>();
                    r.sourceColor.y = s["color"][1].get<float>();
                    r.sourceColor.z = s["color"][2].get<float>();
                }
                if (s.contains("offset") && s["offset"].is_array() && s["offset"].size() == 2) {
                    r.sourceOffset.x = s["offset"][0].get<float>();
                    r.sourceOffset.y = s["offset"][1].get<float>();
                }
            }

            if (o.contains("ember") && o["ember"].is_object()) {
                const auto& e = o["ember"];
                r.emberEnabled = e.value("enabled", r.emberEnabled);
                r.emberRate = e.value("rate", r.emberRate);
                r.emberParticleLife = e.value("particleLife", r.emberParticleLife);
                r.emberSizeStart = e.value("sizeStart", r.emberSizeStart);
                r.emberSizeEnd = e.value("sizeEnd", r.emberSizeEnd);
                r.emberAdditive = e.value("additive", r.emberAdditive);
                if (e.contains("velMin") && e["velMin"].is_array() && e["velMin"].size() == 2) {
                    r.emberVelMin.x = e["velMin"][0].get<float>();
                    r.emberVelMin.y = e["velMin"][1].get<float>();
                }
                if (e.contains("velMax") && e["velMax"].is_array() && e["velMax"].size() == 2) {
                    r.emberVelMax.x = e["velMax"][0].get<float>();
                    r.emberVelMax.y = e["velMax"][1].get<float>();
                }
                if (e.contains("colorStart") && e["colorStart"].is_array() && e["colorStart"].size() == 3) {
                    r.emberColorStart.x = e["colorStart"][0].get<float>();
                    r.emberColorStart.y = e["colorStart"][1].get<float>();
                    r.emberColorStart.z = e["colorStart"][2].get<float>();
                }
                if (e.contains("colorEnd") && e["colorEnd"].is_array() && e["colorEnd"].size() == 3) {
                    r.emberColorEnd.x = e["colorEnd"][0].get<float>();
                    r.emberColorEnd.y = e["colorEnd"][1].get<float>();
                    r.emberColorEnd.z = e["colorEnd"][2].get<float>();
                }
                if (e.contains("offset") && e["offset"].is_array() && e["offset"].size() == 2) {
                    r.emberOffset.x = e["offset"][0].get<float>();
                    r.emberOffset.y = e["offset"][1].get<float>();
                }
            }

            if (o.contains("flicker") && o["flicker"].is_object()) {
                const auto& fli = o["flicker"];
                r.flickerEnabled = fli.value("enabled", r.flickerEnabled);
                r.flickerIntensityMin = fli.value("intensityMin", r.flickerIntensityMin);
                r.flickerIntensityMax = fli.value("intensityMax", r.flickerIntensityMax);
                r.flickerSpeed = fli.value("speed", r.flickerSpeed);
                r.flickerTime = fli.value("time", r.flickerTime);
            }

            out.push_back(r);
        }

        return true;
    }

    /**
     * @brief Apply a LightJsonRecord_ snapshot to an entity's LightComponent.
     *
     * If the entity does not have a LightComponent, one is added before applying
     * values.
     *
     * @param ctx ECS context used to query/add LightComponent.
     * @param e Target entity.
     * @param r Snapshot values to apply.
     */
    void ApplyLightRecord_(IComponentContext& ctx, Entity e, const LightJsonRecord_& r) {
        LightComponent* l = ctx.GetLight(e);
        if (!l) {
            ctx.AddLightComponent(e);
            l = ctx.GetLight(e);
        }
        if (!l)
            return;

        l->enabled = r.enabled;

        l->glow.enabled = r.glowEnabled;
        l->glow.radius = r.glowRadius;
        l->glow.color = r.glowColor;
        l->glow.offset = r.glowOffset;
        l->glow.intensity = r.glowIntensity;
        l->glow.opacity = r.glowOpacity;
        l->glow.softness = r.glowSoftness;

        l->source.enabled = r.sourceEnabled;
        l->source.radius = r.sourceRadius;
        l->source.color = r.sourceColor;
        l->source.offset = r.sourceOffset;
        l->source.intensity = r.sourceIntensity;
        l->source.opacity = r.sourceOpacity;
        l->source.attenuation = r.sourceAttenuation;

        l->ember.enabled = r.emberEnabled;
        l->ember.rate = r.emberRate;
        l->ember.particleLife = r.emberParticleLife;
        l->ember.velMin = r.emberVelMin;
        l->ember.velMax = r.emberVelMax;
        l->ember.colorStart = r.emberColorStart;
        l->ember.colorEnd = r.emberColorEnd;
        l->ember.sizeStart = r.emberSizeStart;
        l->ember.sizeEnd = r.emberSizeEnd;
        l->ember.additive = r.emberAdditive;
        l->ember.offset = r.emberOffset;

        l->flicker.enabled = r.flickerEnabled;
        l->flicker.intensityMin = r.flickerIntensityMin;
        l->flicker.intensityMax = r.flickerIntensityMax;
        l->flicker.speed = r.flickerSpeed;
        l->flicker.time = r.flickerTime;
    }

    /**
     * @brief Find the best matching entity for a saved light record.
     *
     * Matching strategy:
     * - If the record has a non-empty name, prefer exact entity name match.
     * - Otherwise select the nearest Transform position, optionally filtered by prefabTag.
     *
     * @param ctx ECS context used to enumerate entities and transforms.
     * @param app GameApp used for name/prefabTag queries.
     * @param r Saved light record.
     * @param maxDistSq Maximum squared distance allowed for positional matching.
     * @return Matched entity or INVALID_ENTITY if no suitable match is found.
     */
    Entity FindLightTarget_(const IComponentContext& ctx, const GameApp& app, const LightJsonRecord_& r, float maxDistSq) {
        const auto& entities = ctx.GetEntities();

        if (!r.name.empty()) {
            for (Entity e : entities) {
                if (app.GetEntityNameByEntity(e) == r.name)
                    return e;
            }
        }

        Entity best = INVALID_ENTITY;
        float bestDistSq = maxDistSq;

        for (Entity e : entities) {
            const Transform* t = ctx.TryGetTransform(e);
            if (!t)
                continue;

            if (!r.prefabTag.empty()) {
                const std::string tag = app.GetPrefabTag(e);
                if (tag != r.prefabTag)
                    continue;
            }

            const float d2 = DistSq_(t->GetPosition(), r.position);
            if (d2 < bestDistSq) {
                bestDistSq = d2;
                best = e;
            }
        }

        return best;
    }

    /**
     * @brief Serialize all LightComponents in the current scene to a sidecar JSON file.
     *
     * Iterates all entities and writes out records for those that have both a Transform
     * and a LightComponent. Records include entity name and prefabTag (when present)
     * to improve matching during LoadLevel() / ApplyLightOverrides().
     *
     * @param ctx ECS context providing entity/component access.
     * @param app GameApp used for entity name and prefab tag lookup.
     * @param sceneStem Scene stem written into the `"scene"` field.
     * @param outPath Output path for the sidecar JSON file.
     */
    void SaveLightFile_(const IComponentContext& ctx, const GameApp& app, const std::string& sceneStem, const std::string& outPath) {
        json j;
        j["version"] = 1;
        j["scene"] = sceneStem;
        j["lights"] = json::array();

        for (Entity e : ctx.GetEntities()) {
            const Transform* t = ctx.TryGetTransform(e);
            const LightComponent* l = ctx.TryGetLight(e);
            if (!t || !l)
                continue;

            json o;
            const std::string name = app.GetEntityNameByEntity(e);
            if (!name.empty())
                o["name"] = name;

            const std::string prefabTag = app.GetPrefabTag(e);
            if (!prefabTag.empty())
                o["prefabTag"] = prefabTag;

            o["position"] = { t->GetPosition().x, t->GetPosition().y };
            o["enabled"] = l->enabled;

            o["glow"] = {
                { "enabled", l->glow.enabled },
                { "radius", l->glow.radius },
                { "color", { l->glow.color.x, l->glow.color.y, l->glow.color.z } },
                { "offset", { l->glow.offset.x, l->glow.offset.y } },
                { "intensity", l->glow.intensity },
                { "opacity", l->glow.opacity },
                { "softness", l->glow.softness }
            };

            o["source"] = {
                { "enabled", l->source.enabled },
                { "radius", l->source.radius },
                { "color", { l->source.color.x, l->source.color.y, l->source.color.z } },
                { "offset", { l->source.offset.x, l->source.offset.y } },
                { "intensity", l->source.intensity },
                { "opacity", l->source.opacity },
                { "attenuation", l->source.attenuation }
            };

            o["ember"] = {
                { "enabled", l->ember.enabled },
                { "rate", l->ember.rate },
                { "particleLife", l->ember.particleLife },
                { "velMin", { l->ember.velMin.x, l->ember.velMin.y } },
                { "velMax", { l->ember.velMax.x, l->ember.velMax.y } },
                { "colorStart", { l->ember.colorStart.x, l->ember.colorStart.y, l->ember.colorStart.z } },
                { "colorEnd", { l->ember.colorEnd.x, l->ember.colorEnd.y, l->ember.colorEnd.z } },
                { "sizeStart", l->ember.sizeStart },
                { "sizeEnd", l->ember.sizeEnd },
                { "additive", l->ember.additive },
                { "offset", { l->ember.offset.x, l->ember.offset.y } }
            };

            o["flicker"] = {
                { "enabled", l->flicker.enabled },
                { "intensityMin", l->flicker.intensityMin },
                { "intensityMax", l->flicker.intensityMax },
                { "speed", l->flicker.speed },
                { "time", l->flicker.time }
            };

            j["lights"].push_back(std::move(o));
        }

        fs::path p(outPath);
        std::error_code ec;
        if (!p.parent_path().empty())
            fs::create_directories(p.parent_path(), ec);

        std::ofstream f(outPath);
        if (!f.is_open()) {
            DebugConsole::Get().Error("[SceneLoader] Failed to save " + outPath + "\n");
            return;
        }

        f << j.dump(2);
    }
}

/*
 * @brief Read a 2D vector from JSON.
 *
 * Extracts a x, y arr from the JSON obj if present.
 *
 * @param j JSON object containing the key.
 * @param key Field name to parse.
 * @param def Default value if not found.
 * @return Parsed Vector2 value.
*/
static Vector2 ReadVec2(const json& j, const std::string& key, Vector2 def = { 0, 0 }) {
    if (j.contains(key) && j[key].is_array() && j[key].size() == 2)
        return { j[key][0].get<float>(), j[key][1].get<float>() };
    return def;
}
/*
 * @brief Read a 3D vector from JSON.
 *
 * Extracts a rgb arr from the JSON obj if present.
 *
 * @param j JSON object containing the key.
 * @param key Field name to parse.
 * @param def Default color if not found.
 * @return Parsed Vector3 value.
 */
static Vector3 ReadVec3(const json& j, const std::string& key, Vector3 def = { 1, 1, 1 }) {
    if (j.contains(key) && j[key].is_array() && j[key].size() == 3)
        return { j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>() };
    return def;
}

/**
    * @brief Loads all entities described in a JSON scene file into the GameApp.
    *
    * The loader handles:
    *  - Procedural maze/room generation (map.generateMaze, map.generateRoom)
    *  - Grid loading from external .txt files
    *  - Prefab instantiation (including Player, Doorway, and custom prefabs)
    *  - Legacy field compatibility (old center-based transforms)
    *  - Hierarchical transform setup (local/world + parent)
    *  - Automatic sword spawning + parenting for the Player prefab
    *  - Optional particle emitter attachment (based on prefab data)
    *
    * Entity creation flow:
    *  1. Parse the top-level map block and either:
    *      - generate a maze,
    *      - generate a procedural room,
    *      - load a grid from file, or
    *      - read inline grid rows.
    *  2. Determine the array of objects, or treat JSON root as an array.
    *  3. For each object:
    *      - If `"prefab"` exists → call InstantiatePrefab().
    *      - Special logic for Player prefab:
    *            * Reuse persistent player if available
    *            * Otherwise spawn new player + attach sword
    *      - For non-prefab objects → fallback legacy entity creation flow.
    *  4. Apply transform, renderer, collider, script, and parent fields.
    *  5. Register doors discovered in scene data.
    *
    * Supported JSON fields include:
    *  name, meshIndex, position, scale, rotation,
    *  meshExtent, color, texture, prefab, parent,
    *  local, colliderType, isTrigger, targetScene,
    *  arrivalDir, targetDoor.
    *
    * @param app      Reference to the running GameApp instance.
    * @param filename Absolute or relative path to the JSON scene file.
    *
    * @return true  if file opened and parsed successfully.
    * @return false if file could not be opened or JSON parsing failed.
*/
bool LoadEntitiesJSON(GameApp& app, const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) { DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[ERROR] Could not open " + filename + "\n"); return false; }

    InitForceSystem(app); /// Ensure force system is initialized 

    json root;
    try { f >> root; }
    catch (const std::exception& e) {
        std::string s = e.what();
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[ERROR] JSON parse failed in " + filename, ": " + s + "\n");
        return false;
    }

    app.GetDoorSystem().ClearDoors();

    if (root.contains("map") && root["map"].is_object()) {
        const auto& map = root["map"];
        float tileSize = map.value("tileSize", 64.0f);
        std::vector<std::string> grid;

        // procedural maze generation
        if (map.value("generateMaze", false)) {
            int worldSize = map.value("worldSizePx", 5000);
            DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[MapGenerator] Procedurally generating maze (", worldSize, "px, tileSize=", tileSize, ")\n");
            MapGenerator::GenerateMaze(app, worldSize, tileSize);
        }
        else if (map.value("generateRoom", false)) {
            int roomW = map.value("roomWidth", 40);
            int roomH = map.value("roomHeight", 25);

            // Either explicit path from JSON, or derive one
            std::string gridPath;
            if (map.contains("gridFile") && map["gridFile"].is_string()) {
                gridPath = AssetPath(map["gridFile"].get<std::string>());
            }
            else {
                // derive: "Assets/scene/entities_Level1.json" -> "Assets/maps/entities_Level1.txt"
                std::string base = filename;
                size_t slash = base.find_last_of("/\\");
                if (slash != std::string::npos)
                    base = base.substr(slash + 1);
                size_t dot = base.find_last_of('.');
                if (dot != std::string::npos)
                    base = base.substr(0, dot);
                gridPath = AssetPath("maps/" + base + ".txt");
            }

            DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[MapGenerator] Procedurally generating room '", gridPath, "' (", roomW, "x", roomH, ", tileSize=", tileSize, ")\n");

            MapGenerator::GenerateRoom(app,
                filename,   // scenePath
                gridPath,   // where to save/load room txt
                roomW,
                roomH,
                tileSize);
        }
        // existing grid loading (for text/JSON maps)
        else if (map.contains("gridFile") && map["gridFile"].is_string()) {
            std::string gridPath = AssetPath(map["gridFile"].get<std::string>());
            std::ifstream file(gridPath);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    if (!line.empty())
                        grid.push_back(line);
                }
                DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[MapGenerator] Loaded grid from file: ", gridPath, " (", grid.size(), " rows)\n");
            }
            else {
                DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[MapGenerator] Failed to open gridFile: ", gridPath, "\n");
            }
        }
        else if (map.contains("grid") && map["grid"].is_array()) {
            for (const auto& row : map["grid"])
                grid.push_back(row.get<std::string>());
            DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[MapGenerator] Loaded grid inline (", grid.size(), " rows)\n");
        }

        if (!grid.empty()) {
            const std::string bakePrefix = "MapBake_" + std::filesystem::path(filename).stem().string() + "_";
            MapGenerator::FromGrid(app, grid, tileSize, bakePrefix);
            app.SetEnemyGrid(grid, tileSize);
        }
    }

    json arr;
    if (root.is_object() && root.contains("objects") && root["objects"].is_array())
        arr = root["objects"];
    else if (root.is_object() && root.contains("entities") && root["entities"].is_array())
        arr = root["entities"];
    else if (root.is_array())
        arr = root;
    else {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[ERROR] Scene file must be an array or have an 'objects' or 'entities' array: " + filename + "\n");
        return false;
    }

    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[INFO] Loading scene objects from ", filename, " (count=", arr.size(), ")\n");

    app.ReserveLevelPools(arr.size());
    DebugConsole::Get().AddFormattedMessage(LogLevel::Info,
        "[INFO] Pools reserved. "
        "T(pages=", app.TransformPoolPages(), " live=", app.TransformPoolLive(), " exp=", app.TransformPoolExpansions(), ") "
        "C(pages=", app.ColliderPoolPages(), " live=", app.ColliderPoolLive(), " exp=", app.ColliderPoolExpansions(), ") "
        "R(pages=", app.RendererPoolPages(), " live=", app.RendererPoolLive(), " exp=", app.RendererPoolExpansions(), ") "
        "P(pages=", app.ProjectilePoolPages(), " live=", app.ProjectilePoolLive(), " exp=", app.ProjectilePoolExpansions(), ")\n");

    for (size_t i = 0; i < arr.size(); ++i) {
        const json& e = arr[i];
        try {
            std::string name = e.value("name", "Unnamed");
            Vector2 pos = ReadVec2(e, "position", { 0,0 });

            if (e.contains("prefab") && e["prefab"].is_string()) {
                std::string prefabName = e["prefab"].get<std::string>();
                DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "  [Prefab] ", prefabName, " as '", name, "' @(", pos.x, ",", pos.y, ")\n");

                // --- Player prefab ---
                if (prefabName == "Player") {
                    int meshIndex = 0;  // from your prefab
                    Vector2 scale = ReadVec2(e, "scale", { 50, 80 });
                    Vector3 color = ReadVec3(e, "color", { 1, 1, 1 });
                    float rot = e.value("rotation", 0.0f);
                    Vector2 extent = ReadVec2(e, "meshExtent", { 1, 1 });
                    Vector2 posP = app.GetPlayerSpawn();
                    if (posP == Vector2{ 0,0 })  // fallback to JSON-defined position
                        posP = ReadVec2(e, "position", { 0,0 });


                    // --- Check if a persistent player already exists ---
                    Entity existingPlayer = 0;
                    for (auto& [id, tag] : app.GetPersistentTags()) {
                        if (app.GetController(id)) { // PlayerController = marker of Player entity
                            existingPlayer = id;
                            break;
                        }
                    }

                    if (existingPlayer) {
                        // --- Reuse existing persistent player ---
                        if (Transform* t = app.GetTransform(existingPlayer)) {
                            t->SetPosition(posP);
                            t->SetRotation(rot);
                            t->SetLocalPosition(t->GetPosition());
                            t->SetLocalScale(t->GetScale());
                            t->SetLocalRotation(t->GetRotation());
                            t->SetParent(INVALID_ENTITY);

                        }

                        if (auto* ctl = app.GetController(existingPlayer)) {
                            // ctl->ResetSpeed();
                            if (g_entityForceProxy) {
                                /*if (SpeedComponent* spd = g_entityForceProxy->GetSpeedComponent(existingPlayer)) {
                                    spd->maxSpeed = 250.0f;
                                    spd->friction = 625.0f;
                                    spd->roomSpeed = false;
                                    spd->outsideSpeed = true;
                                }*/
                                ctl->setForceProxy(g_entityForceProxy);
                            }
                        }

                        // Make sure the global handle points at the reused player
                        GameApp::playerEntity = existingPlayer;

                        DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Loader] Found persistent player; repositioned to new spawn.\n");

                    }
                    else {
                        // --- No player yet: create a new one ---
                        GameApp::playerEntity = MakePlayer(app, name, app.GetMesh(meshIndex), posP, scale, rot, color, extent);
                        DebugConsole::Get().Info("[Loader] Spawned new persistent player.\n");
                        auto* ctl = app.GetController(GameApp::playerEntity);
                        if (Transform* tNew = app.GetTransform(GameApp::playerEntity)) {
                            tNew->SetLocalPosition(tNew->GetPosition());
                            tNew->SetLocalScale(tNew->GetScale());
                            tNew->SetLocalRotation(tNew->GetRotation());
                            tNew->SetParent(INVALID_ENTITY);
                        }
                        /// Check if the player controller exits
                        if (ctl) {
                            /// Assign global EntityForceProxy pointer to the controller
                            ctl->setForceProxy(g_entityForceProxy);
                            DebugConsole::Get().Info("[PROXY CHECK] PlayerController found and ForceProxy assigned.\n");
                        }
                    }
                    // --- Attach particle emitter if defined in prefab ---
                    if (GameApp::playerEntity != INVALID_ENTITY) {
                        const Prefab* p = app.prefabManager.Get("Player");
                        if (p && p->hasEmitter) {
                            // Only add if not already attached
                            if (!app.GetEmitter(GameApp::playerEntity)) {
                                ParticleEmitter& em = app.AddEmitter(GameApp::playerEntity);
                                em.enabled = p->emitter.enabled;   // your prefab sets this to false initially
                                em.rate = p->emitter.rate;
                                em.particleLife = p->emitter.particleLife;
                                em.velMin = p->emitter.velMin;
                                em.velMax = p->emitter.velMax;
                                em.offset = p->emitter.offset;
                                em.sizeStart = p->emitter.sizeStart;
                                em.sizeEnd = p->emitter.sizeEnd;
                                em.colorStart = p->emitter.colorStart;
                                em.colorEnd = p->emitter.colorEnd;
                                em.quad = app.GetMesh(static_cast<size_t>(p->emitter.meshIndex));
                                em.texture = 0;

                                if (!p->emitter.texture.empty()) {
                                    if (GLuint texID = ResourceManager::GetTexture(p->emitter.texture)) {
                                        em.texture = texID;
                                    }
                                    else {
                                        DebugConsole::Get().AddFormattedMessage(LogLevel::Warning, "Emitter texture not found: '", p->emitter.texture, "'\n");
                                    }
                                }

                                DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Loader] Attached emitter to Player (rate=", em.rate, ", life=", em.particleLife, ")\n");
                            }
                        }
                    }

                    if (auto* ctrl = app.GetController(GameApp::playerEntity)) {
                        if (auto* em = app.GetEmitter(GameApp::playerEntity)) {
                            ctrl->BindEmitter(em);   // let the controller toggle + offset it
                            em->enabled = false;     // start off; controller will enable on move
                            DebugConsole::Get().Info("[Loader] Bound emitter to player controller (start disabled)\n");
                        }
                    }

                    if (auto* t = app.GetTransform(GameApp::playerEntity)) {
                        app.GetCamera().setPosition(t->GetPosition());
                        DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Init] Camera centered on player at ", t->GetPosition().x, ", ", t->GetPosition().y, "\n");
                    }
                    continue;
                }

                // --- def prefab instantiation for everything else ---
                Entity ent = app.InstantiatePrefab(prefabName, pos);
                if (!ent) { DebugConsole::Get().Error("   -> failed (missing prefab?)\n"); continue; }

                app.RegisterPrefabDefaults(ent);

                if (Transform* t = app.GetTransform(ent)) {
                    // If the JSON has NO "local", it�s an old scene that stored center coords.
                    const bool legacyCenter = !(e.contains("local") && e["local"].is_object());
                    if (legacyCenter) {
                        // Compute size (prefer collider if present)
                        Vector2 size = t->GetScale();
                        if (const Collider* c = app.GetCollider(ent)) size = c->size;

                        // Convert center -> top-left exactly once on load
                        t->SetPosition(pos - size);
                    }
                }

                if (prefabName == "Doorway") {
                    DoorLink link{};
                    link.entity = ent;
                    link.name = e.value("name", "UnnamedDoor");
                    link.targetScene = e.value("targetScene", "");
                    link.targetDoor = e.value("targetDoor", "");

                    if (e.contains("arrivalDir")) {
                        std::string dir = e["arrivalDir"].get<std::string>();
                        if (dir == "top")    link.arrivalDir = DoorArrivalDir::Top;
                        if (dir == "bottom") link.arrivalDir = DoorArrivalDir::Bottom;
                        if (dir == "left")   link.arrivalDir = DoorArrivalDir::Left;
                        if (dir == "right")  link.arrivalDir = DoorArrivalDir::Right;
                    }
                    app.GetDoorSystem().RegisterDoor(link);
                    //app.RegisterDoor(link);

                    if (Collider* c = app.GetCollider(ent))
                        c->isTrigger = true;
                }

                if (Transform* t = app.GetTransform(ent)) {
                    if (e.contains("scale"))    t->SetScale(ReadVec2(e, "scale", t->GetScale()));
                    if (e.contains("rotation")) t->SetRotation(e["rotation"].get<float>());
                    // hierarchy-friendly fields
                    if (e.contains("local") && e["local"].is_object()) {
                        const auto& L = e["local"];
                        if (L.contains("position")) t->SetLocalPosition(ReadVec2(L, "position", t->GetLocalPosition()));
                        if (L.contains("scale"))    t->SetLocalScale(ReadVec2(L, "scale", t->GetLocalScale()));
                        if (L.contains("rotation")) t->SetLocalRotation(L["rotation"].get<float>());
                    }
                    else {
                        // back-compat for old scenes: treat world as local
                        t->SetLocalPosition(t->GetPosition());
                        t->SetLocalScale(t->GetScale());
                        t->SetLocalRotation(t->GetRotation());
                    }
                    if (e.contains("parent")) t->SetParent(static_cast<Entity>(e["parent"].get<int>()));
                }
                if (MeshRenderer* mr = app.GetRenderer(ent)) {
                    if (e.contains("color"))    mr->SetColor(ReadVec3(e, "color", mr->GetColor()));
                }

                if (e.value("isDead", false)) {
                    app.OnEnemyDeath(ent);
                }
                continue;
            }

            // non-prefab (old JUST IN CASE)
            std::string type = e.value("type", "GameObject");
            int meshIndex = e.value("meshIndex", 0);
            Vector2 scale = ReadVec2(e, "scale", { 1,1 });
            Vector3 color = ReadVec3(e, "color", { 1,1,1 });
            float rot = e.value("rotation", 0.0f);
            Vector2 extent = ReadVec2(e, "meshExtent", { 1,1 });
            std::string tex = e.value("texture", "");

            bool trigger = false;
            if (e.contains("isTrigger")) {
                if (e["isTrigger"].is_boolean()) trigger = e["isTrigger"].get<bool>();
                else if (e["isTrigger"].is_number_integer()) trigger = (e["isTrigger"].get<int>() != 0);
            }
            std::string colliderStr = e.value("colliderType", "Box");
            ColliderType colType = (colliderStr == "Circle") ? ColliderType::Circle :
                (colliderStr == "Triangle") ? ColliderType::Triangle : ColliderType::Box;

            DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "  [Collider] Type=", colliderStr, ", isTrigger=", trigger, "\n");

            Entity ent = INVALID_ENTITY;
            if (type == "Player") {
                ent = MakePlayer(app, name, app.GetMesh(meshIndex), pos, scale, rot, color, extent);
            }
            else {
                ent = MakeGameObject(app, name, app.GetMesh(meshIndex),
                    pos, scale, rot, color, colType, trigger, extent);
                if (!tex.empty()) {
                    GLuint texID = ResourceManager::GetTexture(tex);
                    if (texID != 0)
                        if (auto* mr = app.GetRenderer(ent)) mr->SetTexture(texID);
                }
            }

            if (ent != INVALID_ENTITY && e.value("isDead", false)) {
                app.OnEnemyDeath(ent);
            }
        }
        catch (const nlohmann::json::exception& ex) {
            DebugConsole::Get().AddFormattedMessage(LogLevel::Warning, " Entity #", i, " skipped: JSON error: ", ex.what(), "\n");
        }
        catch (...) {
            DebugConsole::Get().AddFormattedMessage(LogLevel::Warning, " Entity #", i, " skipped: Unknown error\n");
        }
    }
    return true;
}




/**
 * @brief Loads an audio profile from a JSON file and applies it at runtime.
 *
 * This function parses a structured JSON file to configure or override
 * the game’s audio state dynamically, without modifying the default
 * `config.json`. It can adjust bus volumes, register or replace sound
 * assets, and optionally start a background music (BGM) track.
 *
 * **Supported JSON sections**:
 *
 * - **"volumes"** (optional):
 *   - `"masterDb"` — Global master bus volume in decibels (float)
 *   - `"musicDb"`  — Music bus volume in decibels (float)
 *   - `"sfxDb"`    — SFX bus volume in decibels (float)
 *
 * - **"sounds"** (optional): Dictionary mapping logical sound keys to file paths.
 *   ```json
 *   "sounds": {
 *     "door_open":  "audio/Door Open FX.mp3",
 *     "door_close": "audio/Door Close FX.mp3",
 *     "button":     "audio/Button.mp3"
 *   }
 *   ```
 *   Each sound is loaded from `"Assets/" + path` and registered under its key.
 *   If an existing key is found, the old sound is unloaded before replacing it.
 *
 * - **"bgm"** (optional): Defines a background music track to play immediately.
 *   - `"key"`  — logical key for lookup or registration
 *   - `"file"` — relative path to the audio file
 *   - `"loop"` — whether the track should loop (default: true)
 *   - `"gain"` — playback gain multiplier (default: 0.9)
 *
 * **Behavior**:
 * - When @p applyVolumes is true, updates FMOD bus dB levels.
 * - When @p startBgmIfAny is true, stops current music (short fade)
 *   and plays the new BGM described in `"bgm"`.
 *
 * **Example**:
 * ```json
 * {
 *   "volumes": { "masterDb": 0.0, "musicDb": -6.0, "sfxDb": 0.0 },
 *   "sounds":  { "door_open": "audio/door.wav" },
 *   "bgm":     { "key": "battleA", "file": "audio/battleThemeA.mp3", "loop": true, "gain": 0.9 }
 * }
 * ```
 *
 * @param filename      Path to the JSON audio profile (e.g., `"Assets/audio.json"`).
 * @param applyVolumes  If true, applies any `"volumes"` values to audio buses.
 * @param startBgmIfAny If true, starts playback of `"bgm"` when present.
 * @return true if the file was successfully opened and parsed; false otherwise.
 */
bool LoadAudioJSON(const std::string& filename, bool applyVolumes, bool startBgmIfAny) {
    std::ifstream f(filename);
    if (!f.is_open()) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[AudioLoader] Could not open ", filename, "\n");
        return false;
    }

    nlohmann::json j;
    try { f >> j; }
    catch (const std::exception& e) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[AudioLoader] JSON parse error in ", filename, ": ", e.what(), "\n");
        return false;
    }

    fs::path root = fs::path(filename).parent_path();   // e.g. .../Game/assets

    // 1) (Optional) set bus volumes
    if (applyVolumes && j.contains("volumes") && j["volumes"].is_object()) {
        const auto& v = j["volumes"];
        if (v.contains("masterDb")) Audio::SetBusDb(Audio::Bus::Master, v["masterDb"].get<float>());
        if (v.contains("musicDb"))  Audio::SetBusDb(Audio::Bus::Music, v["musicDb"].get<float>());
        if (v.contains("sfxDb"))    Audio::SetBusDb(Audio::Bus::Sfx, v["sfxDb"].get<float>());
    }

    // 2) (Optional) load/override sounds
    if (j.contains("sounds") && j["sounds"].is_object()) {
        for (auto& [key, val] : j["sounds"].items()) {
            if (!val.is_string()) continue;
            const std::string rel = val.get<std::string>();
            fs::path fullPath = root / rel;

            // If an old sound exists under this key, unload it first to avoid leaks
            if (auto old = ResourceManager::GetAudio(key); old >= 0) {
                Audio::UnloadSound(old);
            }

            ResourceManager::RegisterAudio(key, fullPath.string()); // registers (key -> SoundID)
        }
    }

    // 3) (Optional) start/replace BGM
    if (startBgmIfAny && j.contains("bgm") && j["bgm"].is_object()) {
        const auto& b = j["bgm"];
        const std::string key = b.value("key", "");
        const std::string file = b.value("file", "");
        const bool loop = b.value("loop", true);
        const float gain = b.value("gain", 0.0f);

        if (!key.empty() && !file.empty()) {
            // ensure the bgm key is loaded (if not loaded by "sounds" above)
            if (ResourceManager::GetAudio(key) < 0) {
                fs::path full = root / file;
                ResourceManager::RegisterAudio(key, full.string());
            }

            // fade out current music gracefully, then start new
            Audio::StopAll(150);

            if (auto sid = ResourceManager::GetAudio(key); sid >= 0) {
                Audio::PlayDesc d;
                d.bus = Audio::Bus::Music;
                d.loop = loop;
                d.gain = gain;
                Audio::Play(sid, d);
            }
            else {
                DebugConsole::Get().Error("[AudioLoader] BGM key '" + key + "' not found/loaded.\n");
            }
        }
    }

    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[AudioLoader] Applied audio profile: ", filename, "\n");
    return true;
}

/**
 * @brief Load textures from JSON and register them with the ResourceManager.
 * JSON shape:
 * { "textures": { "logicalName": "relative/path.png", ... } }
 */
bool LoadTexturesJSON(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) { DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[TexLoader] Could not open " + filename, "\n"); return false; }

    nlohmann::json j;
    try { f >> j; }
    catch (const std::exception& e) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[TexLoader] JSON parse error in " + filename + ": " + e.what() + "\n");
        return false;
    }

    if (!j.contains("textures") || !j["textures"].is_object()) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[TexLoader] Missing object 'textures' in " + filename + "\n");
        return false;
    }

    fs::path root = fs::path(filename).parent_path();   // e.g. .../Game/assets

    size_t ok = 0, fail = 0;
    for (auto& [name, val] : j["textures"].items()) {
        if (!val.is_string()) { ++fail; continue; }

        fs::path abs = root / val.get<std::string>();   // <- root + "textures/..." etc.

        if (ResourceManager::ImportTexture(abs.string(), name)) ++ok;
        else {
            DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[TexLoader] Failed to load texture: ", abs.string(), " as '", name, "'\n");
            ++fail;
        }
    }


    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[TexLoader] Loaded " + std::to_string(ok), " Textures (" + std::to_string(fail), " failed)\n");
    return ok > 0 || fail == 0;
}

/**
 * @brief Load sprite sheets from JSON and register them with the ResourceManager.
 * JSON shape:
 * { "sheets": [
 *    {"name":"run","file":"atlases/run.png","frameW":40,"frameH":55,"cols":5,"rows":1,"frameDuration":0.8}
 * ] }
 */
bool LoadSpriteSheetsJSON(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) { DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[SheetLoader] Could not open " + filename, "\n"); return false; }

    nlohmann::json j;
    try { f >> j; }
    catch (const std::exception& e) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[SheetLoader] JSON parse error in " + filename + ": " + e.what() + "\n");
        return false;
    }

    if (!j.contains("sheets") || !j["sheets"].is_array()) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[SheetLoader] Missing array 'sheets' in " + filename + "\n");
        return false;
    }

    fs::path root = fs::path(filename).parent_path();   // .../Game/assets

    size_t ok = 0, fail = 0;
    for (const auto& s : j["sheets"]) {
        if (!s.is_object()) { ++fail; continue; }

        const std::string name = s.value("name", "");
        const std::string file = s.value("file", "");
        if (name.empty() || file.empty()) {
            DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[SheetLoader] Sheet missing name/file\n");
            ++fail;
            continue;
        }

        SpriteSheet sh;
        sh.frameW = s.value("frameW", 0);
        sh.frameH = s.value("frameH", 0);
        sh.cols = s.value("cols", 1);
        sh.rows = s.value("rows", 1);
        sh.frameDuration = s.value("frameDuration", 0.1f);

        fs::path abs = root / file;                    // <- root + "atlases/..."

        if (auto tex = ResourceManager::loadTexAbs(abs.string())) {
            sh.texture = std::move(*tex);
            if (ResourceManager::RegisterSpriteSheet(name, std::move(sh))) {
                ++ok;
            }
            else {
                DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[SheetLoader] Failed to register sprite sheet as '", name, "'\n");
                ++fail;
            }
        }
        else {
            DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[SheetLoader] Failed to load texture for sprite sheet: ", abs.string(), "\n");
            ++fail;
        }
    }

    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[SheetLoader] Loaded " + std::to_string(ok), " Sprite Sheets (" + std::to_string(fail), " failed)\n");
    return ok > 0 || fail == 0;
}

namespace SceneLoader {

    /**
     * @brief Loads entities from a scene JSON file and applies light sidecar overrides.
     * @param app Target GameApp instance to populate.
     * @param sceneFile Scene JSON path (e.g. "Assets/scene/entities_Level1.json").
     * @return true if scene JSON was loaded successfully; false otherwise.
     */
    bool LoadLevel(GameApp& app, const std::string& sceneFile) {
        const bool ok = LoadEntitiesJSON(app, sceneFile);
        if (ok)
            ApplyLightOverrides(app, sceneFile);
        return ok;
    }

    /**
     * @brief Saves only the light sidecar JSON for a scene.
     *
     * @param context ECS context providing LightComponent access.
     * @param sceneFile Scene JSON path used to derive `<sceneStem>`.
     */
    void SaveLightSidecar(const IComponentContext& context, const std::string& sceneFile) {
        const GameApp* app = dynamic_cast<const GameApp*>(&context);
        if (!app) {
            DebugConsole::Get().Error("[SceneLoader] SaveLightSidecar: context is not a GameApp!\n");
            return;
        }

        const std::string stem = fs::path(sceneFile).stem().string();
        const std::string lightPath = AssetPath("scene/" + stem + "_lights.json");
        SaveLightFile_(context, *app, stem, lightPath);
    }

    /**
    * @brief Serializes all ECS entities into a JSON scene file.
    *
    * Features:
    *  - Skips runtime-only entities (e.g., Sword).
    *  - Writes prefab name and override flags (scale/rotation/color).
    *  - Writes transform, local transform, parent hierarchy.
    *  - Writes renderer color and collider data.
    *  - Special handling for .txt paths → saves labyrinth grid.
    *
    * @param context ECS context providing component access.
    * @param path    Destination file path (.json or .txt).
    */
    void SaveLevel(const IComponentContext& context, const std::string& path, const std::vector<Entity>* entitiesToSave) {
        const GameApp* app = dynamic_cast<const GameApp*>(&context);
        if (!app) {
            DebugConsole::Get().Error("[SceneLoader] SaveLevel: context is not a GameApp!\n");
            return;
        }

        if (path.size() >= 4 && path.ends_with(".txt")) {
            SaveLabyrinthLayout(context, *app, path);
            DebugConsole::Get().Info("[SceneLoader] Saved labyrinth to " + path + "\n");

            const std::string stem = fs::path(path).stem().string();
            const std::string lightPath = AssetPath("scene/" + stem + "_lights.json");
            SaveLightFile_(context, *app, stem, lightPath);

            return;
        }

        json j;
        j["objects"] = json::array();

        const auto& entities = entitiesToSave ? *entitiesToSave : context.GetEntities();

        for (Entity e : entities) {
            const Transform* t = context.TryGetTransform(e);
            if (!t) continue;

            // Skip saving runtime sword
            std::string entName = app->GetEntityNameByEntity(e);
            if (entName == "Sword") continue;

            const MeshRenderer* mr = context.TryGetRenderer(e);
            const Collider* c = context.TryGetCollider(e);

            json je;

            // --- Prefab info ---
            const std::string prefabName = app->GetPrefabTag(e);
            const bool hasPrefabTag = !prefabName.empty();

            if (hasPrefabTag)
                je["prefab"] = prefabName;

            // read defaults through accessor (NO private member access)
            const GameApp::PrefabInstanceDefaults* defs =
                hasPrefabTag ? app->TryGetPrefabDefaults(e) : nullptr;

            je["position"] = { t->GetPosition().x, t->GetPosition().y };

            json overrides = json::object();

            if (defs) {
                Vector2 curScale = t->GetScale();
                if (!AlmostEqualVec2(curScale, defs->scale)) {
                    je["scale"] = { curScale.x, curScale.y };
                    overrides["scale"] = true;
                }

                float curRot = t->GetRotation();
                if (!AlmostEqual(curRot, defs->rotation)) {
                    je["rotation"] = curRot;
                    overrides["rotation"] = true;
                }
            }
            else {
                je["scale"] = { t->GetScale().x, t->GetScale().y };
                je["rotation"] = t->GetRotation();
            }

            // hierarchy fields
            je["parent"] = INVALID_ENTITY;
            je["local"] = {
                { "position", { t->GetLocalPosition().x, t->GetLocalPosition().y } },
                { "scale",    { t->GetLocalScale().x,    t->GetLocalScale().y    } },
                { "rotation",  t->GetLocalRotation() }
            };
            je["parent"] = t->GetParent();

            if (mr) {
                const Vector3& col = mr->GetColor();
                if (defs) {
                    if (!AlmostEqualVec3(col, defs->color)) {
                        je["color"] = { col.x, col.y, col.z };
                        overrides["color"] = true;
                    }
                }
                else {
                    je["color"] = { col.x, col.y, col.z };
                }
            }

            if (c) {
                je["colliderSize"] = { c->size.x, c->size.y };
                je["isTrigger"] = c->isTrigger;
            }

            if (defs && !overrides.empty())
                je["overrides"] = overrides;

            j["objects"].push_back(je);
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            DebugConsole::Get().Error("[SceneLoader] Failed to save " + path + "\n");
            return;
        }

        file << j.dump(4);
        DebugConsole::Get().Info("[SceneLoader] Saved " +
            std::to_string(j["objects"].size()) + " entities to " + path + "\n");

        // Removed light saving functionality for now
        // const std::string stem = fs::path(path).stem().string();
        // const std::string lightPath = (fs::path(path).parent_path() / (stem + "_lights.json")).string();
        // SaveLightFile_(context, *app, stem, lightPath);
    }

    /**
     * @brief Applies LightComponent overrides from `Assets/scene/<stem>_lights.json`.
     *
     * Matches saved records to live entities using:
     *  - Exact entity name match (if record provides a name)
     *  - Otherwise, nearest entity position (optionally filtered by prefabTag)
     *
     * @param app GameApp whose ECS will be updated.
     * @param sceneFile Scene JSON path used to derive `<stem>`.
     */
    void ApplyLightOverrides(GameApp& app, const std::string& sceneFile) {
        IComponentContext& ctx = app;
        const std::string stem = fs::path(sceneFile).stem().string();
        const std::string lightPath = AssetPath("scene/" + stem + "_lights.json");

        std::string sceneStem;
        std::vector<LightJsonRecord_> records;
        if (!ReadLightJsonFile_(lightPath, sceneStem, records))
            return;

        const float tile = std::max(app.GetLabyrinthTileSize(), app.GetTileSize());
        const float maxDistSq = (tile > 0.0f ? (tile * 0.5f) * (tile * 0.5f) : 4096.0f);

        for (const auto& r : records) {
            Entity target = FindLightTarget_(ctx, app, r, maxDistSq);
            if (target != INVALID_ENTITY)
                ApplyLightRecord_(ctx, target, r);
        }
    }

    /**
    * @brief Saves the labyrinth grid layout (walls, floors, doors, enemies, crates)
    *        to a .txt file in top-origin coordinate order.
    *
    * Procedure:
    *  1. Load existing grid as the authoritative base.
    *  2. Clear enemy ('4') and crate ('3') positions.
    *  3. Convert world positions of entities into grid coordinates.
    *  4. Place enemies and crates back into the grid.
    *  5. Write the updated grid, preserving walls/floors/doors.
    *
    * @param ctx   ECS component context.
    * @param app   GameApp instance for tile/world bounds data.
    * @param txtPath Output path (.txt).
    */
    void SaveLabyrinthLayout(const IComponentContext& ctx, const GameApp& app, const std::string& txtPath) {
        const float   tile = app.GetLabyrinthTileSize();
        const Vector2 minB = app.GetWorldMinBound();
        const Vector2 maxB = app.GetWorldMaxBound();

        auto clampi = [](int v, int lo, int hi) { return (v < lo ? lo : (v > hi ? hi : v)); };

        std::vector<std::string> base;
        {
            std::ifstream in(txtPath);
            std::string line;
            while (std::getline(in, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!line.empty()) base.push_back(line);
            }
        }

        int width = 0, height = 0;
        if (!base.empty()) {
            height = (int)base.size();
            width = (int)base[0].size();
            for (auto& row : base)
                if ((int)row.size() < width) row.resize(width, '0');
        }
        else {
            width = std::max(1, (int)std::floor(((maxB.x - minB.x) / tile) + 1e-4f));
            height = std::max(1, (int)std::floor(((maxB.y - minB.y) / tile) + 1e-4f));
            base.assign(height, std::string(width, '0'));
        }

        std::vector<std::string> grid = base;

        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                if (grid[y][x] == '4' || grid[y][x] == '3' ||
                    grid[y][x] == 'C' || grid[y][x] == 'B' ||
                    grid[y][x] == 'H' || grid[y][x] == '9')
                    grid[y][x] = '0';

        auto worldToGridTop = [&](float wx, float wy) {
            int gx = (int)std::floor(((wx - minB.x) / tile) + 1e-4f);
            int gy_from_bottom = (int)std::floor(((wy - minB.y) / tile) + 1e-4f);
            gx = clampi(gx, 0, width - 1);
            gy_from_bottom = clampi(gy_from_bottom, 0, height - 1);
            int gy = (height - 1) - gy_from_bottom;
            return std::pair<int, int>(gx, gy);
            };

        auto placeEnemyCode = [&](float wx, float wy, char code) {
            auto [gx, gy] = worldToGridTop(wx, wy);
            char under = base[gy][gx];
            if (under == '0' || under == '4' || under == 'C' || under == 'B' || under == 'H' || under == '9')
                grid[gy][gx] = code;
        };

        auto placeCrate = [&](float wx, float wy) {
            auto [gx, gy] = worldToGridTop(wx, wy);
            char under = base[gy][gx];
            if (under == '0' || under == '3')
                grid[gy][gx] = '3';
        };

        for (Entity e : ctx.GetEntities()) {
            const Transform* t = ctx.TryGetTransform(e);
            if (!t) continue;

            const std::string entName = app.GetEntityNameByEntity(e);

            // These are produced by MapGenerator::FromGrid() as merged wall colliders:
            if (entName.find("WallBlock_") != std::string::npos) {
                // Fill all grid cells covered by this wall block’s rectangle.
                const Vector2 pos = t->GetPosition();
                const Vector2 size = t->GetScale();

                int gx0, gy0;
                std::tie(gx0, gy0) = worldToGridTop(pos.x, pos.y);

                int gx1, gy1;
                std::tie(gx1, gy1) = worldToGridTop(pos.x + size.x - 1e-4f, pos.y + size.y - 1e-4f);

                // ensure ordering
                int minx = std::min(gx0, gx1), maxx = std::max(gx0, gx1);
                int miny = std::min(gy0, gy1), maxy = std::max(gy0, gy1);

                for (int yy = miny; yy <= maxy; ++yy)
                    for (int xx = minx; xx <= maxx; ++xx)
                        grid[yy][xx] = '1';

                continue;
            }

            const std::string tag = app.GetPrefabTag(e);

            if (tag == "Enemy" || tag == "EnemyProjectile")
                placeEnemyCode(t->GetPosition().x, t->GetPosition().y, '4');
            else if (tag == "EnemyContact")
                placeEnemyCode(t->GetPosition().x, t->GetPosition().y, 'C');
            else if (tag == "burrow_mini_boss")
                placeEnemyCode(t->GetPosition().x, t->GetPosition().y, 'B');
            else if (tag == "heal_mini_boss")
                placeEnemyCode(t->GetPosition().x, t->GetPosition().y, 'H');
            else if (tag == "Ranged_mini_Boss")
                placeEnemyCode(t->GetPosition().x, t->GetPosition().y, '9');
            else if (tag == "Crate")
                placeCrate(t->GetPosition().x, t->GetPosition().y);
            else if (tag == "Wall" || tag == "WallVariant") {
                auto [gx, gy] = worldToGridTop(t->GetPosition().x, t->GetPosition().y);
                if (gy >= 0 && gy < (int)grid.size() && gx >= 0 && gx < (int)grid[gy].size()) {
                    grid[gy][gx] = '1';
                }
            }
        }

        std::ofstream out(txtPath);
        if (!out.is_open()) {
            DebugConsole::Get().Error("[SceneLoader] Failed to open " + txtPath + " for write.\n");
            return;
        }
        for (int y = 0; y < height; ++y) {
            if ((int)grid[y].size() < width) grid[y].resize(width, '0');
            out << grid[y] << "\n";
        }
    }


}

namespace SceneRuntime {

    /**
    * @brief Loads a scene from JSON or TXT, replacing all non-persistent entities.
    *
    * Steps:
    *  - Remove all entities except those tagged as persistent
    *  - Clear prefab defaults and door data
    *  - Resolve asset path if needed
    *  - Call ResourceManager::LoadLevel() to populate entities
    *  - Sync editor fields (editScenePath / playScenePath)
    *
    * @param path Path to level JSON or TXT.
    *
    * @note Editor mode and Play mode maintain distinct level path tracking.
    */
    bool LoadScene(GameApp& app, const std::string& path, bool skipPuzzleSave) {
        // 0) Save puzzle states before destroying old entities
        if (!skipPuzzleSave && app.GetPuzzleSystem()) {
            app.GetPuzzleSystem()->SaveAllPuzzleStates();
        }

        // 1) clear runtime state
        app.ClearEnemiesControllers();
        app.DestroyAllNonPersistentEntities();
        app.ClearSpawnedEntitiesSelection();
        app.ClearPrefabDefaults();
        app.GetDoorSystem().ClearDoors();

        // 2) resolve asset path consistently
        std::string fullPath = path;
        if (fullPath.rfind("Assets/", 0) != 0 && fullPath.rfind(ASSET_ROOT_DIR, 0) != 0)
            fullPath = AssetPath(fullPath);

        // 3) actual file->ECS load
        bool success = SceneLoader::LoadLevel(app, fullPath);

        // 4) Restore puzzle states for newly loaded entities
        if (!skipPuzzleSave && success && app.GetPuzzleSystem()) {
            app.GetPuzzleSystem()->RestoreAllPuzzleStates();
        }

        return success;
    }


    bool LoadGridTxt(const std::string& path,
        std::vector<std::string>& outGrid,
        int* outDoorX,
        int* outDoorY)
    {
        outGrid.clear();
        if (outDoorX) *outDoorX = -1;
        if (outDoorY) *outDoorY = -1;

        std::ifstream f(path);
        if (!f.is_open()) return false;

        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            // Convert whitespace to VOID
            for (char& c : line) {
                if (c == ' ' || c == '\t')
                    c = '.';
            }

            if (!line.empty())
                outGrid.push_back(line);
        }

        if (outGrid.empty()) return false;

        // Normalize width using VOID
        size_t w = 0;
        for (auto& r : outGrid)
            w = std::max(w, r.size());

        for (auto& r : outGrid)
            if (r.size() < w)
                r.resize(w, '.');

        // Validate characters
        auto isValid = [](char c) {
            switch (c) {
            case '.': case '0': case '1':
            case '2': case '3': case '4':
            case '6': case '7': case '8': case '9':
            case 'C': case 'B': case 'H':
            case 'P': case '#': case 'L': case 'D': case 'M': case 'F':
                return true;
            default:
                return false;
            }
        };

        for (auto& r : outGrid) {
            for (char& c : r) {
                if (!isValid(c))
                    c = '.'; // fail-safe to void
            }
        }

        // Find first door
        if (outDoorX && outDoorY) {
            for (int y = 0; y < (int)outGrid.size(); ++y) {
                for (int x = 0; x < (int)outGrid[y].size(); ++x) {
                    if (outGrid[y][x] == '2') {
                        *outDoorX = x;
                        *outDoorY = y;
                        return true;
                    }
                }
            }
        }

        return true;
    }

}
