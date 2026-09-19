#pragma once
/**
 * @file      resourceManager.h
 * @author    Jethro Sung
 * @email     sung.h, w.kyele, jianlin.low
 * @co-author Woh Kye Le , Low JianLin
 * @date      2025-09-29
 *
 * @brief    Declaration of the ResourceManager class, which centralizes
 *           the loading and retrieval of assets such as textures, sprite sheets,
 *           audio files, and configuration data for the game.
 *
 * The ResourceManager act as a centralised facade for :
 *  1) configuration
 *  2) graphics assets (textures/sprite sheets)
 *  3) audio (load/init/update/shutdown)
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include <string>
#include <glm/glm.hpp>
#include "Core/config.h"
#include "Core/loader.h"
#include "Audio/audio.h" 
#include "Graphics/texture2d.h"
#include "Graphics/spritesheet.h"
#include "Core/componentcontext.h"
 //#include <json.hpp>
 //
 //using json = nlohmann::json;

  /**
    * @struct AudioBootstrap
    * @brief optional configuration for initialising the audio system
    */
struct AudioBootstrap {
    int   deviceSampleRate = 48000;
    int   deviceChannels = 2;
    int   maxVoices = 256;
    std::string bgmName;        // optional logical name for auto-play bgm
    float       bgmGain = 0.8f;
    bool        bgmLoop = true;
};



/**
 * @class ResourceManager
 * @brief A static asset manager responsible for initialization, loading,
 *        retrieval, and cleanup of game assets such as textures, audio,
 *        sprite sheets, and configuration files.
 *
 * This class is not meant to be instantiated. All functions and data are static,
 * providing a centralized way to access shared resources across the engine.
 */
class ResourceManager {
public:

    // -------------------------------------- Initialization / Shutdown ----------------------------------------
    // ---------------------
    /**
     * @brief Initialize the resource manager with the specified base directory.
     *
     * Loads the game configuration (`config.json`) and initializes the Assets
     * system with the provided base directory. This should be called once
     * at engine startup.
     *
     * @param baseDir Path to the base asset directory (e.g., "Assets").
     * @return true if initialization succeeded, false otherwise.
     */
    static bool Init(const AudioBootstrap* audioBoot = nullptr);

    /**
     * @brief Per-frame maintenance for managed subsystems.
     *
     * Currently forwards to TickAudio() when the audio backend is active, add more updates according to game needs
     * Call once per frame from your main loop.
     */
    static void Update();

    /**
     * @brief Clean up and free all loaded assets.
     *
     * Clears textures, sprite sheets, audio maps, and shuts down the Assets system.
     * Should be called once at engine shutdown.
     */
    static void Shutdown();
    // ---------------------
    // ---------------------------------------------------------------------------------------------------------



    // ------------------------------ Getter function (Config, Audio & graphics) -------------------------------
    // ---------------------
    /**
     * @brief Get the loaded game configuration object.
     *
     * @return A const reference to the global Config struct.
     */
    static const Config& GetConfig();

    /**
     * @brief Retrieve a previously loaded audio asset by name.
     *
     * @param name Name of the audio asset.
     * @return The Audio::SoundID associated with the name, or -1 if not found.
     */
    static Audio::SoundID GetAudio(const std::string& name);

    /**
    * @brief Retrieve a texture by its registered name.
    *
    * The texture must have been previously loaded by the Assets system.
    *
    * @param name Name of the texture.
    * @return OpenGL texture ID (GLuint) associated with the name, or 0 if not found.
    */
    static GLuint GetTexture(const std::string& name);

    static const std::unordered_map<std::string, std::unique_ptr<Texture2D>>& GetAllTextures();

    /**
    * @brief Retrieve a sprite sheet by its name.
    *
    * The sprite sheet must have been previously loaded by the Assets system.
    *
    * @param name Name of the sprite sheet.
    * @return Pointer to the SpriteSheet object, or nullptr if not found.
    */
    static const SpriteSheet* GetSpriteSheet(const std::string& name);

    static bool ImportTexture(const std::string& absPath, const std::string& name);

    static std::string GetTextureNameByID(GLuint id);
    // ---------------------
    // ---------------------------------------------------------------------------------------------------------


    // ------------------------------------------ Public audio functions ---------------------------------------
    // ---------------------
    static void PauseAudio();

    static void ResumeAudio();

    /**
    * @brief Load and register an audio asset by name.
    *
    * Provides safe public access to load a new sound at runtime without exposing
    * internal implementation details. The asset will be registered under the given
    * logical key and stored in the ResourceManager’s internal audio map.
    *
    * @param name     Logical key to register the sound (e.g., "door_open").
    * @param filepath Full path to the audio file (e.g., "Assets/audio/door.wav").
    * @return true if the sound was successfully loaded and registered; false otherwise.
    */
    static bool RegisterAudio(const std::string& name, const std::string& filepath);

    static bool LoadAudioProfile(const std::string& filename, bool applyVolumes = true, bool startBgmIfAny = true);

    static Audio::VoiceHandle Play(const std::string& key, const Audio::PlayDesc& desc);
    static Audio::VoiceHandle Play(const std::string& key, Audio::Bus bus, float gain = 1.0f, bool loop = false, float pan = 0.0f);

    static void PlaySfx(const std::string& key, float gain = 1.0f);
    static void PlaySfx(const GameApp& app, const std::string& key, glm::vec2 sourcePos, float minRoll, float maxRoll, float maxGain);
    static Audio::VoiceHandle PlaySfxHandle(const std::string& key, float gain = 1.0f, bool loop = false, float pan = 0.0f);
    static void PlayLoopSfx(const GameApp& app, Entity e, const std::string& key, glm::vec2 sourcePos, float minRoll, float maxRoll, float maxGain);

    static void PlayBgm(const std::string& key, bool loop = true, float gain = 0.0f);
    static void PauseBgm(int fadeMs = 200);
    static void ResumeBgm(int fadeMs = 200);
    static void StopBgm(int fadeMs = 200);

    static void SetLPFilter(bool muffled, int fadeMs = 200);

    // ---------------------
    // ---------------------------------------------------------------------------------------------------------



    // ---------------------------------------- Public graphics functions --------------------------------------
    // ---------------------
    static bool RegisterSpriteSheet(const std::string& name, SpriteSheet&& sh);

    /**
    * @brief Load a single texture from a path.
    *
    * Helper utility that creates a Texture2D object, loads image data from the specified
    * file using stb_image, uploads it to OpenGL, and returns a unique_ptr managing
    * the resulting texture object.
    *
    * @param abs file path to the texture (e.g., "Assets/textures/crate.png").
    * @return std::unique_ptr<Texture2D> Owning pointer to the successfully loaded texture;
    *         returns nullptr if loading or GPU upload fails.
    */
    static std::unique_ptr<Texture2D> loadTexAbs(const std::string& abs);
    // ---------------------
    // ---------------------------------------------------------------------------------------------------------
private:
    static Config cfg;  // @brief Global configuration object.
    static std::unordered_map<std::string, std::unique_ptr<Texture2D>> textures; // @brief Storage for loaded textures mapped by name.
    static std::unordered_map<std::string, SpriteSheet> sheets;// @brief Storage for loaded sprite sheets mapped by name.
    static std::unordered_map<std::string, Audio::SoundID> audios;// @brief Storage for loaded audio assets mapped by name.
    static std::unordered_map<Entity, Audio::VoiceHandle> s_activeLoops;// Tracks active loops: Entity ID -> FMOD Voice Handle
    static glm::vec2 GetPlayerPos(const GameApp& app);
    static bool s_audioInited;
    static Audio::VoiceHandle s_bgmHandle;

    // --------------------------------------------- Graphics Assets -------------------------------------------
    // ---------------------
    /**
     * @brief Initialize and preload built-in graphic assets (textures and sprite sheets).
     *
     * This function loads all essential 2D textures and sprite sheets used by the game
     * into GPU memory. Each asset is registered under a logical name and stored in
     * the internal ResourceManager maps for later retrieval via GetTexture() or
     * GetSpriteSheet().
     *
     * @param baseDir Root path of the asset directory (default: "Assets").
     *
     * @return true if initialization completes (individual load failures are logged
     *         but do not cause this function to return false).
     *
     * @note This is typically called once during engine startup by ResourceManager::Init().
     */
    static bool InitAssets(const std::string& baseDir = "Assets");



    /**
    * @brief Release all loaded texture and sprite-sheet resources.
    *
    * Clears all internal maps storing Texture2D and SpriteSheet objects.
    * GPU texture objects are automatically freed by the Texture2D destructor.
    *
    * @return void
    *
    * @note Call this during engine shutdown to cleanly release all GPU and CPU
    *       resources associated with loaded textures and sprite sheets.
    */
    static void ShutDownAssets();
    // ---------------------
    // ---------------------------------------------------------------------------------------------------------



    // -------------------------------------------------- Audio ------------------------------------------------
    // ---------------------
    /**
     * @brief Initialize the audio backend using the provided bootstrap settings.
     *
     * Sets up FMOD with the desired device format and max voices, applies safe
     * default bus levels, and optionally auto-plays a BGM track (if provided
     * in @p boot and already loaded via LoadAudio/PreloadAudioFromConfig()).
     *
     * @param boot Audio bootstrap parameters (device format, voices, optional BGM).
     * @return true if audio was initialized, false on failure.
     */
    static bool InitAudio(const AudioBootstrap& boot);

    /**
      * @brief Advance the audio engine by one frame.
      *
      * Processes streaming, decoding, DSP, and mixing within the audio backend.
      * This is a no-op if audio has not been initialized.
      */
    static void TickAudio();

    /**
     * @brief Cleanly shut down the audio backend.
     *
     * Stops all voices (with a short fade), releases FMOD resources, and marks
     * the audio system as uninitialized.
     */
    static void ShutdownAudio();

    /**
     * @brief Load an audio file and register it under the specified name.
     *
     * @param name Name to register the audio asset under.
     * @param filepath Full path to the audio file.
     * @return true if the audio file was loaded successfully, false otherwise.
     */
    static bool LoadAudio(const std::string& name, const std::string& filepath);


    // ---------------------
    // ---------------------------------------------------------------------------------------------------------




};
