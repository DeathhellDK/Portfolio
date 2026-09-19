/**
* @file		  EditorShared.cpp
* @author     Woh Kye Le
* @email      w.kyele
* @date		  2026 - 01 - 27
*
* @brief Shared editor helpers: room variants JSON load/save.
*
* @version 1.0
* @copyright
* Copyright (C) 2026 DigiPen Institute of Technology.
*/

#include "Editor/EditorShared.h"

#if ENABLE_EDITOR
#include <imgui.h>
#endif

#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_set>

#include "Core/componentcontext.h"
#include "Editor/UndoRedoManager.h"
#include "Light/lightComponent.h"

#include <json.hpp>
using json = nlohmann::json;

namespace {
    /**
     * @brief Internal helper to read a JSON file.
     *
     * @param path Path to the JSON file
     * @param out  JSON object to populate with file contents
     *
     * @return bool True if file was successfully read and parsed, false otherwise
     */
    bool ReadJsonFile_(const std::string& path, json& out) {
        std::ifstream in(path);
        if (!in.is_open())
            return false;

        try { in >> out; }
        catch (...) { return false; }

        return true;
    }

    /**
     * @brief Internal helper to write a JSON object to file.
     *
     * Creates parent directories if needed and writes formatted JSON.
     *
     * @param path Path for the output JSON file
     * @param j    JSON object to write
     *
     * @return bool True if file was successfully written, false otherwise
     */
    bool WriteJsonFile_(const std::string& path, const json& j) {

        std::error_code ec;
        std::filesystem::path p(path);
        if (!p.parent_path().empty())
            std::filesystem::create_directories(p.parent_path(), ec);

        std::ofstream out(path);
        if (!out.is_open())
            return false;

        out << j.dump(2);
        return true;
    }

    /**
     * @brief Parses a single variant object from JSON.
     *
     * Converts JSON object to VariantPlacement structure, handling
     * Y-coordinate inversion between editor and game coordinate systems.
     *
     * @param o          JSON object containing variant data
     * @param gridHeight Height of the navigation grid for Y-coordinate conversion
     *
     * @return VariantPlacement Parsed variant placement data
     */
    VariantPlacement ParseObject_(const json& o, int gridHeight) {
        VariantPlacement vp{};
        vp.type = o.value("type", "WallTile");
        vp.name = o.value("name", "");
        vp.gx = o.value("gx", 0);

        int jsonGy = o.value("gy", 0);
        vp.gy = (gridHeight - 1) - jsonGy;

        vp.textureKey = o.value("textureKey", "");
        vp.solid = o.value("solid", true);
        vp.hasSignature = o.value("hasSignature", false);
        vp.removed = o.value("removed", false);

        if (o.contains("color")) {
            auto& c = o["color"];
            if (c.is_array() && c.size() == 4) {
                vp.color[0] = c[0]; vp.color[1] = c[1]; vp.color[2] = c[2]; vp.color[3] = c[3];
            }
        }
        if (o.contains("colliderSize")) {
            auto& s = o["colliderSize"];
            if (s.is_array() && s.size() == 2) {
                vp.colliderSize[0] = s[0]; vp.colliderSize[1] = s[1];
            }
        }

        if (o.contains("puzzle")) {
            const auto& p = o["puzzle"];
            vp.hasPuzzleData = true;
            vp.puzzleKind = p.value("kind", 0);
            vp.puzzleGroupId = p.value("groupId", 0);
            vp.puzzleActive = p.value("active", true);

            vp.pi0 = p.value("i0", 0);
            vp.pi1 = p.value("i1", 0);
            vp.pi2 = p.value("i2", 0);
            vp.pi3 = p.value("i3", 0);

            vp.pf0 = p.value("f0", 0.0f);
            vp.pf1 = p.value("f1", 0.0f);
            vp.pf2 = p.value("f2", 0.0f);
            vp.pf3 = p.value("f3", 0.0f);

            vp.ps0 = p.value("s0", std::string{});
            vp.ps1 = p.value("s1", std::string{});
        }

        vp.animStart = o.value("animStart", 0);
        vp.animEnd = o.value("animEnd", 0);
        vp.animSpeed = o.value("animSpeed", 0.0f);

        if (vp.name.empty())
            vp.name = "V_" + vp.type + "_" +
            std::to_string(vp.gx) + "_" +
            std::to_string(vp.gy);

        return vp;
    }

    /**
     * @brief Converts a VariantPlacement to JSON format.
     *
     * Serializes variant data for storage, including Y-coordinate inversion
     * back to editor coordinate system.
     *
     * @param vp         Variant placement to serialize
     * @param gridHeight Height of the navigation grid for Y-coordinate conversion
     *
     * @return json JSON object representing the variant placement
     */
    json ToObject_(const VariantPlacement& vp, int gridHeight) {
        json o;
        o["type"] = vp.type;
        o["name"] = vp.name.empty()
            ? ("V_" + vp.type + "_" +
                std::to_string(vp.gx) + "_" +
                std::to_string(vp.gy))
            : vp.name;

        o["gx"] = vp.gx;
        o["gy"] = (gridHeight - 1) - vp.gy;

        o["textureKey"] = vp.textureKey;
        o["solid"] = vp.solid;
        o["hasSignature"] = vp.hasSignature;
        o["color"] = { vp.color[0], vp.color[1], vp.color[2], vp.color[3] };
        o["colliderSize"] = { vp.colliderSize[0], vp.colliderSize[1] };

        if (vp.hasPuzzleData) {
            o["puzzle"] = {
                { "kind", vp.puzzleKind },
                { "groupId", vp.puzzleGroupId },
                { "active", vp.puzzleActive },
                { "i0", vp.pi0 },
                { "i1", vp.pi1 },
                { "i2", vp.pi2 },
                { "i3", vp.pi3 },
                { "f0", vp.pf0 },
                { "f1", vp.pf1 },
                { "f2", vp.pf2 },
                { "f3", vp.pf3 },
                { "s0", vp.ps0 },
                { "s1", vp.ps1 }
            };
        }

        o["animStart"] = vp.animStart;
        o["animEnd"] = vp.animEnd;
        o["animSpeed"] = vp.animSpeed;
        o["removed"] = vp.removed;
        return o;
    }

}


namespace Variant {
    /**
     * @brief Loads variant placements from a JSON file.
     *
     * Implementation of LoadFile that reads JSON, parses variant objects,
     * and performs coordinate system conversion.
     *
     * @param path       Path to the JSON variant file
     * @param gridHeight Height of the navigation grid (used for Y-coordinate inversion)
     * @param out        Output vector to receive parsed variant placements
     *
     * @return bool True if file was successfully loaded or doesn't exist (empty variants),
     *              false if file exists but has invalid format
     *
     * @details
     * Expected JSON structure:
     * {
     *   "version": 1,
     *   "scene": "entities_Level1",
     *   "tileSize": 100.0,
     *   "objects": [
     *     {
     *       "type": "WallTile",
     *       "name": "V_WallTile_10_5",
     *       "gx": 10,
     *       "gy": 5,
     *       "textureKey": "wall_TL",
     *       "solid": true
     *     }
     *   ]
     * }
     */
    bool LoadFile(const std::string& path,
        int gridHeight,
        std::vector<VariantPlacement>& out) {
        out.clear();

        json j;
        if (!ReadJsonFile_(path, j))
            return true;

        if (j.contains("objects")) {
            for (const auto& o : j["objects"])
                out.push_back(ParseObject_(o, gridHeight));
            return true;
        }

        return false;
    }


    /**
    * @brief Saves both the navigation grid (.txt) and room variants (.json).
    *
    * Synchronizes variant data back into the grid before writing to disk,
    * ensuring consistency between visual representation and navigation data.
    *
    * @param txtPath    Path for the navigation grid .txt output file
    * @param jsonPath   Path for the variant placements .json output file
    * @param sceneStem  Base scene name (e.g., "entities_Level1")
    * @param tileSize   Size of each grid cell in world units
    * @param grid       Reference to the navigation grid (modified in-place)
    * @param variants   Vector of variant placements to save
    *
    * @return bool True if both files were saved successfully, false otherwise
    *
    * @details
    * Operation steps:
    * 1. Sync WallTile variants into grid as '1' characters
    * 2. Save updated navigation grid to .txt file
    * 3. Generate unique names for all variants
    * 4. Convert variants to JSON with Y-coordinate inversion
    * 5. Save JSON to disk with pretty formatting
    */
    bool SaveRoomData(
        const std::string& txtPath,
        const std::string& jsonPath,
        const std::string& sceneStem,
        float tileSize,
        std::vector<std::string>& grid,
        const std::vector<VariantPlacement>& variants) {

        const int gridHeight = (int)grid.size();

        // Sync variants into grid 
        for (const auto& v : variants) {
            if (v.type != "WallTile") continue;

            const int row = (gridHeight - 1) - v.gy;

            if (row >= 0 && row < gridHeight &&
                v.gx >= 0 && v.gx < (int)grid[row].size())
            {
                // don't overwrite door marker
                if (grid[row][v.gx] == '2')
                    continue;

                if (v.removed)
                    grid[row][v.gx] = '0';
                else
                    grid[row][v.gx] = '1';
            }
        }

        // Save TXT grid
        if (!txtPath.empty()) {
            std::ofstream out(txtPath);
            if (!out.is_open())
                return false;

            for (const auto& row : grid)
                out << row << "\n";
        }

        // Save variants JSON
        if (!jsonPath.empty()) {
            json j;
            j["version"] = 1;
            j["scene"] = sceneStem;
            j["tileSize"] = tileSize;

            std::vector<json> objects;
            objects.reserve(variants.size());

            std::unordered_set<std::string> usedNames;

            for (const auto& v : variants) {
                VariantPlacement vp = v;
                //  unique names
                if (vp.name.empty())
                    vp.name = "V_" + vp.type + "_" + std::to_string(vp.gx) + "_" + std::to_string(vp.gy);

                std::string base = vp.name;
                int suffix = 1;
                while (usedNames.count(vp.name))
                    vp.name = base + "_" + std::to_string(suffix++);

                usedNames.insert(vp.name);

                objects.push_back(ToObject_(vp, gridHeight));
            }

            j["objects"] = std::move(objects);

            if (!WriteJsonFile_(jsonPath, j))
                return false;
        }

        return true;
    }

}

/**
 * @brief Light serialization and LightComponent ImGui editing helpers.
 *
 * This file contains the implementation of the LightEffects utilities declared in
 * EditorShared.h:
 */
namespace LightEffects {
    namespace {
        /**
         * @brief Parse a LightRecord from a JSON light object.
         *
         * @param o JSON object describing a single light record.
         * @return Parsed LightRecord.
         */
        LightRecord ParseLight_(const json& o) {
            LightRecord r{};
            r.name = o.value("name", std::string{});
            r.prefabTag = o.value("prefabTag", std::string{});
            r.gx = o.value("gx", -1);
            r.gy = o.value("gy", -1);

            if (o.contains("position") && o["position"].is_array() && o["position"].size() == 2) {
                r.position[0] = o["position"][0].get<float>();
                r.position[1] = o["position"][1].get<float>();
            }

            r.enabled = o.value("enabled", true);

            if (o.contains("glow") && o["glow"].is_object()) {
                const auto& g = o["glow"];
                r.glowEnabled = g.value("enabled", r.glowEnabled);
                r.glowRadius = g.value("radius", r.glowRadius);
                r.glowIntensity = g.value("intensity", r.glowIntensity);
                r.glowOpacity = g.value("opacity", r.glowOpacity);
                r.glowSoftness = g.value("softness", r.glowSoftness);
                if (g.contains("color") && g["color"].is_array() && g["color"].size() == 3) {
                    r.glowColor[0] = g["color"][0].get<float>();
                    r.glowColor[1] = g["color"][1].get<float>();
                    r.glowColor[2] = g["color"][2].get<float>();
                }
                if (g.contains("offset") && g["offset"].is_array() && g["offset"].size() == 2) {
                    r.glowOffset[0] = g["offset"][0].get<float>();
                    r.glowOffset[1] = g["offset"][1].get<float>();
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
                    r.sourceColor[0] = s["color"][0].get<float>();
                    r.sourceColor[1] = s["color"][1].get<float>();
                    r.sourceColor[2] = s["color"][2].get<float>();
                }
                if (s.contains("offset") && s["offset"].is_array() && s["offset"].size() == 2) {
                    r.sourceOffset[0] = s["offset"][0].get<float>();
                    r.sourceOffset[1] = s["offset"][1].get<float>();
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
                    r.emberVelMin[0] = e["velMin"][0].get<float>();
                    r.emberVelMin[1] = e["velMin"][1].get<float>();
                }
                if (e.contains("velMax") && e["velMax"].is_array() && e["velMax"].size() == 2) {
                    r.emberVelMax[0] = e["velMax"][0].get<float>();
                    r.emberVelMax[1] = e["velMax"][1].get<float>();
                }
                if (e.contains("colorStart") && e["colorStart"].is_array() && e["colorStart"].size() == 3) {
                    r.emberColorStart[0] = e["colorStart"][0].get<float>();
                    r.emberColorStart[1] = e["colorStart"][1].get<float>();
                    r.emberColorStart[2] = e["colorStart"][2].get<float>();
                }
                if (e.contains("colorEnd") && e["colorEnd"].is_array() && e["colorEnd"].size() == 3) {
                    r.emberColorEnd[0] = e["colorEnd"][0].get<float>();
                    r.emberColorEnd[1] = e["colorEnd"][1].get<float>();
                    r.emberColorEnd[2] = e["colorEnd"][2].get<float>();
                }
                if (e.contains("offset") && e["offset"].is_array() && e["offset"].size() == 2) {
                    r.emberOffset[0] = e["offset"][0].get<float>();
                    r.emberOffset[1] = e["offset"][1].get<float>();
                }
            }

            if (o.contains("flicker") && o["flicker"].is_object()) {
                const auto& f = o["flicker"];
                r.flickerEnabled = f.value("enabled", r.flickerEnabled);
                r.flickerIntensityMin = f.value("intensityMin", r.flickerIntensityMin);
                r.flickerIntensityMax = f.value("intensityMax", r.flickerIntensityMax);
                r.flickerSpeed = f.value("speed", r.flickerSpeed);
                r.flickerTime = f.value("time", r.flickerTime);
            }

            return r;
        }

        /**
         * @brief Serialize a LightRecord into a JSON object.
         *
         * @param r LightRecord to serialize.
         * @return JSON object for a single light record.
         */
        json ToLightJson_(const LightRecord& r) {
            json o;
            if (!r.name.empty()) o["name"] = r.name;
            if (!r.prefabTag.empty()) o["prefabTag"] = r.prefabTag;
            if (r.gx >= 0) o["gx"] = r.gx;
            if (r.gy >= 0) o["gy"] = r.gy;
            o["position"] = { r.position[0], r.position[1] };
            o["enabled"] = r.enabled;

            o["glow"] = {
                { "enabled", r.glowEnabled },
                { "radius", r.glowRadius },
                { "color", { r.glowColor[0], r.glowColor[1], r.glowColor[2] } },
                { "offset", { r.glowOffset[0], r.glowOffset[1] } },
                { "intensity", r.glowIntensity },
                { "opacity", r.glowOpacity },
                { "softness", r.glowSoftness }
            };

            o["source"] = {
                { "enabled", r.sourceEnabled },
                { "radius", r.sourceRadius },
                { "color", { r.sourceColor[0], r.sourceColor[1], r.sourceColor[2] } },
                { "offset", { r.sourceOffset[0], r.sourceOffset[1] } },
                { "intensity", r.sourceIntensity },
                { "opacity", r.sourceOpacity },
                { "attenuation", r.sourceAttenuation }
            };

            o["ember"] = {
                { "enabled", r.emberEnabled },
                { "rate", r.emberRate },
                { "particleLife", r.emberParticleLife },
                { "velMin", { r.emberVelMin[0], r.emberVelMin[1] } },
                { "velMax", { r.emberVelMax[0], r.emberVelMax[1] } },
                { "colorStart", { r.emberColorStart[0], r.emberColorStart[1], r.emberColorStart[2] } },
                { "colorEnd", { r.emberColorEnd[0], r.emberColorEnd[1], r.emberColorEnd[2] } },
                { "sizeStart", r.emberSizeStart },
                { "sizeEnd", r.emberSizeEnd },
                { "additive", r.emberAdditive },
                { "offset", { r.emberOffset[0], r.emberOffset[1] } }
            };

            o["flicker"] = {
                { "enabled", r.flickerEnabled },
                { "intensityMin", r.flickerIntensityMin },
                { "intensityMax", r.flickerIntensityMax },
                { "speed", r.flickerSpeed },
                { "time", r.flickerTime }
            };

            return o;
        }
    }

    /**
     * @brief Load light records from a JSON sidecar file.
     *
     * Schema (current):
     * @code{.json}
     * { "version": 1, "scene": "<stem>", "lights": [ { ... }, ... ] }
     * @endcode
     *
     * @param path Path to the light JSON file.
     * @param out Output list of parsed light records (cleared first).
     * @return true if loaded successfully or file is missing; false for invalid format.
     */
    bool LoadFile(const std::string& path, std::vector<LightRecord>& out) {
        out.clear();

        json j;
        if (!ReadJsonFile_(path, j))
            return true;

        const json* arr = nullptr;
        if (j.contains("lights") && j["lights"].is_array())
            arr = &j["lights"];
        else if (j.contains("objects") && j["objects"].is_array())
            arr = &j["objects"];

        if (!arr)
            return false;

        for (const auto& o : *arr)
            if (o.is_object())
                out.push_back(ParseLight_(o));

        return true;
    }

    /**
     * @brief Save light records to a JSON sidecar file.
     *
     * Writes:
     * - `"version"`: schema version
     * - `"scene"`: the caller-provided scene stem (for debugging/inspection)
     * - `"lights"`: array of serialized LightRecord values
     *
     * @param path Output path for the light JSON file.
     * @param sceneStem Scene stem written into the `"scene"` field.
     * @param records Light records to serialize.
     * @return true if the file was written successfully; false otherwise.
     */
    bool SaveFile(const std::string& path, const std::string& sceneStem, const std::vector<LightRecord>& records) {
        json j;
        j["version"] = 1;
        j["scene"] = sceneStem;

        std::vector<json> lights;
        lights.reserve(records.size());
        for (const auto& r : records)
            lights.push_back(ToLightJson_(r));

        j["lights"] = std::move(lights);

        return WriteJsonFile_(path, j);
    }
}

#if ENABLE_EDITOR
namespace LightEffects {
    /**
     * @brief Draw the shared LightComponent editor UI for an entity.
     *
     * @param ctx ECS context used to read/write LightComponent data.
     * @param e Entity being edited.
     * @param undoRedo Optional undo/redo manager.
     * @return true if any property was changed this frame; false otherwise.
     */
    bool DrawLightEffectsTab(IComponentContext& ctx, Entity e, UndoRedoManager* undoRedo) {
        LightComponent* l = ctx.GetLight(e);
        if (!l) {
            ImGui::TextDisabled("No LightComponent on selection.");
            if (ImGui::Button("Add LightComponent")) {
                ctx.AddLightComponent(e);
                l = ctx.GetLight(e);
            }
            if (!l)
                return false;
        }

        LightSnapshot before{};
        if (undoRedo)
            before = CaptureLightSnapshot(ctx, e);

        bool changed = false;

        changed |= ImGui::Checkbox("Light Enabled", &l->enabled);

        if (ImGui::CollapsingHeader("Glow", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID("GlowEffect");
            changed |= ImGui::Checkbox("Enabled", &l->glow.enabled);
            changed |= ImGui::DragFloat("Radius", &l->glow.radius, 1.0f, 0.0f, 2000.0f);
            changed |= ImGui::ColorEdit3("Color", &l->glow.color.x);
            changed |= ImGui::DragFloat2("Offset", &l->glow.offset.x, 0.1f);
            changed |= ImGui::DragFloat("Intensity", &l->glow.intensity, 0.01f, 0.0f, 20.0f);
            changed |= ImGui::DragFloat("Opacity", &l->glow.opacity, 0.01f, 0.0f, 1.0f);
            changed |= ImGui::DragFloat("Softness", &l->glow.softness, 0.01f, 0.0f, 20.0f);
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Source", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID("SourceEffect");
            changed |= ImGui::Checkbox("Enabled", &l->source.enabled);
            changed |= ImGui::DragFloat("Radius", &l->source.radius, 1.0f, 0.0f, 2000.0f);
            changed |= ImGui::ColorEdit3("Color", &l->source.color.x);
            changed |= ImGui::DragFloat2("Offset", &l->source.offset.x, 0.1f);
            changed |= ImGui::DragFloat("Intensity", &l->source.intensity, 0.01f, 0.0f, 20.0f);
            changed |= ImGui::DragFloat("Opacity", &l->source.opacity, 0.01f, 0.0f, 1.0f);
            changed |= ImGui::DragFloat("Attenuation", &l->source.attenuation, 0.01f, 0.0f, 20.0f);
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Ember", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID("EmberEffect");
            changed |= ImGui::Checkbox("Enabled", &l->ember.enabled);
            changed |= ImGui::DragFloat("Rate", &l->ember.rate, 0.1f, 0.0f, 1000.0f);
            changed |= ImGui::DragFloat("Particle Life", &l->ember.particleLife, 0.01f, 0.01f, 20.0f);
            changed |= ImGui::DragFloat2("Vel Min", &l->ember.velMin.x, 0.1f);
            changed |= ImGui::DragFloat2("Vel Max", &l->ember.velMax.x, 0.1f);
            changed |= ImGui::ColorEdit3("Start Color", &l->ember.colorStart.x);
            changed |= ImGui::ColorEdit3("End Color", &l->ember.colorEnd.x);
            changed |= ImGui::DragFloat("Start Size", &l->ember.sizeStart, 0.1f, 0.0f, 1000.0f);
            changed |= ImGui::DragFloat("End Size", &l->ember.sizeEnd, 0.1f, 0.0f, 1000.0f);
            changed |= ImGui::Checkbox("Additive", &l->ember.additive);
            changed |= ImGui::DragFloat2("Offset", &l->ember.offset.x, 0.1f);
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Flicker", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID("FlickerEffect");
            changed |= ImGui::Checkbox("Enabled", &l->flicker.enabled);
            changed |= ImGui::DragFloat("Intensity Min", &l->flicker.intensityMin, 0.01f, 0.0f, 10.0f);
            changed |= ImGui::DragFloat("Intensity Max", &l->flicker.intensityMax, 0.01f, 0.0f, 10.0f);
            changed |= ImGui::DragFloat("Speed", &l->flicker.speed, 0.01f, 0.0f, 50.0f);
            changed |= ImGui::DragFloat("Time", &l->flicker.time, 0.01f, 0.0f, 10000.0f);
            ImGui::PopID();
        }

        if (changed && undoRedo) {
            LightSnapshot after = CaptureLightSnapshot(ctx, e);
            undoRedo->Push_LightEdit(e, before, after);
        }

        return changed;
    }
}
#endif
