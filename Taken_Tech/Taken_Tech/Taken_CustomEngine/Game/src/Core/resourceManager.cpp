/**
 * @file      resourceManager.cpp
 * @author    Jethro Sung
 * @email     sung.h, w.kyele, jianlin.low,t.weiliangterril
 * @co-author Woh Kye Le, Low JianLin, Tan Wei Liang Terril
 * @date      2025-09-29
 *
 * @brief    Implementation of the ResourceManager class.
 *
 * This file defines the static functions of the ResourceManager, which handle
 * initialization, loading, retrieval, and shutdown of textures, sprite sheets,
 * audio files, and configuration data used by the game engine.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */

#include "Core/resourceManager.h"
#include <iostream>
#include <filesystem>
#include "Font/FontRenderer.h"
#include "Core/gameApp.h"
#include <json.hpp>
#include <fstream>
#include "Core/loader.h"
#include "Input/DebugConsole.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// -----------------------------------------------------------------------------
// Static member definitions
// -----------------------------------------------------------------------------
Config ResourceManager::cfg;
std::unordered_map<std::string, std::unique_ptr<Texture2D>> ResourceManager::textures;
std::unordered_map<std::string, SpriteSheet>                ResourceManager::sheets;
std::unordered_map<std::string, Audio::SoundID>             ResourceManager::audios;
bool ResourceManager::s_audioInited = false;
Audio::VoiceHandle ResourceManager::s_bgmHandle = 0;
std::unordered_map<Entity, Audio::VoiceHandle> ResourceManager::s_activeLoops;

// ------------------------------------------------------------------------------


namespace {
    // helper to round down safely
    inline int f2i_floor(float x) { return (int)std::floor(x + 0.0001f); }
}

// -----------------------------------------------------------------------------------
// ---------------------------------- Main API --------------------------------------- 
/**
    * @brief Initializes the resource system, loading configuration, textures,
    *        sprite sheets, fonts, and audio.
    *
    * Steps performed:
    *  1. Load config.json into the global Config object.
    *  2. Initialize graphics assets (textures, sprite sheets).
    *  3. Initialize audio engine (FMOD backend).
    *  4. Load audio profile from audio.json (volumes, sounds, BGM).
    *  5. Initialize the font system and register default fonts.
    *
    * If the caller supplied an AudioBootstrap with a BGM override,
    * that BGM is played after initialization.
    *
    * @param audioBoot Optional bootstrap settings controlling device audio,
    *                  max voices, and initial BGM.
    * @return true on success, false if config or asset loading fails.
*/
bool ResourceManager::Init(const AudioBootstrap* audioBoot) {

    const std::string root = ASSET_ROOT_DIR;

    // 1) load config file
    if (!LoadConfig(cfg, root + "/config.json")) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "File Type:[ResourceManager]\n", "Details: Failed to load config.json; using defaults.\n");
        return false;
    }

    // 2) Initialize Assets (sprite/spritesheets)
    if (!InitAssets(root)) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "File Type:[ResourceManager]\n", "Details: Failed to load assets\n");
        return false;
    }

    // 3) Initialize Audio
    if (!s_audioInited) {
        AudioBootstrap boot;
        if (audioBoot) boot = *audioBoot; // caller may override


        if (!InitAudio(boot)) {
            DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "File Type:[ResourceManager]\n", "Details: Audio init failed\n");
        }
        else {
            // 3a) After backend is ready, load ALL audio from Assets/audio.json
            //     Apply volumes from profile; start profile BGM only if caller didn't request a boot BGM.
            const bool startProfileBgm = false;
            LoadAudioProfile(root + "/audio.json", /*applyVolumes=*/true, /*startBgmIfAny=*/startProfileBgm);
        }
    }

    // 4) initialised font 
    EngineCore::FontRenderer::Init();
    EngineCore::FontRenderer::RegisterFont("default", root + "/fonts/VT323FontA.ttf");
    EngineCore::FontRenderer::RegisterFont("exit", root + "/fonts/RegularFontB.ttf");

    DebugConsole::Get().Info("[ResourceManager] Init complete.\n");
    return true;
}

/**
    * @brief Per-frame update for resource subsystems.
    *
    * Currently forwards to Audio::Update() to process FMOD streaming,
    * mixing, and voice management.
*/
void ResourceManager::Update() {
    if (s_audioInited) Audio::Update();
}

/**
    * @brief Shuts down the resource system and frees all loaded assets.
    *
    * Actions performed:
    *  - Clears texture and sprite-sheet maps (GPU objects freed via RAII).
    *  - Shuts down the audio backend and clears name→SoundID map.
    *  - Calls ShutDownAssets() to release graphics asset state.
*/
void ResourceManager::Shutdown() {

    textures.clear();
    sheets.clear();
    ShutdownAudio(); // calls the backend audio to release every Fmod::Sound 
    audios.clear(); // only clears ResourceManager's name (soundIDmap)

    //// Fully free texture and sheet maps
    //std::unordered_map<std::string, std::unique_ptr<Texture2D>>().swap(textures);
    //std::unordered_map<std::string, SpriteSheet>().swap(sheets);

    //// Shut down audio backend and free the name→SoundID map
    //ShutdownAudio();
    //std::unordered_map<std::string, Audio::SoundID>().swap(audios);
    ShutDownAssets();

}
// -----------------------------------------------------------------------------------



// -----------------------------------------------------------------------------------
// ----------------------------------- config ---------------------------------------- 
/**
 * @brief Get the global configuration loaded during Init().
 *
 * @return A const reference to the Config structure.
 */
const Config& ResourceManager::GetConfig() {
    return cfg;
}
// -----------------------------------------------------------------------------------



// -----------------------------------------------------------------------------------
// ------------------------------- graphics assets ----------------------------------- 

/**
 * @brief Load a texture from a path.
 *
 * @param abs path to the image file
 * @return std::unique_ptr<Texture2D> Owning pointer to a valid texture on
 *         success; nullptr if loading or GL upload failed.
 */
std::unique_ptr<Texture2D> ResourceManager::loadTexAbs(const std::string& abs) {
    auto tex = std::make_unique<Texture2D>();
    if (!tex->LoadFromFile(abs)) return nullptr;
    return tex;
}

/**
 * @brief Retrieve an OpenGL texture ID by name.
 *
 * @param name Name of the texture.
 * @return GLuint representing the texture ID, or 0 if not found.
 */
GLuint ResourceManager::GetTexture(const std::string& name) {
    /*if (auto it = textures.find(name); it != textures.end() && it->second)
        return it->second->ID();
    if (auto it2 = sheets.find(name); it2 != sheets.end())
        return it2->second.texture.ID();*/
    auto it = textures.find(name);
    if (it != textures.end() && it->second)
        return it->second->ID();

    auto it2 = sheets.find(name);
    if (it2 != sheets.end())
        return it2->second.texture.ID();


    return 0;
}

/**
 * @brief Retrieve the logical texture name given its OpenGL ID.
 *
 * @param id OpenGL texture ID.
 * @return std::string The logical texture name, or an empty string if not found.
 */
std::string ResourceManager::GetTextureNameByID(GLuint id) {
    for (const auto& [name, texPtr] : textures) {
        if (texPtr && texPtr->ID() == id)
            return name;
    }
    for (const auto& [name, sheet] : sheets) {
        if (sheet.texture.ID() == id)
            return name;
    }
    return "";
}

/**
 * @brief Retrieve a sprite sheet by its name.
 *
 * @param name Name of the sprite sheet.
 * @return Pointer to the SpriteSheet, or nullptr if not found.
 */
const SpriteSheet* ResourceManager::GetSpriteSheet(const std::string& name) {
    auto it = sheets.find(name);
    //if (auto it = sheets.find(name); it != sheets.end()) return &it->second;
    if (it != sheets.end())
        return &it->second;
    return nullptr;
}

/**
    * @brief Returns the internal map of all loaded textures.
    *
    * @return Const ref to name → Texture2D map.
*/
const std::unordered_map<std::string, std::unique_ptr<Texture2D>>& ResourceManager::GetAllTextures() {
    return textures;
}

/**
    * @brief Imports a new texture from disk and registers it under a name.
    *
    * Logs success or failure through DebugConsole.
    *
    * @param absPath Absolute path to image.
    * @param name    Logical texture name to register.
    * @return true on successful load.
*/
bool ResourceManager::ImportTexture(const std::string& absPath, const std::string& name)
{
    auto tex = loadTexAbs(absPath);
    if (!tex) {
        DebugConsole::Get().Error("[Assets] Failed to load texture: " + absPath + "\n");
        return false;
    }

    textures[name] = std::move(tex);
    DebugConsole::Get().Success("[Assets] Imported: " + name + "\n");
    return true;
}

/**
* @brief Initialize and preload built-in textures and sprite sheets.
*
*
* @param baseDir Root assets directory (e.g., "Assets").
* @return true Always returns true; individual files that fail to load are
*         logged to stderr and skipped.
* @note  names must be unique across textures and sheets.
*/
bool ResourceManager::InitAssets(const std::string& baseDir) {
    // Load textures.json
    if (!LoadTexturesJSON(baseDir + "/textures.json")) {
        DebugConsole::Get().Error("[ResourceManager] textures.json load had issues; continuing\n");
    }

    // Load spritesheets.json
    if (!LoadSpriteSheetsJSON(baseDir + "/spritesheets.json")) {
        DebugConsole::Get().Error("[ResourceManager] spritesheets.json load had issues; continuing\n");
    }

    DebugConsole::Get().Info("[ResourceManager] Loaded " + std::to_string(textures.size()) +
        " textures, " + std::to_string(sheets.size()) + " sheets.\n");
    return true;
}

/**
    * @brief Registers a sprite sheet under a name.
    *
    * @param name Name to register the sheet under.
    * @param sh   SpriteSheet data (texture + metadata).
    * @return true if name is valid.
*/
bool ResourceManager::RegisterSpriteSheet(const std::string& name, SpriteSheet&& sh) {
    if (name.empty()) return false;
    // If a sheet exists with same name, overwrite safely.
    sheets[name] = std::move(sh);
    return true;
}

/**
 * @brief Release all GPU/CPU resources managed by the graphics asset store.
 *
 * Clears the sprite-sheet and texture maps. Texture2D RAII destructors free
 * the OpenGL objects automatically.
 */
void ResourceManager::ShutDownAssets() {
    sheets.clear();
    textures.clear();
}
// -----------------------------------------------------------------------------------




// -----------------------------------------------------------------------------------
// ------------------------------------ Audio ---------------------------------------- 
/**
 * @brief Initializes the audio subsystem using the provided bootstrap settings
 * (configurable in gameApp.cpp - initialise :AudioBootstrap audioBoot{.......}).
 *
 * This function creates and configures the FMOD audio system using the given
 * device sample rate, channel count, and maximum number of active voices.
 * It also sets safe default bus levels for Master, Music, and SFX buses.
 *
 * If a background music name (`boot.bgmName`) is provided, this function will
 * automatically start playback of that track after initialization.
 *
 * @param boot Reference to an AudioBootstrap struct containing device and BGM setup info.
 * @return true if initialization succeeds, false otherwise.
 */
bool ResourceManager::InitAudio(const AudioBootstrap& boot) {
    if (s_audioInited) return true;

    Audio::Settings s;
    s.deviceSampleRate = boot.deviceSampleRate;
    s.deviceChannels = boot.deviceChannels;
    s.maxVoices = boot.maxVoices;

    if (!Audio::Initialise(s)) {
        DebugConsole::Get().Error("[ResourceManager] Audio init failed\n");
        return false;
    }

    // Safe default bus levels (tweak if you want it in config)
    Audio::SetBusDb(Audio::Bus::Master, 0.0f);
    Audio::SetBusDb(Audio::Bus::Music, 0.0f);
    Audio::SetBusDb(Audio::Bus::Sfx, 0.0f);

    s_audioInited = true;

    return true;
}

/**
 * @brief Updates the audio system per frame.
 *
 * This function should be called once every frame to allow FMOD
 * to process streaming, mixing, and internal DSP updates.
 * It does nothing if audio was not initialized.
 */
void ResourceManager::TickAudio() {
    if (s_audioInited) Audio::Update();
}

/**
 * @brief Updates the audio system per frame.
 *
 * This function should be called once every frame to allow FMOD
 * to process streaming, mixing, and internal DSP updates.
 * It does nothing if audio was not initialized.
 */
void ResourceManager::ShutdownAudio()
{
    if (!s_audioInited) return;
    Audio::StopAll(200);
    Audio::Shutdown();
    s_audioInited = false;
}


/**
 * @brief Load an audio file from disk and register it under a given name.
 *
 * @param name Logical name to associate with this audio file.
 * @param filepath Full path to the audio file.
 * @return true if the sound was successfully loaded, false otherwise.
 */
bool ResourceManager::LoadAudio(const std::string& name, const std::string& filepath) {
    Audio::SoundID sid = Audio::LoadSound(filepath);
    if (sid < 0) {
        DebugConsole::Get().Error("[ResourceManager] Failed to load audio: " + filepath + "\n");
        return false;
    }
    audios[name] = sid;
    DebugConsole::Get().AddFormattedMessage(LogLevel::Success, "[ResourceManager] Loaded audio ", name, " (", filepath, "), ID=", std::to_string(sid), "\n");
    return true;
}

/**
 * @brief Retrieve the Audio::SoundID associated with a previously loaded audio name.
 *
 * @param name Name of the audio asset.
 * @return The Audio::SoundID, or -1 if not found.
 */
Audio::SoundID ResourceManager::GetAudio(const std::string& name) {
    auto it = audios.find(name);
    if (it != audios.end()) return it->second;
    return -1;
}

void ResourceManager::PauseAudio() {
    if (!s_audioInited) return;
    Audio::PauseAll();
}

void ResourceManager::ResumeAudio() {
    if (!s_audioInited) return;
    Audio::ResumeAll();
}

bool ResourceManager::RegisterAudio(const std::string& name, const std::string& filepath) {
    return LoadAudio(name, filepath);
}

/**
    * @brief Loads an audio profile JSON file (volumes, SFX, BGM) and applies it.
    *
    * Supported sections:
    *  - "volumes": masterDb, musicDb, sfxDb
    *  - "sounds": name → filepath
    *  - "bgm": key, file, loop, gain
    *
    * Behavior:
    *  - Existing sounds with matching keys are safely unloaded before replacing.
    *  - BGM starts immediately only if @p startBgmIfAny is true.
    *
    * @param filename JSON path for audio profile.
    * @param applyVolumes If true, bus volumes are updated.
    * @param startBgmIfAny If true, BGM block triggers playback.
    * @return true on success, false on file/parse errors.
*/
bool ResourceManager::LoadAudioProfile(const std::string& filename,
    bool applyVolumes,
    bool /*startBgmIfAny*/) {

    std::ifstream f(filename);
    if (!f.is_open()) {
        DebugConsole::Get().Error("[AudioProfile] Could not open " + filename + "\n");
        return false;
    }

    nlohmann::json j;
    try { f >> j; }
    catch (const std::exception& e) {
        DebugConsole::Get().Error("[AudioProfile] JSON parse error in " + filename + ": " + e.what() + "\n");
        return false;
    }

    fs::path root = fs::path(filename).parent_path();  // .../Game/assets

    // 1) Optional volumes
    if (applyVolumes && j.contains("volumes") && j["volumes"].is_object()) {
        const auto& v = j["volumes"];
        if (v.contains("masterDb")) Audio::SetBusDb(Audio::Bus::Master, v["masterDb"].get<float>());
        if (v.contains("musicDb"))  Audio::SetBusDb(Audio::Bus::Music, v["musicDb"].get<float>());
        if (v.contains("sfxDb"))    Audio::SetBusDb(Audio::Bus::Sfx, v["sfxDb"].get<float>());
    }

    // 2) Optional sounds (register/override)
    if (j.contains("sounds") && j["sounds"].is_object()) {
        for (auto& [key, val] : j["sounds"].items()) {
            if (!val.is_string()) continue;
            const std::string rel = val.get<std::string>();
            fs::path fullPath = root / rel;

            // if previously loaded, unload old backend sound to avoid leaks
            if (auto old = ResourceManager::GetAudio(key); old >= 0) {
                Audio::UnloadSound(old);
            }
            ResourceManager::RegisterAudio(key, fullPath.string());
        }
    }

    DebugConsole::Get().Success("[AudioProfile] Applied: " + filename + "\n");
    return true;
}

/**
    * @brief Plays an audio asset with full playback control.
    *
    * This is the lowest-level audio playback wrapper exposed by the
    * ResourceManager. It allows complete control over playback parameters
    * such as bus routing, looping, gain, and panning through Audio::PlayDesc.
    *
    * Intended for advanced use cases such as looping ambience, charge sounds,
    * or any audio that requires lifecycle management via a returned handle.
    *
    * @param key  Logical audio key registered in the audio profile.
    * @param desc Playback description specifying bus, loop state, gain, and pan.
    * @return     A VoiceHandle for the playing sound, or 0 if playback failed.
 */
Audio::VoiceHandle ResourceManager::Play(const std::string& key, const Audio::PlayDesc& desc) {
    auto sid = GetAudio(key);
    if (sid < 0) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[Audio] Play failed for key '", key, "' (sound not loaded)\n");
        return 0;
    }
    Audio::VoiceHandle h = Audio::Play(sid, desc);
    if (!h) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[Audio] Play failed for key '", key, "' (no voices left)\n)");
        //std::cerr << "[Audio] Play failed for key '", key, "' (no voices left)\n)";
    }
    return h;
}

/**
    * @brief Plays an audio asset using direct playback parameters.
    *
    * This is a convenience overload of Play() that allows callers to specify
    * common playback settings directly without manually constructing an
    * Audio::PlayDesc structure. Internally, this function builds the descriptor
    * and forwards the call to the core Play() implementation.
    *
    * @param key   Logical audio key registered in the audio profile.
    * @param bus   Audio bus to route the sound through (e.g. Sfx or Music).
    * @param gain  Linear volume multiplier for this playback instance.
    * @param loop  Whether the sound should loop continuously.
    * @param pan   Stereo pan value (-1.0 = left, 0.0 = center, 1.0 = right).
    * @return      A VoiceHandle for the playing sound, or 0 if playback failed.
 */
Audio::VoiceHandle ResourceManager::Play(const std::string& key, Audio::Bus bus, float gain, bool loop, float pan) {
    Audio::PlayDesc d{};
    d.bus = bus;
    d.gain = gain;
    d.loop = loop;
    d.pan = pan;

    return Play(key, d);
}

/**
    * @brief Plays a one-shot sound effect.
    *
    * This helper is intended for the majority of gameplay and UI sound effects
    * such as footsteps, button clicks, hits, and interactions. The sound is
    * routed through the SFX bus and played once without returning a handle.
    *
    * Use this function when no further control over the sound is required
    * after it starts playing.
    *
    * @param key   Logical audio key registered in the audio profile.
    * @param gain  Linear volume multiplier for this playback instance.
 */
void ResourceManager::PlaySfx(const std::string& key, float gain) {
    (void)Play(key, Audio::Bus::Sfx, gain, /*loop=*/false, /*pan= */0.0f);
}

/**
 * @brief Retrieves the current world position of the player entity.
 * @param app Reference to the GameApp context.
 * @return glm::vec2 of player coordinates, or {0,0} if unavailable.
 */
glm::vec2 ResourceManager::GetPlayerPos(const GameApp& app) {
    // 1. Get the player entity ID directly from the GameApp
    Entity player = app.GetPlayerEntity();

    // 2. Prepare output variables for x and y as per your screenshot
    float x = 0.0f, y = 0.0f;

    // 3. Use the GameApp's helper to extract the coordinates
    if (player != INVALID_ENTITY && app.TryGetWorldPos(player, x, y)) {
        return glm::vec2(x, y); // Successfully found player pos
    }

    return glm::vec2(0.0f, 0.0f); // Default if player doesn't exist
}

/**
 * @brief Advanced spatialized SFX with dynamic volume and panning.
 * @param app       Reference to the GameApp to find the listener (player).
 * @param key       Asset key (e.g., "Enemy_Roar").
 * @param sourcePos The world position where the sound happens.
 * @param maxGain   The highest volume allowed (e.g., 0.5f).
 */
void ResourceManager::PlaySfx(const GameApp& app, const std::string& key, glm::vec2 sourcePos, float minRoll, float maxRoll, float maxGain) {
    if (!s_audioInited) return;

    glm::vec2 playerPos = GetPlayerPos(app);
    float distance = glm::distance(sourcePos, playerPos);

    // 1. Initial gain starts at desired 100% (maxGain)
    float gain = maxGain;

    // 2. Only decrease if we are outside the "Full Volume" circle
    if (distance > minRoll) {
        // Logarithmic falloff starting from the edge of minRoll
        float falloff = minRoll / distance;
        gain = maxGain * falloff;
    }

    // 3. Hard cutoff at the edge of hearing range
    if (distance > maxRoll) gain = 0.0f;

    // 4. Optimization: Exit if silent
    if (gain <= 0.001f) return;

    // 5. Panning (normalized -1.0 to 1.0)
    float pan = std::clamp((sourcePos.x - playerPos.x) / maxRoll, -1.0f, 1.0f);

    Audio::PlayDesc d;
    d.gain = gain;
    d.pan = pan;
    Play(key, d);
}

/**
    * @brief Plays a sound effect and returns a handle for lifecycle control.
    *
    * This function behaves similarly to PlaySfx(), but returns a VoiceHandle
    * to allow the caller to manage the sound after playback begins. It is
    * suitable for looping sound effects or effects that need to be stopped,
    * faded, or queried later (e.g. charging sounds or ambient loops).
    *
    * The sound is routed through the SFX bus by default.
    *
    * @param key   Logical audio key registered in the audio profile.
    * @param gain  Linear volume multiplier for this playback instance.
    * @param loop  Whether the sound should loop continuously.
    * @param pan   Stereo pan value (-1.0 = left, 0.0 = center, 1.0 = right).
    * @return      A VoiceHandle for the playing sound, or 0 if playback failed.
 */
Audio::VoiceHandle ResourceManager::PlaySfxHandle(const std::string& key, float gain, bool loop, float pan) {
    return Play(key, Audio::Bus::Sfx, gain, loop, pan);
}

/**
    * @brief Updates or starts a spatialized looping sound effect.
    * * This function calculates the appropriate gain and panning based on the distance
    * between the player (listener) and the sound source. It is intended to be called
    * every frame for persistent ambient sounds (e.g., torches) to ensure the audio
    * pans correctly as the player moves.
    * @param app       Reference to the GameApp to retrieve the current listener (player) position.
    * @param key       The unique string identifier for the audio asset in the JSON configuration.
    * @param sourcePos The world-space coordinates of the object emitting the sound.
    * @param minRoll   The "Inner Radius": distance within which the sound plays at 100% of maxGain.
    * @param maxRoll   The "Outer Radius": distance at which the sound becomes completely silent.
    * @param maxGain   The volume ceiling for the loop, typically a lower value for ambient effects (e.g., 0.3f).
 */
void ResourceManager::PlayLoopSfx(const GameApp& app, Entity e, const std::string& key,
    glm::vec2 sourcePos, float minRoll, float maxRoll, float maxGain) {

    // 1. Calculate spatial parameters
    glm::vec2 playerPos = GetPlayerPos(app);
    float distance = glm::distance(sourcePos, playerPos);
    float gain = (distance > minRoll) ? (minRoll / distance) * maxGain : maxGain;
    if (distance > maxRoll) gain = 0.0f;
    float pan = std::clamp((sourcePos.x - playerPos.x) / maxRoll, -1.0f, 1.0f);

    // 2. Check if this entity already has a voice handle
    auto it = s_activeLoops.find(e);

    if (it != s_activeLoops.end() && Audio::IsPlaying(it->second)) {
        // UPDATE EXISTING VOICE: Do not start a new one
        Audio::SetGain(it->second, gain);
        Audio::SetPan(it->second, pan);
    }
    else if (gain > 0.0f) {
        // START NEW VOICE: Only if one isn't already playing for this entity
        Audio::PlayDesc d;
        d.gain = gain;
        d.pan = pan;
        d.loop = true;
        s_activeLoops[e] = Play(key, d);
    }
}

/**
    * @brief Starts background music playback.
    *
    * This function plays a background music track through the Music bus.
    * Only one BGM track is active at any time; starting a new BGM will
    * automatically stop and replace the previously playing track.
    *
    * Intended to be called during scene transitions, level initialization,
    * or major gameplay state changes.
    *
    * @param key   Logical audio key registered in the audio profile.
    * @param loop  Whether the music should loop continuously.
    * @param gain  Linear volume multiplier for the music track.
 */
void ResourceManager::PlayBgm(const std::string& key, bool loop, float gain) {
    auto sid = GetAudio(key);
    if (sid < 0) {
        std::cerr << "[Audio] BGM key not found: " << key << "\n";
        return;
    }

    if (s_bgmHandle) {
        Audio::Stop(s_bgmHandle, 200);
        s_bgmHandle = 0;
    }

    Audio::PlayDesc d;
    d.bus = Audio::Bus::Music;
    d.loop = loop;
    d.gain = gain;

    s_bgmHandle = Audio::Play(sid, d);
    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[BGM] PlayBgm '", key, "' handle= ", s_bgmHandle, "\n");
}

void ResourceManager::PauseBgm(int fadeMs) {
    Audio::PauseBgm(fadeMs);
}

void ResourceManager::ResumeBgm(int fadeMs) {
    Audio::ResumeBgm(fadeMs);
}

/**
    * @brief Stops the currently playing background music.
    *
    * This function fades out and stops the active BGM track if one is playing.
    * It safely clears the internal BGM handle and can be used when entering
    * menus, pausing the game, or transitioning between scenes.
    *
    * @param fadeMs Duration of the fade-out in milliseconds.
 */
void ResourceManager::StopBgm(int fadeMS) {
    if (!s_bgmHandle)
        return;
    Audio::Stop(s_bgmHandle, fadeMS);
    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[BGM] StopBgm handle= ", s_bgmHandle, "\n");
    s_bgmHandle = 0;
}

/**
 * @brief Transitions the audio to a "muffled" state over a period of time.
 *
 * Commonly used when going underground.
 *
 * @param muffled Whether the audio should be muffled.
 * @param fadeMs  How long the transition should take in milliseconds.
 */
void ResourceManager::SetLPFilter(bool muffled, int fadeMs) {
    if (!s_audioInited)
        return;

    // 22000Hz is effectively "off" (human hearing limit)
    // 800Hz - 1000Hz is a good "muffled" value
    float target = muffled ? 800.0f : 22000.0f;
    Audio::SetLowPass(target, fadeMs);
}

// -----------------------------------------------------------------------------------



// -----------------------------------------------------------------------------------
// ------------------------------------- Font ---------------------------------------- 

// add helper function for font management over here

// -----------------------------------------------------------------------------------