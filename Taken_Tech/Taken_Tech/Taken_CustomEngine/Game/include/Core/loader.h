	#pragma once
/**
 * @file       loader.h
 * @author     Jethro Sung
 * @co_author  Low JianLin, Sng Swee Yong Dillon
 * @email      sung.h, jianlin.low,sweeyongdillon.sng
 * @date       2025-09-29
 *
 * @brief    Declares the entity loader interface for populating the GameApp from JSON files.
 *
 * This loader is responsible for instantiating runtime assets and entities from
 * editable JSON files. It supports multiple asset categories:
 *
 *  - Textures       -> GPU-loaded 2D textures (PNG)
 *  - Audio          -> Sound effects and BGM registered to AudioManager
 *  - Sprite Sheets  -> Texture atlases with animation frame data
 *  - Entities       -> Prefab-driven ECS component creation
 *
 * Implementation relies on GameApp for:
 *  - Access to entity factories (e.g., MakePlayer, MakeEnemy)
 *  - Physics setup (Colliders, Force Proxy)
 *  - SpriteAnimator + MeshRenderer binding
 *
 * @version 1.0
 * @copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include <string>
#include <vector>
#include <cstdint>

using Entity = std::uint32_t;

class GameApp;
class IComponentContext;

namespace SceneLoader{
    
    /**
     * @brief Load a JSON scene file into the game and apply light overrides.
     *
     * This creates entities/components from the scene JSON, then (if present)
     * loads a light sidecar file `<sceneStem>_lights.json` from `Assets/scene/`
     * and applies its LightComponent values to matching entities.
     *
     * @param app       Target GameApp instance to populate.
     * @param sceneFile Scene JSON path (e.g. "Assets/scene/entities_Level1.json").
     * @return true if scene JSON was loaded successfully; false otherwise.
     */
    bool LoadLevel(GameApp& app, const std::string& sceneFile);

    /**
     * @brief Save the current level to disk.
     *
     * When @p path ends with ".txt", this saves the labyrinth/room layout text
     * representation and also writes a light sidecar file to `Assets/scene/`.
     *
     * When @p path ends with ".json", this saves the scene JSON and also writes
     * a light sidecar file next to the scene (`<sceneStem>_lights.json`).
     *
     * @param context ECS context providing component access.
     * @param path    Destination file path (.json or .txt).
     */
    void SaveLevel(const IComponentContext& context, const std::string& path, const std::vector<Entity>* entitiesToSave = nullptr);

    /**
     * @brief Save only the light sidecar JSON for a scene.
     *
     * This does not modify the scene JSON or any labyrinth/room grid text file.
     * The output is always written to:
     * `Assets/scene/<sceneStem>_lights.json`
     *
     * @param context  ECS context providing LightComponent access.
     * @param sceneFile Scene JSON path used to derive `<sceneStem>`.
     */
    void SaveLightSidecar(const IComponentContext& context, const std::string& sceneFile);

    /**
     * @brief Apply LightComponent overrides from a scene's sidecar light JSON.
     *
     * Reads `Assets/scene/<sceneStem>_lights.json` (if present) and applies the
     * serialized values onto matching entities in the active ECS.
     *
     * @param app       GameApp whose ECS will be updated.
     * @param sceneFile Scene JSON path used to derive `<sceneStem>`.
     */
    void ApplyLightOverrides(GameApp& app, const std::string& sceneFile);

    void SaveLabyrinthLayout(const IComponentContext& ctx, const GameApp& app, const std::string& txtPath);

}

namespace SceneRuntime {

    /**
     * Load a scene from a JSON level file. Removes all curr entities and loads new ones using the loader sys.
     * Called from the ImGui editor's lvl selector. Path to the level JSON file (e.g., "Assets/scene/entities_Level2.json").
     */
    bool LoadScene(GameApp& app, const std::string& sceneFile, bool skipPuzzleSave = false);

    /*
    * @brief Load a room grid from a text file.
    * @param path Path to the text file containing the grid layout.
    * @param outGrid Output vector of strings representing the grid rows.
    * @param outDoorX Optional pointer to store the door's X coordinate.
    * @param outDoorY Optional pointer to store the door's Y coordinate.
    * @return true if the file was successfully loaded, false otherwise.
    */
    bool LoadGridTxt(const std::string& path,
        std::vector<std::string>& outGrid,
        int* outDoorX,
        int* outDoorY);
}


/**
 * @brief Loads entities from a JSON-like file and adds them to the given GameApp instance.
 *
 * This function parses a simple line-based JSON-like entity description file. For each
 * entity object found, the appropriate factory function (`MakePlayer` or `MakeGameObject`)
 * is called to create and register the entity with the game. The loader supports a variety
 * of fields for both Player and GameObject types:
 *
 * - **Common fields** (Player & GameObject):
 *   - `"type"`: `"Player"` or `"GameObject"`
 *   - `"name"`: Entity name string
 *   - `"meshIndex"`: Index of the mesh to use
 *   - `"position"`: `[x, y]` world position
 *   - `"scale"`: `[x, y]` scale factors
 *   - `"rotation"`: Rotation in radians
 *   - `"color"`: `[r, g, b]` per-entity color
 *   - `"meshExtent"`: `[x, y]` mesh local size for scaling
 *
 * - **GameObject-only fields**:
 *   - `"colliderType"`: `"Box"`, `"Circle"`, or `"Triangle"`
 *   - `"isTrigger"`: `0` or `1` indicating if the collider is a trigger
 *
 * Debug information is printed to the console for each entity loaded.
 *
 * @param app      Reference to the running GameApp where the entities will be spawned.
 * @param filename Path to the JSON-like entity file (e.g., `"Assets/entities.json"`).
 * @return true if the file was successfully opened and parsed, false otherwise.
 */
bool LoadEntitiesJSON(GameApp& app, const std::string& filename);


/**
 * @brief Loads an audio profile from a JSON file and applies it at runtime.
 *
 * This function parses a structured JSON file to (re)configure the audio system
 * without touching the startup defaults in `config.json`. It can update bus
 * volumes, register/override sound assets by logical key, and optionally start
 * a background-music (BGM) track. All file paths are expected to be relative
 * to the `Assets/` directory.
 *
 *
 * - `"volumes"` (optional): per-bus gain in decibels
 *   - `"masterDb"`: float, e.g. `0.0`
 *   - `"musicDb"`:  float, e.g. `-6.0`
 *   - `"sfxDb"`:    float, e.g. `0.0`
 *
 * - `"sounds"` (optional): map of **logical key → relative path**
 *   ```json
 *   "sounds": {
 *     "door_open":  "audio/Door Open Only FX v1.1.mp3",
 *     "door_close": "audio/Door Close Only FX.mp3",
 *     "button":     "audio/Button Push FX.mp3",
 *     "battleA":    "audio/battleThemeA.mp3"
 *   }
 *   ```
 *   Each entry is loaded (or reloaded) into memory and registered under its key.
 *   If a key already exists, the old sound is unloaded before the new one is loaded.
 *
 *   - `"bgm"` (optional): description of the BGM to start after loading
 *   - `"key"`:  logical key to play (will be loaded if not already present)
 *   - `"file"`: relative path to the audio file (used if the key isn’t loaded yet)
 *   - `"loop"`: boolean, default `true`
 *   - `"gain"`: float linear gain, default `0.9`
 *
 * **Behavior**:
 * - If @p applyVolumes is true and `"volumes"` exists, the Master/Music/Sfx bus
 *   levels are updated immediately (in dB).
 * - If `"sounds"` exists, all listed assets are loaded and registered via
 *   `ResourceManager::LoadAudio(key, "Assets/" + file)`.
 * - If @p startBgmIfAny is true and `"bgm"` exists, the current music is faded out
 *   (short global fade) and the new BGM is started on the Music bus using the given
 *   loop/gain settings.
 *
 * **Example**:
 * ```json
 * {
 *   "volumes": { "masterDb": 0.0, "musicDb": -6.0, "sfxDb": 0.0 },
 *   "sounds":  { "door_open": "audio/Door Open Only FX v1.1.mp3" },
 *   "bgm":     { "key": "battleA", "file": "audio/battleThemeA.mp3", "loop": true, "gain": 0.9 }
 * }
 * ```
 *
 * Debug information is printed to the console for any file/JSON errors or missing keys.
 *
 * @param filename          Path to the JSON audio profile (e.g., `"Assets/audio.json"`).
 * @param applyVolumes      If true, apply `"volumes"` to audio buses when present.
 * @param startBgmIfAny     If true, start the `"bgm"` track when present.
 * @return true if the file was opened and parsed successfully (and actions attempted),
 *         false if the file couldn’t be opened or the JSON failed to parse.
 */
bool LoadAudioJSON(const std::string& filename, bool applyVolumes = true, bool startBgmIfAny = true);


/**
 * @brief Load textures from a JSON file and register them in ResourceManager.
 *
 * Expected JSON shape:
 * @code{.json}
 * { "textures": { "logicalName": "relative/path.png", "...": "..." } }
 * @endcode
 *
 * Paths are resolved relative to the JSON file's directory.
 *
 * @param filename Path to the JSON file.
 * @return true if the JSON was parsed and at least one texture was loaded (or no failures);
 *         false on file/JSON errors.
 */
bool LoadTexturesJSON(const std::string& filename);

/**
 * @brief Load sprite sheets from a JSON file and register them in ResourceManager.
 *
 * Expected JSON shape:
 * @code{.json}
 * { "sheets": [
 *   { "name": "run", "file": "atlases/run.png", "frameW": 40, "frameH": 55, "cols": 5, "rows": 1, "frameDuration": 0.08 }
 * ] }
 * @endcode
 *
 * Paths are resolved relative to the JSON file's directory.
 *
 * @param filename Path to the JSON file.
 * @return true if the JSON was parsed and at least one sheet was registered (or no failures);
 *         false on file/JSON errors.
 */
bool LoadSpriteSheetsJSON(const std::string& filename);
