#pragma once
/**
 * @file      sceneManager.h
 * @author    Sng Swee Yong Dillon,
 * @co-author Woh Kye le, Jethro Sung
 * @email     sweeyongdillon.sng, w.kyele, sung.h
 * @date      2025-11-29
 *
 * @brief declares the SceneManager class, which functions as the central controller for deciding what part of the game is currently active and visible.
 * It defines the SceneState enum (MainMenu, Playing, Paused, Settings, Exiting) and provides helper functions such as IsPlaying() and IsPaused() for evaluating the current state.
 * The class stores pointers to the GameApp, MainMenu, PauseMenu, and their corresponding GuiSystem instances, and exposes methods to attach them at initialization.
 *
 *  * Key responsibilities:
 * - Manage scene state transitions (MainMenu ↔ Playing ↔ Paused ↔ Settings ↔ Exiting)
 * - Coordinate UI rendering based on current state
 * - Handle gameplay scene loading with proper door spawn logic
 * - Maintain separation between game simulation and UI navigation
 * - Support editor integration with pause/play state management
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "Input/input.h"
#include "Editor/PlayStopManager.h"
#include "Editor/editor.h"
#include "UI/cutsceneSequence.h"

#include <string>
#include <vector>
#include <utility> 

class GameApp;
class RoomEditor;
class UiEditor;
class MainMenu;
class PauseMenu;
class SettingsMenu;
class ConfirmDialog;
class GuiSystem;
class Renderer;
class LoseMenu;
class TutorialMenu;
class ControlMenu;
class CreditsMenu;

/**
 * @enum SceneState
 * @brief Represents the current high-level state of the game application.
 *
 * This enum defines all possible states that the SceneManager can be in,
 * controlling what is rendered and updated each frame.
 */
enum class SceneState {
    MainMenu,
    CutsceneIntro,
    CutsceneEnd,
    Playing,
    Paused,
    Lose,
    Settings,
    Controls,
    Credits,
    Tutorial,
    ConfirmQuit,
    Exiting
};

/**
 * @enum GameplaySceneId
 * @brief Identifies specific gameplay scenes by ID.
 *
 * Used to map scene identifiers to their corresponding file paths.
 */
enum class GameplaySceneId {
    Labyrinth,
    Level1,
    Level2,
    Level3,
    Level4,
    Level5
};

/**
 * @brief Converts a GameplaySceneId to its corresponding file path.
 *
 * @param id The scene identifier to convert.
 * @return const char* Path to the scene JSON file.
 */
static inline const char* ToScenePath(GameplaySceneId id) {
    switch (id) {
    case GameplaySceneId::Labyrinth: return "scene/labyrinth.json";
    case GameplaySceneId::Level1:    return "scene/entities_Level1.json";
    case GameplaySceneId::Level2:    return "scene/entities_Level2.json";
    case GameplaySceneId::Level3:    return "scene/entities_Level3.json";
    case GameplaySceneId::Level4:    return "scene/entities_Level4.json";
    case GameplaySceneId::Level5:    return "scene/entities_Level5.json";
    default:                         return "scene/labyrinth.json";
    }
}

/**
* @enum WorldActionType
* @brief Actions SceneManager can request GameApp to perform (world-side operations).
*/
enum class WorldActionType {
    None,
    RecreateEditorOverlay,
    ResetWorld,
    RestorePlayerSnapshot,
    ResetHUD
};


/**
 * @struct SceneChangeRequest
 * @brief Holds information for a pending gameplay scene change request.
 *
 * This structure contains all necessary data to load a new scene,
 * including optional door spawning information for player positioning.
 */
struct SceneChangeRequest {
    std::string sceneFile;
    std::string spawnDoorName;
    bool hasSpawn = false;
    bool skipPuzzleSave = false;
};


/**
 * @class SceneManager
 * @brief Central state machine controlling game flow and scene transitions.
 *
 * The SceneManager orchestrates the high-level flow of the game application,
 * managing transitions between different states (menus, gameplay, etc.) and
 * coordinating what should be rendered and updated each frame. It maintains
 * references to all UI systems and the GameApp instance.
 */
class SceneManager {
public:
    SceneManager() = default;

    /**
    * @brief Changes the active SceneState and updates menu visibility.
    *
    * This function is the central state-transition handler. It hides all menus,
    * then selectively enables the one associated with the new SceneState.
    * If the target state is Exiting, a confirmation dialog is shown.
    *
    * @param s The new scene state to activate.
    */
    void SetState(SceneState s);

    /**
    * @brief Gets the current scene state.
    * @return SceneState The current active state.
    */
    SceneState GetState() const { return current; }


    bool IsMainMenu()        const { return current == SceneState::MainMenu; }
    bool IsCutsceneIntro()   const { return current == SceneState::CutsceneIntro; }
    bool IsCutsceneEnd()     const { return current == SceneState::CutsceneEnd; }
    bool IsPlaying()         const { return current == SceneState::Playing; }
    bool IsPaused()          const { return current == SceneState::Paused; }
    bool IsSettings()        const { return current == SceneState::Settings; }
    bool IsControls()        const { return current == SceneState::Controls; }
    bool IsCredits()         const { return current == SceneState::Credits; }
    bool IsTutorial()        const { return current == SceneState::Tutorial; }
    bool IsExiting()         const { return current == SceneState::Exiting; }
    bool IsLose() const { return current == SceneState::Lose; }



    /**
    * @brief Checks if gameplay simulation should be paused.
    * Returns true when the game is in a state where gameplay logic
    * should not be updated (Paused or ConfirmQuit states).
    *
    * @return true if gameplay should be paused, false otherwise.
    */
    bool IsGameplaySimPaused() const { return current == SceneState::Paused || current == SceneState::Lose || current == SceneState::ConfirmQuit || current == SceneState::Tutorial || current == SceneState::CutsceneIntro || current == SceneState::CutsceneEnd; }

    /**
    * @brief Attaches the SceneManager to the GameApp and binds all GUI/menu systems.
    *
    * This function must be called once during initialization. It supplies the
    * SceneManager with references to all required components.
    *
    * @param app          Pointer to the running GameApp instance.
    * @param mainMenu     Pointer to the main menu UI controller.
    * @param mainMenuGui  Pointer to the GUI system used by the main menu.
    * @param pauseMenu    Pointer to the pause menu UI controller.
    * @param pauseGui     Pointer to the GUI system used by the pause menu.
    * @param settings     Pointer to the settings menu UI controller.
    * @param settingsGui  Pointer to the GUI system used by the settings menu.
    * @param confirmQuit  Pointer to the confirmation dialog UI controller.
    * @param confirmGui   Pointer to the GUI system used by the confirmation dialog.
    */
    void Attach(GameApp* app, MainMenu* mainMenu, GuiSystem* mainMenuGui, PauseMenu* pauseMenu, GuiSystem* pauseGui, LoseMenu* loseMenu, GuiSystem* loseGui, SettingsMenu* settings, GuiSystem* settingsGui, ControlMenu* controlMenu, GuiSystem* controlGui, CreditsMenu* creditsMenu, GuiSystem* creditsGui, TutorialMenu* tutorialMenu, GuiSystem* tutorialGui, ConfirmDialog* confirmQuit, GuiSystem* confirmGui);
    /**
    * @brief Initializes all UI menus managed by the SceneManager.
    *
    * This function must be called once after all menu objects and GUI systems
    * have been attached. It builds and loads the UI layouts for all menus,
    * wiring their button callbacks to appropriate state transitions.
    *
    * @param app Pointer to the owning GameApp instance.
    */
    void Init(GameApp* app);

    /**
    * @brief Routes per-frame input updates to the correct active UI system.
    *
    * This function ensures only the GUI belonging to the current SceneState
    * receives mouse/keyboard events. Also handles global ESC key for pause/resume.
    *
    * @param in The current frame's input state (mouse/keyboard).
    */
    void Update(const eng::Input& in, double dt);

    /**
    * @brief Renders the scene based on the current SceneState.
    *
    * Behavior per state:
    * - MainMenu / Settings: Draws the corresponding menu UI.
    * - Playing: Draws gameplay only (no menus).
    * - Paused: Draws gameplay, then overlays the pause menu.
    * - ConfirmQuit: Draws background state with modal dialog on top.
    * - Exiting: Does nothing; GameApp will handle shutdown.
    *
    * @param renderer Reference to the active Renderer.
    */
    void Draw(Renderer& renderer);

    // ========================= GAMEPLAY SCENE LOADING =========================
    void RequestGameplayScene(const std::string& sceneFile, const std::string& spawnDoorName = "", bool skipPuzzleSave = false);
    void RequestGameplaySceneWithAutoSpawn(const std::string& nextFile, const std::string& previousFile, bool skipPuzzleSave = false);
    void CommitPendingSceneLoad();
    void RestartGameplay();

    // ================================= PATHING ================================
    void SetEditScenePath(std::string p) { editScenePath = std::move(p); }
    const std::string& GetEditScenePath() const { return editScenePath; }
    const std::string& GetPlayScenePath() const { return playScenePath; }
    const std::string& GetRuntimeScenePath() const { return runtimeScenePath; }
    const std::string& GetActiveScenePath() const;
    const std::string& GetCurrentGameplayScene() const { return currentGameplayScene; }

    // ========================== EDITOR SIM CONTROL ============================
    bool IsEditorPlaying() const { return editorIsPlaying; }
    bool IsEditorPaused()  const { return editorIsPaused; }
    void SetEditorPaused(bool paused) { editorIsPaused = paused; }

#if ENABLE_EDITOR
    enum class EditorCmdType { ChangeLevel, Play, TogglePause, Stop };

    struct EditorCmd {
        EditorCmdType type{};
        std::string   levelPath{};
    };

    void EnqueueEditorCmd(EditorCmd cmd) { editorCmdQueue.emplace_back(std::move(cmd));}
    void HandleDeferredEditorActions();
    EditorPlayControlsState GetPlayControlsState() const;

    EditorPlayControlsCallbacks GetEditorPlayCallbacks() {
        EditorPlayControlsCallbacks cb;
        cb.onPlay = [this]() { EnqueueEditorCmd({ EditorCmdType::Play, {} }); };
        cb.onPause = [this]() { EnqueueEditorCmd({ EditorCmdType::TogglePause, {} }); };
        cb.onStop = [this]() { EnqueueEditorCmd({ EditorCmdType::Stop, {} }); };
        return cb;
    }

    void CommitEditorChangesNow();
    // Toggle editor overlay and room editor visibility based on current scene.
    void ToggleEditorsForActiveScene();
    // Drive editor ImGui frame and route scene texture to editor viewports.
    void DrawEditorFrame(class Renderer& renderer, float dt, bool usingEditor);
#endif

private:

    // ========================= UI STATE =========================
    SceneState current = SceneState::MainMenu;      // start on main menu
    SceneState beforeConfirm = SceneState::MainMenu;
    SceneState pauseBackground = SceneState::Playing;

    // ====================== GAMEPLAY PATHS ======================
    std::string bootGameplayScene = "scene/labyrinth.json";
    std::string currentGameplayScene = "";          // track what scene gameplay is currently using

    std::string editScenePath = bootGameplayScene;
    std::string playScenePath;
    std::string runtimeScenePath;

    // ===================== EDITOR SIM FLAGS =====================
    bool editorIsPlaying = false;
    bool editorIsPaused = false;

    GameApp* app = nullptr;

    MainMenu* mainMenu = nullptr;
    PauseMenu* pauseMenu = nullptr;
    SettingsMenu* settingsMenu = nullptr;
    ControlMenu* controlMenu = nullptr;
    CreditsMenu* creditsMenu = nullptr;
    TutorialMenu* tutorialMenu = nullptr;
    ConfirmDialog* confirmQuit = nullptr;
    LoseMenu* loseMenu = nullptr;

    GuiSystem* mainMenuGui = nullptr;
    GuiSystem* pauseGui = nullptr;
    GuiSystem* settingsGui = nullptr;
    GuiSystem* controlGui = nullptr;
    GuiSystem* creditsGui = nullptr;
    GuiSystem* tutorialGui = nullptr;
    GuiSystem* confirmGui = nullptr;
    GuiSystem* loseGui = nullptr;

    bool hasPending = false;
    SceneChangeRequest pending;

    CutsceneSequence introCutscenes;
    CutsceneSequence endCutscenes;
    FadeOverlay gameplayFade;

#if ENABLE_EDITOR
    std::vector<EditorCmd> editorCmdQueue;
#endif

    void DispatchWorldAction(WorldActionType act);
};
