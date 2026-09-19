#pragma once
/**
* @file		  EditorShared.h
* @author     Woh Kye Le
* @email      w.kyele
* @date		  2026 - 01 - 27
*
* @brief      Shared helpers/data for editors (room variants, etc.)
*
* @version 1.0
* @copyright
* Copyright (C) 2026 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include <string>
#include <vector>

#include "Core/component.h"

class IComponentContext;
class UndoRedoManager;

/**
 * @struct VariantPlacement
 * @brief Represents a single variant entity placement in the editor grid.
 *
 * Contains all necessary information to position and configure a variant entity
 * in the scene, including grid coordinates, visual properties, and collision settings.
 */
struct VariantPlacement {
    std::string type;        // e.g. "WallTile", "Enemy", etc.
    std::string name;        // e.g. "V_WallTile_10_5"
    int gx = 0, gy = 0;      // grid coords (top-down like your editor grid indexing)
    std::string textureKey;  // e.g. "wall_TL"
    bool solid = true;
    bool hasSignature = false;
    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float colliderSize[2] = { 0.0f, 0.0f }; // 0,0 means use default
    bool removed = false;

    // --- Puzzle override data ---
    bool hasPuzzleData = false;
    int puzzleKind = 0;      // cast from PuzzleKind
    int puzzleGroupId = 0;
    bool puzzleActive = true;

    int pi0 = 0, pi1 = 0, pi2 = 0, pi3 = 0;
    float pf0 = 0.f, pf1 = 0.f, pf2 = 0.f, pf3 = 0.f;
    std::string ps0;
    std::string ps1;

    // Animator properties
    int animStart = 0;
    int animEnd = 0;
    float animSpeed = 0.0f; // 0 means use default
};

/**
 * Handles serialization/deserialization of variant placements to/from JSON format
 * and manages coordinate system conversion between editor and game space.
 */
namespace Variant {

    /**
     * @brief Loads variant placements from a JSON file.
     *
     * Parses a JSON file containing variant objects and converts them to
     * VariantPlacement structures. Handles Y-coordinate inversion between
     * editor JSON storage and game world coordinates.
     *
     * @param path       Path to the JSON variant file
     * @param gridHeight Height of the navigation grid (used for Y-coordinate inversion)
     * @param out        Output vector to receive parsed variant placements
     *
     * @return bool True if file was successfully loaded or doesn't exist (empty variants),
     *              false if file exists but has invalid format
     */

    bool LoadFile(const std::string& path,
        int gridHeight,
        std::vector<VariantPlacement>& out);

    /**
     * @brief Saves both navigation grid and variant data to disk.
     *
     * Performs a synchronized save operation that:
     * 1. Updates the navigation grid with variant wall placements
     * 2. Saves the updated navigation grid as a .txt file
     * 3. Saves variant placements as a .json file with unique naming
     *
     * @param txtPath    Path for the navigation grid .txt output file
     * @param jsonPath   Path for the variant placements .json output file
     * @param sceneStem  Base scene name (e.g., "entities_Level1")
     * @param tileSize   Size of each grid cell in world units
     * @param grid       Reference to the navigation grid (modified in-place)
     * @param variants   Vector of variant placements to save
     *
     * @return bool True if both files were saved successfully, false otherwise
     */
    bool SaveRoomData(const std::string& txtPath,
        const std::string& jsonPath,
        const std::string& sceneStem,
        float tileSize,
        std::vector<std::string>& grid,
        const std::vector<VariantPlacement>& variants);

}

/**
 * @struct LightRecord
 * @brief Serializable snapshot of a LightComponent for editor persistence.
 *
 * This structure is used by editor tooling to persist light settings to a
 * sidecar JSON file (e.g. `<scene>_lights.json`). 

 * - @ref name (optional exact match)
 * - @ref prefabTag (optional filter)
 * - @ref position (nearest-match within a tolerance)
 */
struct LightRecord {
    std::string name;
    std::string prefabTag;
    int gx = -1;
    int gy = -1;
    float position[2] = { 0.0f, 0.0f };

    bool enabled = true;

    bool glowEnabled = true;
    float glowRadius = 180.0f;
    float glowColor[3] = { 1.0f, 0.85f, 0.55f };
    float glowOffset[2] = { 0.0f, 0.0f };
    float glowIntensity = 1.0f;
    float glowOpacity = 0.35f;
    float glowSoftness = 2.0f;

    bool sourceEnabled = false;
    float sourceRadius = 180.0f;
    float sourceColor[3] = { 1.0f, 0.85f, 0.55f };
    float sourceOffset[2] = { 0.0f, 0.0f };
    float sourceIntensity = 1.0f;
    float sourceOpacity = 0.35f;
    float sourceAttenuation = 1.0f;

    bool emberEnabled = false;
    float emberRate = 20.0f;
    float emberParticleLife = 0.8f;
    float emberVelMin[2] = { -20.0f, 30.0f };
    float emberVelMax[2] = { 20.0f, 60.0f };
    float emberColorStart[3] = { 1.0f, 0.8f, 0.5f };
    float emberColorEnd[3] = { 0.3f, 0.1f, 0.0f };
    float emberSizeStart = 6.0f;
    float emberSizeEnd = 1.0f;
    bool emberAdditive = true;
    float emberOffset[2] = { 0.0f, 0.0f };

    bool flickerEnabled = false;
    float flickerIntensityMin = 0.8f;
    float flickerIntensityMax = 1.0f;
    float flickerSpeed = 5.0f;
    float flickerTime = 0.0f;
};

/**
 * Shared ImGui drawing helpers for light effects.
 */
namespace LightEffects {
    /**
     * @brief Load light records from a JSON file.
     *
     * Accepts the current sidecar schema:
     * @code{.json}
     * { "version": 1, "scene": "<stem>", "lights": [ ... ] }
     * @endcode
     *
     * @param path Path to the light JSON file.
     * @param out  Output list of parsed light records (cleared first).
     * @return true if the file was read successfully or does not exist; false
     *         if the file exists but is invalid/unparseable.
     */
    bool LoadFile(const std::string& path, std::vector<LightRecord>& out);

    /**
     * @brief Save light records to a JSON file.
     *
     * Writes the current sidecar schema with a top-level `"lights"` array.
     *
     * @param path      Path to the light JSON file to write.
     * @param sceneStem Scene stem written into the `"scene"` field for debugging.
     * @param records   Light records to serialize.
     * @return true if the file was written successfully; false otherwise.
     */
    bool SaveFile(const std::string& path, const std::string& sceneStem, const std::vector<LightRecord>& records);

#if ENABLE_EDITOR
    /**
     * @brief Draws the shared "Light Effects" tab content.
     *
     * If the selected entity does not have a LightComponent, this UI provides a
     * button to add one.
     *
     * @param ctx      Component context used to read/write LightComponent data.
     * @param e        Entity being edited.
     * @param undoRedo Optional undo/redo manager (records before/after snapshots).
     * @return true if any property was changed this frame; false otherwise.
     */
    bool DrawLightEffectsTab(IComponentContext& ctx, Entity e, UndoRedoManager* undoRedo);
#endif
}
