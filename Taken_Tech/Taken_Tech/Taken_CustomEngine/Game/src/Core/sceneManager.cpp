/**
 * @file      sceneManager.cpp
 * @author    Sng Swee Yong Dillon
 * @co-author Woh Kye Le, Low Jianlin, Jethro Sung
 * @email     sweeyongdillon.sng, w.kyele, jianlin.low, sung.h
 * @date      2025-11-29
 *
 * @brief
 * Implements the central state controller that governs which part of the game is currently being displayed.
 * It coordinates the transition between main menu, settings, gameplay, paused state, and exiting, and ensures that
 * each state renders the correct combination of UI and gameplay elements. The SceneManager does not perform gameplay logic itself;
 * instead, it delegates rendering to either the GameApp (for world drawing) or to the menu systems (MainMenu and PauseMenu), allowing the game to maintain a clean
 * separation between core game simulation and higher-level navigation flow.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "Core/sceneManager.h"
#include "Core/gameApp.h"
#include "UI/mainMenu.h"
#include "UI/pauseMenu.h"
#include "UI/settingsMenu.h"
#include "UI/controlMenu.h"
#include "UI/creditsMenu.h"
#include "UI/guiSys.h"
#include "UI/loseMenu.h"
#include "UI/tutorialMenu.h"
#include "UI/confirmDialog.h"
#include "UI/proximityPromptText.h"
#include "Graphics/renderer.h"
#include "Math/matrix3x3.h"
#include "Math/vect3.h"


#include <cstring>   // std::strlen
#include <cctype>    // std::isdigit
#include <utility>   // std::move

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if ENABLE_EDITOR
#include "Editor/editor.h"
#include "Editor/RoomEditor.h"
#include "Editor/UiEditor.h"
#include "Editor/ImGuiHost.h"
#include "Core/utils.h"
#endif

 // ---- Static functions ----
//static Audio::VoiceHandle s_fireloop = 0;
static std::string s_currentGameplayBGMKey;

// ---- Helper functions ----

/**
 * @brief Extracts room index number from a scene file path.
 *
 * Parses scene paths like "scene/entities_Level1.json" to extract the numeric
 * room identifier. Used for determining appropriate door names when transitioning
 * between rooms and the labyrinth.
 *
 * @param[in] path The scene file path to parse.
 * @return int The extracted room index, or 1 if parsing fails.
 *
 * @note Returns 1 as default if no valid number is found after "entities_Level".
 */
static int ExtractRoomIndexFromPath(const std::string& path) {
    constexpr const char* needle = "entities_Level";
    size_t pos = path.rfind(needle);
    if (pos == std::string::npos) return 1;

    pos += std::strlen(needle);

    size_t q = pos;
    while (q < path.size() && std::isdigit(static_cast<unsigned char>(path[q])))
        ++q;

    if (q <= pos) return 1;

    try { return std::stoi(path.substr(pos, q - pos)); }
    catch (...) { return 1; }
}

/**
 * @brief Generates a door name for transitioning from a room to the labyrinth.
 *
 * Creates a door name in the format "DoorToRoomX" where X is the room index
 * extracted from the previous scene file path.
 *
 * @param[in] previousFile The path of the room scene being exited.
 * @return std::string Door name like "DoorToRoom1", "DoorToRoom2", etc.
 */
static std::string DoorToRoomFromPrevious(const std::string& previousFile) {
    int roomIndex = ExtractRoomIndexFromPath(previousFile);
    return "DoorToRoom" + std::to_string(roomIndex);
}

static const char* GetBgmKeyForScene(const std::string& sceneFile)
{
    //if (sceneFile.find("labyrinth.json") != std::string::npos) return "testfile";
    //if (sceneFile.find("entities_Level1") != std::string::npos) return "bgm_room1";
    //if (sceneFile.find("entities_Level2") != std::string::npos) return "bgm_room2";
    if (sceneFile.find("labyrinth.json") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("entities_Level1") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("entities_Level2") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("entities_Level3") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("entities_Level4") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("entities_Level5") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("tut0.json") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("tut1.json") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("tut2.json") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("tut3.json") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("tut4.json") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("tut5.json") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("tut6.json") != std::string::npos) return "bgm_labyrinth";
    if (sceneFile.find("tut7.json") != std::string::npos) return "bgm_labyrinth";
    //if (sceneFile.find("loseMenu.json") != std::string::npos) return "bgm_losescene";
    return nullptr; // no bgm for unknown scenes
}


// ---- end of helper function ----

/**
    * @brief Changes the active SceneState and updates menu visibility.
    *
    * This function is the central state-transition handler. It hides all menus,
    * then selectively enables the one associated with the new SceneState.
    * If the target state is Exiting, the SceneManager forwards a quit request
    * to the owning GameApp.
    *
    * @param s The new scene state to activate.
    *
*/
void SceneManager::SetState(SceneState s) {

    if (s == SceneState::Exiting) {
        // If confirm system not wired, fallback to old behavior
        if (!confirmQuit || !confirmGui) {
            if (app) app->RequestQuit();
            return;
        }

        // Save state to restore if user cancels
        beforeConfirm = current;
        current = SceneState::ConfirmQuit;

        // Show modal
        confirmQuit->Show(
            "Quit game?",
            [this]() { if (app) app->RequestQuit(); },      // YES
            [this]() { current = beforeConfirm; }           // NO
        );

        return;
    }

    SceneState old = current;
    // If leaving Settings, persist settings UI layout
    if (old == SceneState::MainMenu && s != SceneState::MainMenu) {
        if (mainMenu) mainMenu->SaveLayout("scene/mainMenu.json", *mainMenuGui);
    }
    if (old == SceneState::Settings && s != SceneState::Settings) {
        if (settingsMenu) settingsMenu->SaveLayout("scene/settingsMenu.json", *settingsGui);
    }
    if (old == SceneState::Paused && s != SceneState::Paused) {
        if (pauseMenu) pauseMenu->SaveLayout("scene/pauseMenu.json", *pauseGui);
    }
    if (old == SceneState::Credits && s != SceneState::Credits) {
        if (creditsMenu) creditsMenu->SaveLayout("scene/creditsMenu.json", *creditsGui);
    }
    if (old == SceneState::ConfirmQuit && s != SceneState::ConfirmQuit) {
        if (confirmQuit) confirmQuit->SaveLayout("scene/confirmDialog.json", *confirmGui);
    }
    if (old == SceneState::Lose && s != SceneState::Lose) {
        if (loseMenu) loseMenu->SaveLayout("scene/loseMenu.json", *loseGui);
    }
    if (s == SceneState::Paused && old != SceneState::Paused) {
        pauseBackground = old;
    }

    current = s;

    // --- AUDIO: Pause only BGM in pause menu, resume when leaving pause ---

    const bool enteringPauseLike = (s == SceneState::Paused);
    const bool leavingPauseLike = (old == SceneState::Paused) && !enteringPauseLike;

    if (!(old == SceneState::Paused || old == SceneState::Lose) && enteringPauseLike)
    {
        ResourceManager::PauseBgm(300);
    }

    if (leavingPauseLike)
    {
        ResourceManager::ResumeBgm(400);
    }

    if (s == SceneState::Playing && (old == SceneState::MainMenu || old == SceneState::Settings || old == SceneState::Tutorial || old == SceneState::CutsceneIntro)) {
        if (app && !hasPending) {
            // Ensure fresh start when entering gameplay from menus or tutorial
            RestartGameplay();
        }
    }
    if (s == SceneState::Playing && old == SceneState::CutsceneIntro) {
        float fadeIn = introCutscenes.GetFadeInSecondsAfter();
        if (fadeIn <= 0.0f) fadeIn = 3.0f;
        gameplayFade.StartFadeIn(fadeIn);
    }

#if ENABLE_EDITOR
    if (app) {
        app->SyncEditorToActiveScene();
    }
#endif


    // Hide everything initially
    if (mainMenu)     mainMenu->Hide();
    if (pauseMenu)    pauseMenu->Hide();
    if (settingsMenu) settingsMenu->Hide();
    if (controlMenu) controlMenu->Hide();
    if (creditsMenu) creditsMenu->Hide();
    if (tutorialMenu) tutorialMenu->Hide();
    if (confirmQuit) confirmQuit->Hide();
    if (loseMenu)     loseMenu->Hide();
    introCutscenes.Stop();
    endCutscenes.Stop();

    // Detect transition: Gameplay/Pause -> MainMenu
    const bool leavingGameplay = (old == SceneState::Playing || old == SceneState::Paused || old == SceneState::Lose) && (s == SceneState::MainMenu);

    if (leavingGameplay) {
        if (app)
            RestartGameplay();   // Restart game 
    }

    // Show only the active menu
    switch (s) {
    case SceneState::MainMenu:
        s_currentGameplayBGMKey.clear();
        ResourceManager::StopBgm(300);

        ResourceManager::PlayBgm("bgm", true, 1.0f);

        if (mainMenu) mainMenu->Show();

        break;

    case SceneState::CutsceneIntro:
        s_currentGameplayBGMKey.clear();
        ResourceManager::StopBgm(200);
        introCutscenes.Start();
        ResourceManager::PlayBgm("bgm_startscene", true, 0.27f);
        break;

    case SceneState::CutsceneEnd:
        s_currentGameplayBGMKey.clear();
        ResourceManager::StopBgm(300);
        endCutscenes.Start();
        ResourceManager::PlayBgm("bgm_endcutscene", true, 0.8f);
        break;

    case SceneState::Settings:
        if (settingsMenu) settingsMenu->Show();
        ResourceManager::ResumeBgm();
        break;

    case SceneState::Controls:
        if (controlMenu) controlMenu->Show();
        ResourceManager::ResumeBgm();
        break;

    case SceneState::Credits:
        if (creditsMenu) creditsMenu->Show();
        ResourceManager::StopBgm(300);
        break;

    case SceneState::Tutorial:
        if (tutorialMenu && tutorialGui) {
            tutorialMenu->SetTutorialStep(TutorialMenu::TutorialStep::Tut0, *tutorialGui);
            tutorialMenu->Show();
        }
        ResourceManager::StopBgm(300);
        ResourceManager::PlayBgm("bgm_labyrinth", true, 0.40f);

        s_currentGameplayBGMKey = "bgm_labyrinth";
        break;

    case SceneState::Paused:
        if (pauseMenu) pauseMenu->Show();
       
        break;

    case SceneState::Lose:
        s_currentGameplayBGMKey.clear();
        if (loseMenu) loseMenu->Show();
        ResourceManager::StopBgm(300);
        ResourceManager::PlayBgm("bgm_losescene", false, 1.1f);
        break;

    case SceneState::ConfirmQuit:
        // should be entered only via Exiting interception
        break;


    default:
        break;
    }
}

/**
    * @brief Attaches the SceneManager to the GameApp and binds all GUI/menu systems.
    *
    * This function must be called once during initialization. It supplies the
    * SceneManager with:
    *  - The owning GameApp
    *  - Main menu UI
    *  - Main menu GUI system
    *  - Pause menu UI
    *  - Pause menu GUI system
    *
    * @param app_         Pointer to the running GameApp instance.
    * @param mainMenu_    Pointer to the main menu UI controller.
    * @param mainMenuGui_ Pointer to the GUI system used by the main menu.
    * @param pauseMenu_   Pointer to the pause menu UI controller.
    * @param pauseGui_    Pointer to the GUI system used by the pause menu.
*/
void SceneManager::Attach(GameApp* app_,
    MainMenu* mainMenu_, GuiSystem* mainMenuGui_,
    PauseMenu* pauseMenu_, GuiSystem* pauseGui_, LoseMenu* loseMenu_, GuiSystem* loseGui_,
    SettingsMenu* settingsMenu_, GuiSystem* settingsGui_, ControlMenu* controlMenu_, GuiSystem* controlGui_,
    CreditsMenu* creditsMenu_, GuiSystem* creditsGui_, TutorialMenu* tutorialMenu_, GuiSystem* tutorialGui_,
    ConfirmDialog* confirmQuit_, GuiSystem* confirmGui_) {
    app = app_;
    mainMenu = mainMenu_;
    mainMenuGui = mainMenuGui_;
    pauseMenu = pauseMenu_;
    pauseGui = pauseGui_;
    loseMenu = loseMenu_;
    loseGui = loseGui_;
    settingsMenu = settingsMenu_;
    settingsGui = settingsGui_;
    controlMenu = controlMenu_;
    controlGui = controlGui_;
    creditsMenu = creditsMenu_;
    creditsGui = creditsGui_;
    tutorialMenu = tutorialMenu_;
    tutorialGui = tutorialGui_;
    confirmQuit = confirmQuit_;
    confirmGui = confirmGui_;
}

/**
     * @brief Initializes all UI menus managed by the SceneManager.
     *
     * This function must be called once after all menu objects and GUI systems
     * have been attached through SceneManager::Attach(). It performs the
     * following tasks:
     *
     *  - Registers and builds the **Main Menu** UI, wiring its Play, Settings,
     *    and Quit buttons to corresponding SceneState transitions.
     *  - Registers and builds the **Pause Menu** UI, providing callbacks for
     *    Resume, Main Menu, and Quit actions.
     *  - Registers and builds the **Settings Menu**, currently containing a
     *    single "Back" button that returns the user to the Main Menu.
     *  - Sets the initial scene to SceneState::MainMenu so the game boots into
     *    the title screen.
     *
     * Each menu's Build() function receives its respective GuiSystem instance
     * and lambda callbacks pointing to SetState(), enabling the SceneManager to
     * control high-level navigation flow without embedding any UI layout logic.
     *
     * @param app_      Pointer to the owning GameApp instance.
     * @param renderer  Renderer reference (reserved for future use; currently
     *                  unused in this function but provided for consistency).
     *
     * @note This function assumes that mainMenu, pauseMenu, settingsMenu and
     *       their matching GuiSystem pointers have already been assigned via
     *       SceneManager::Attach().
 */
void SceneManager::Init(GameApp* app_) {
    app = app_;

    // ---------------- MAIN MENU ----------------
    if (mainMenu && mainMenuGui) {
        mainMenu->Build(*mainMenuGui,
            [this]() { UI::ResetPromptsForCutsceneIntro(); SetState(SceneState::CutsceneIntro); },
            [this]() { SetState(SceneState::Tutorial); },
            [this]() { SetState(SceneState::Settings); },
            [this]() { SetState(SceneState::Credits); },
            [this]() { SetState(SceneState::Exiting); }
        );
        mainMenu->LoadLayout("scene/mainMenu.json", *mainMenuGui);
        mainMenu->ReloadTextures(*mainMenuGui);
    }

    // ---------------- PAUSE MENU ----------------
    if (pauseMenu && pauseGui) {
        pauseMenu->Build(*pauseGui,
            [this]() { SetState(pauseBackground); },
            [this]() { SetState(SceneState::MainMenu); },  // main menu
            [this]() { SetState(SceneState::Exiting); }    // quit
        );
        pauseMenu->LoadLayout("scene/pauseMenu.json", *pauseGui);
        pauseMenu->ReloadTextures(*pauseGui);
    }

    // ---------------- LOSE MENU ----------------
    if (loseMenu && loseGui)
    {
        loseMenu->Build(*loseGui,
            [this]() {                 // Retry
                UI::ResetPromptsForRetry();
                RestartGameplay();
                SetState(SceneState::Playing);
            },
            [this]() { SetState(SceneState::MainMenu); },  // Main Menu
            [this]() { SetState(SceneState::Exiting); }    // Quit
        );
        loseMenu->LoadLayout("scene/loseMenu.json", *loseGui);
        loseMenu->ReloadTextures(*loseGui);
        loseMenu->Hide();
    }

    // ---------------- SETTINGS MENU ----------------
    if (settingsMenu && settingsGui) {
        settingsMenu->Build(*settingsGui,
            [this]() { SetState(SceneState::MainMenu); },
            [this]() { SetState(SceneState::Controls); }
        );
        settingsMenu->LoadLayout("scene/settingsMenu.json", *settingsGui);
        settingsMenu->ReloadTextures(*settingsGui);
    }

    // ---------------- CONTROLS MENU ----------------
    if (controlMenu && controlGui) {
        controlMenu->LoadLayout("scene/controlMenu.json", *controlGui);
        controlMenu->Build(*controlGui,
            [this]() { SetState(SceneState::Settings); }
        );
        controlMenu->ReloadTextures(*controlGui);
    }

    // ----------------- CREDITS MENU ----------------
    if (creditsMenu && creditsGui) {
        creditsMenu->LoadLayout("scene/creditsMenu.json", *creditsGui);
        creditsMenu->Build(*creditsGui,
            [this]() { SetState(SceneState::MainMenu); }
        );
        creditsMenu->ReloadTextures(*creditsGui);
    }

    // ---------------- TUTORIAL MENU ----------------
    if (tutorialMenu && tutorialGui) {
        tutorialMenu->Build(*tutorialGui, *app,
            [this]() { SetState(SceneState::MainMenu); },
            [this]() { UI::ResetPromptsForCutsceneIntro(); SetState(SceneState::CutsceneIntro); }
        );
    }

    // ---------------- CONFIRM QUIT DIALOG ----------------
    if (confirmQuit && confirmGui) {
        confirmQuit->LoadLayout("scene/confirmDialog.json", *confirmGui);
        confirmQuit->Build(*confirmGui);
        confirmQuit->ReloadTextures(*confirmGui);
    }

    // Default paths
    if (editScenePath.empty())
        editScenePath = bootGameplayScene;

    editorIsPlaying = false;
    editorIsPaused = false;
    playScenePath.clear();
    runtimeScenePath.clear();

    if (!introCutscenes.LoadConfig("scene/cutscenes.json", "intro", [this]() { SetState(SceneState::Playing); })) {
        introCutscenes.Init({ "Cutscene1", "Cutscene2", "Cutscene3" }, 2.5f, [this]() { SetState(SceneState::Playing); });
    }
    introCutscenes.SetFirstSlideZoomRate(0.03f);
    if (!endCutscenes.LoadConfig("scene/cutscenes.json", "end", [this]() { SetState(SceneState::MainMenu); })) {
        endCutscenes.Init({ "EndCutscene1", "EndCutscene2", "EndCutscene3" }, 5.0f, [this]() { SetState(SceneState::MainMenu); });
    }
    endCutscenes.SetFirstSlideZoomRate(0.0f);
    gameplayFade.Init();

    SetState(SceneState::MainMenu);
}

/**
    * @brief Routes per-frame input updates to the correct active UI system.
    *
    * This function ensures only the GUI belonging to the current SceneState
    * receives mouse/keyboard events. The routing rules are:
    *
    *  - MainMenu → update its GuiSystem
    *  - Settings → update the settings GuiSystem
    *  - Playing  → update in-game HUD (Mute / Exit) if enabled
    *  - Paused   → update the pause menu overlay
    *
    * @param in The current frame's input state (mouse/keyboard).
    *
*/
void SceneManager::Update(const eng::Input& in, double dt) {

    Vector2 vpPos = { -1, -1 };
    Vector2 vpSize = { -1, -1 };
#if ENABLE_EDITOR
    if (app && app->GetUiEditor() && app->GetUiEditor()->IsVisible()) {
        float x, y, w, h;
        app->GetUiEditor()->GetViewportRect(x, y, w, h);
        vpPos = { x, y };
        vpSize = { w, h };
    }
#endif

    // --- Global ESC handling for pause ---
    if (current == SceneState::Playing && (in.isKeyPressed(GLFW_KEY_ESCAPE) || in.isGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_START))) {
        SetState(SceneState::Paused);
        return;
    }
    if (current == SceneState::Paused && (in.isKeyPressed(GLFW_KEY_ESCAPE) || in.isGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_START))) {
        SetState(pauseBackground);
        return;
    }

    // CONFIRM QUIT
    if (current == SceneState::ConfirmQuit) {
        if (confirmQuit && confirmGui) {
            confirmGui->navMode = NavMode::Horizontal;
            confirmQuit->Update(in, dt, *confirmGui, vpPos, vpSize);
        }
        return;
    }

    // LOSE MENU
    if (IsLose()) {
        if (loseGui) {
            loseGui->navMode = NavMode::Vertical;
            loseGui->Update(in, dt, vpPos, vpSize);
        }
        return;
    }

    if (IsCutsceneIntro()) {
        introCutscenes.Update(dt);
        return;
    }
    if (IsCutsceneEnd()) {
        endCutscenes.Update(dt);
        return;
    }

    // SCENE TRANSITION 
    CommitPendingSceneLoad();

    if (IsPlaying() && UI::ConsumeEndCutsceneRequest()) {
        SetState(SceneState::CutsceneEnd);
        return;
    }

    // MAIN MENU
    if (IsMainMenu()) {
		//mainMenu->UpdateGamepad(*mainMenuGui, dt);
        if (mainMenuGui) {
            mainMenuGui->navMode = NavMode::Vertical;
            mainMenuGui->Update(in, dt, vpPos, vpSize);
        }
        return;
    }

    // SETTINGS
    if (IsSettings()) {
        if (settingsGui) {
            settingsGui->navMode = NavMode::Vertical;
            settingsGui->Update(in, dt, vpPos, vpSize);
        }

        if (settingsMenu && settingsGui)
            settingsMenu->Update(in, *settingsGui);
        return;
    }

    // CONTROLS
    if (IsControls()) {
        if (controlMenu)
            controlMenu->Update(in);

        if (controlGui) {
            controlGui->navMode = NavMode::Horizontal;
            controlGui->Update(in, dt, vpPos, vpSize);
        }
        return;
    }

    // CREDITS
    if (IsCredits()) {
        if (creditsMenu)
            creditsMenu->Update(in);

        if (creditsGui) {
            creditsGui->navMode = NavMode::Vertical;
            creditsGui->Update(in, dt, vpPos, vpSize);
        }
        return;
    }

    // TUTORIAL
    if (IsTutorial()) {
        if (tutorialMenu && tutorialGui) {
			tutorialGui->navMode = NavMode::Horizontal;
            tutorialMenu->Update(in, dt, *tutorialGui, vpPos, vpSize);
        }
        return;
    }

    if (IsPlaying()) gameplayFade.Update(dt);

    // PAUSE MENU
    if (IsPaused()) {
        if (pauseGui) {
            pauseGui->navMode = NavMode::Horizontal;
            pauseGui->Update(in, dt, vpPos, vpSize);
        }
    }

    // HUD (Mute / Exit buttons)
    if (app && app->IsHUDVisible()) {
        app->GetHUDGui().Update(in, dt, vpPos, vpSize);
    }
}

/**
    * @brief Renders the scene based on the current SceneState.
    *
    * Behavior per state:
    *  - MainMenu / Settings: Draws the main menu UI.
    *  - Playing:             Draws gameplay only (no menus).
    *  - Paused:              Draws gameplay, then overlays the pause menu.
    *  - Exiting:             Does nothing; GameApp will handle shutdown.
    *
    * @param renderer Reference to the active Renderer.
    *
    * @note This function must be called every frame. It does not perform
    *       updates—only rendering based on scene state.
*/
void SceneManager::Draw(Renderer& renderer) {
    if (!app) return;

    switch (current) {
    case SceneState::MainMenu:
        if (mainMenu && mainMenuGui)
            mainMenu->Draw(renderer, *mainMenuGui);
        break;
    case SceneState::CutsceneIntro:
        introCutscenes.Draw(renderer);
        break;
    case SceneState::CutsceneEnd:
        endCutscenes.Draw(renderer);
        break;
    case SceneState::Settings:
        if (settingsMenu && settingsGui)
            settingsMenu->Draw(renderer, *settingsGui);
        break;

    case SceneState::Controls:
        if (controlMenu && controlGui)
            controlMenu->Draw(renderer, *controlGui);
        break;

    case SceneState::Credits:
        if (creditsMenu && creditsGui)
            creditsMenu->Draw(renderer, *creditsGui);
        break;

    case SceneState::Tutorial:
        if (tutorialMenu && tutorialGui)
            tutorialMenu->Draw(renderer, *tutorialGui);
        break;

    case SceneState::Playing:
        // Only gameplay
        app->DrawGameplay(renderer);
        gameplayFade.Draw(renderer);
        break;

    case SceneState::Paused:
        // Gameplay + overlay pause menu 
        if (pauseBackground == SceneState::Tutorial) {
            if (tutorialMenu && tutorialGui)
                tutorialMenu->Draw(renderer, *tutorialGui);
        }
        else {
            app->DrawGameplay(renderer);
        }
        if (pauseMenu && pauseGui)
            pauseMenu->Draw(renderer, *pauseGui);
        break;

    case SceneState::Lose:
        app->DrawGameplay(renderer);
        if (loseMenu && loseGui)
            loseMenu->Draw(renderer, *loseGui);
        break;

    case SceneState::ConfirmQuit: {
        // Draw what's behind the modal 
        switch (beforeConfirm) {
        case SceneState::Playing:
            app->DrawGameplay(renderer);
            break;

        case SceneState::Paused:
            if (pauseBackground == SceneState::Tutorial) {
                if (tutorialMenu && tutorialGui)
                    tutorialMenu->Draw(renderer, *tutorialGui);
            }
            else {
                app->DrawGameplay(renderer);
            }
            if (pauseMenu && pauseGui)
                pauseMenu->Draw(renderer, *pauseGui);
            break;

        case SceneState::Settings:
            if (settingsMenu && settingsGui)
                settingsMenu->Draw(renderer, *settingsGui);
            break;

        case SceneState::Tutorial:
            if (tutorialMenu && tutorialGui)
                tutorialMenu->Draw(renderer, *tutorialGui);
            break;

        case SceneState::Credits:
            if (creditsMenu && creditsGui)
                creditsMenu->Draw(renderer, *creditsGui);
            break;

        case SceneState::Lose:
            app->DrawGameplay(renderer);
            if (loseMenu && loseGui)
                loseMenu->Draw(renderer, *loseGui);
            break;

        case SceneState::MainMenu:
        default:
            if (mainMenu && mainMenuGui)
                mainMenu->Draw(renderer, *mainMenuGui);
            break;
        }

        // Draw modal on top
        int ww = 1280, wh = 720;
        glfwGetWindowSize(glfwGetCurrentContext(), &ww, &wh);
        confirmQuit->Draw(renderer, *confirmGui, ww, wh);
        break;
    }

    case SceneState::Exiting:
        // GameApp will close window.
        break;
    }
}

/**
 * @brief Requests a gameplay scene to be loaded with optional spawn door.
 *
 * Schedules a scene change to occur on the next Update() call.
 * The scene will be loaded via SceneRuntime::LoadScene().
 *
 * If a spawnDoorName is provided:
 * - The DoorSystem will position the player at that door
 * - A 2-second cooldown prevents immediate re-triggering
 *
 * @param sceneFile     Path to the scene JSON file to load.
 * @param spawnDoorName Name of the door to spawn the player at (empty for default spawn).
 *
 */
void SceneManager::RequestGameplayScene(const std::string& sceneFile, const std::string& spawnDoorName, bool skipPuzzleSave) {
    pending.sceneFile = sceneFile;
    pending.spawnDoorName = spawnDoorName;
    pending.hasSpawn = !spawnDoorName.empty();
    pending.skipPuzzleSave = skipPuzzleSave;
    hasPending = true;
}

/**
 * @brief Requests a gameplay scene with automatic door spawn determination.
 *
 * Analyzes the transition between previous and next scenes to determine
 * the appropriate spawn door name:
 * - Labyrinth → RoomN: Spawn at "ExitToLabyrinth" in the room
 * - RoomN → Labyrinth: Spawn at "DoorToRoomX" in labyrinth (X = room index)
 * - RoomN → RoomM: Spawn at "ExitToLabyrinth" in destination room (future use, currently just for show)
 *
 * @param nextFile     Path to the scene file being loaded.
 * @param previousFile Path to the current/previous scene file.
 *
 */
void SceneManager::RequestGameplaySceneWithAutoSpawn(const std::string& nextFile, const std::string& previousFile, bool skipPuzzleSave) {
    std::string spawnDoorName;

    if (previousFile.find("labyrinth.json") != std::string::npos &&
        nextFile.find("entities_Level") != std::string::npos)
        spawnDoorName = "ExitToLabyrinth";  // Labyrinth -> RoomN

    else if (nextFile.find("labyrinth.json") != std::string::npos &&
        previousFile.find("entities_Level") != std::string::npos)
        spawnDoorName = DoorToRoomFromPrevious(previousFile); // RoomN -> Labyrinth

    else if (previousFile.find("entities_Level") != std::string::npos &&
        nextFile.find("entities_Level") != std::string::npos)
        spawnDoorName = "ExitToLabyrinth"; // RoomN -> RoomM direct : treat as “arrive in room at ExitToLabyrinth”

    RequestGameplayScene(nextFile, spawnDoorName, skipPuzzleSave);
}

/**
 * @brief Executes any pending scene load request.
 *
 * Processes scene changes that were scheduled via RequestGameplayScene():
 * 1. Loads the requested scene using SceneRuntime::LoadScene()
 * 2. Updates internal tracking of current gameplay scene
 * 3. Sets up door spawning if requested with 2-second cooldown
 */
void SceneManager::CommitPendingSceneLoad() {
    if (!app || !hasPending) return;

    if ((current == SceneState::Playing || current == SceneState::Paused) && !currentGameplayScene.empty())
        editScenePath = currentGameplayScene;

    hasPending = false;

    SceneRuntime::LoadScene(*app, pending.sceneFile, pending.skipPuzzleSave);
    currentGameplayScene = pending.sceneFile;

    // --- AUDIO: switch BGM for the new scene ---
    if (current == SceneState::Playing || current == SceneState::Paused)
    {
        const char* key = GetBgmKeyForScene(currentGameplayScene);
        if (!key)
        {
            s_currentGameplayBGMKey.clear();
            ResourceManager::StopBgm(200);
        }
        else {
            if (s_currentGameplayBGMKey != key)
            {
                s_currentGameplayBGMKey = key;
                ResourceManager::StopBgm(200);
                ResourceManager::PlayBgm(key, true, 0.40f);
            }
        }
    }
    else {
        s_currentGameplayBGMKey.clear();
    }

    if (pending.hasSpawn) {
        app->GetDoorSystem().SetPendingSpawnDoor(pending.spawnDoorName, 2);
    }

#if ENABLE_EDITOR
    app->SyncEditorToActiveScene();
#endif
}

/**
 * @brief Fully resets gameplay state and returns to the initial scene.
 *
 * Performs a complete gameplay reset when returning to main menu:
 * 1. Calls GameApp::ResetGameplayWorld() to clear entities and reset state
 * 2. Updates internal tracking to boot scene
 * 3. Schedules load of labyrinth.json with no spawn door
 * 4. Resets editor play/pause state if editor is enabled
 *
 * This ensures the game always starts from a clean, predictable state
 * when entering gameplay from the main menu.
 */
void SceneManager::RestartGameplay() {
    if (!app) return;

    // reset low-level gameplay state
    DispatchWorldAction(WorldActionType::ResetWorld);

    //// reset low-level gameplay state (entities, flags, player refs, etc.)
    //app->ResetGameplayWorld();

    editorIsPlaying = false;
    editorIsPaused = false;
    playScenePath.clear();
    runtimeScenePath.clear();

#if ENABLE_EDITOR
    if (EditorOverlay* ed = app->GetEditorOverlay()) {
        ed->SetIsPlaying(false);
        ed->SetIsPaused(false);
    }
#endif

    // canonical boot gameplay scene
    currentGameplayScene = bootGameplayScene;
    editScenePath = bootGameplayScene;

    // schedule a clean load back to labyrinth, no spawn door
    RequestGameplayScene(bootGameplayScene, "");
}

/**
* @brief Returns the file path associated with the current active scene state.
*
* @return A reference to the active scene's JSON path string.
*/
const std::string& SceneManager::GetActiveScenePath() const {
    switch (current) {
    case SceneState::MainMenu: {
        static const std::string mainMenuPath = "scene/mainMenu.json";
        return mainMenuPath;
    }
    case SceneState::Paused: {
        static const std::string pauseMenuPath = "scene/pauseMenu.json";
        return pauseMenuPath;
    }
    case SceneState::Lose: {
        static const std::string loseMenuPath = "scene/loseMenu.json";
        return loseMenuPath;
    }
    case SceneState::Settings: {
        static const std::string settingsMenuPath = "scene/settingsMenu.json";
        return settingsMenuPath;
    }
    case SceneState::Controls: {
        static const std::string controlsMenuPath = "scene/controlMenu.json";
        return controlsMenuPath;
    }
    case SceneState::Tutorial: {
        static const std::string tutorialMenuPath = "scene/tutorialMenu.json";
        return tutorialMenuPath;
    }
    case SceneState::ConfirmQuit: {
        static const std::string confirmPath = "scene/confirmDialog.json";
        return confirmPath;
    }
    case SceneState::Playing: {
        if (!currentGameplayScene.empty())
            return currentGameplayScene;
        break;
    }
    default:
        break;
    }


    if (editorIsPlaying && !currentGameplayScene.empty())
        return currentGameplayScene;

    // editor UI logic
    return editorIsPlaying ? playScenePath : editScenePath;
}


#if ENABLE_EDITOR
/**
 * @brief Processes deferred editor actions queued during the frame.
 *
 * This function handles editor commands that were queued during ImGui rendering,
 * ensuring they are executed at a safe point in the frame cycle. Commands are
 * processed in the order they were queued.
 *
 * Supported command types:
 * - ChangeLevel: Loads a new level with automatic door spawn logic
 * - Play: Enters play mode with current scene
 * - TogglePause: Toggles pause state during play mode
 * - Stop: Exits play mode and restores editor state
 *
 * The function handles state transitions between editor and play modes,
 * manages scene path tracking, and coordinates with the EditorOverlay.
 */
void SceneManager::HandleDeferredEditorActions() {
    if (!app) return;

    bool stopRequested = false;

    // process queued commands in-order
    for (const EditorCmd& cmd : editorCmdQueue) {
        switch (cmd.type) {
        case EditorCmdType::ChangeLevel:
        {
            const std::string& path = cmd.levelPath;
            if (path.empty()) break;

            // Block only when simulation is actively running (not paused)
            if (editorIsPlaying && !editorIsPaused)
                break;

            // If we were paused in Play mode, convert Pause->Stop(editor) before switching
            if (editorIsPlaying && editorIsPaused) {
                editorIsPlaying = false;
                editorIsPaused = false;
                playScenePath.clear();
                runtimeScenePath.clear();

                if (EditorOverlay* ed = app->GetEditorOverlay()) {
                    ed->SetIsPlaying(false);
                    ed->SetIsPaused(false);
                }
            }

            std::string previous = editScenePath;
            if (previous.empty()) previous = bootGameplayScene;

            editScenePath = path;

            // SceneManager does the actual load (with door auto-spawn)
            // Skip puzzle save because we are in Edit Mode (or returning to it), so we don't want runtime states to persist.
            RequestGameplaySceneWithAutoSpawn(path, previous, true);

            // Rebuild overlay to bind to new ECS context
            DispatchWorldAction(WorldActionType::RecreateEditorOverlay);

            if (app) {
                app->GetUndoRedo().Clear();
                if (app->GetPuzzleSystem()) {
                    app->GetPuzzleSystem()->Reset();
                }
            }

            if (EditorOverlay* ed = app->GetEditorOverlay())
                ed->currentLevelPath = editScenePath;

        } break;

        case EditorCmdType::Play:
        {
            if (editorIsPlaying) break;

            // pick play path from editor UI if available
            if (EditorOverlay* ed = app->GetEditorOverlay()) {
                if (!ed->currentLevelPath.empty())
                    playScenePath = ed->currentLevelPath;
                else
                    playScenePath = editScenePath;
            }
            else {
                playScenePath = editScenePath;
            }

            runtimeScenePath = playScenePath;
            editScenePath = playScenePath;

            app->CapturePlayerSnapshot();
            DispatchWorldAction(WorldActionType::ResetHUD);
            editorIsPlaying = true;
            editorIsPaused = false;

            if (EditorOverlay* ed = app->GetEditorOverlay()) {
                ed->SetIsPlaying(true);
                ed->SetIsPaused(false);
                ed->currentLevelPath = runtimeScenePath;
            }

        } break;

        case EditorCmdType::TogglePause:
        {
            if (!editorIsPlaying) break;

            editorIsPaused = !editorIsPaused;

            if (EditorOverlay* ed = app->GetEditorOverlay())
                ed->SetIsPaused(editorIsPaused);

        } break;

        case EditorCmdType::Stop:
        {
            stopRequested = true;
        } break;
        }
    }

    editorCmdQueue.clear();

    if (stopRequested) {
        if (editorIsPlaying) {

            // Restore scene to what was being played/edited
            std::string path = playScenePath.empty()
                ? (editScenePath.empty() ? bootGameplayScene : editScenePath)
                : playScenePath;

            // If it's a room, go back to labyrinth
            if (path.find("entities_Level") != std::string::npos) {
                std::string roomPath = path;
                path = bootGameplayScene; // Revert to labyrinth
                
                // load with auto spawn to get the right door
                RequestGameplaySceneWithAutoSpawn(path, roomPath, true);
            } else {
                // load without door spawn
                RequestGameplayScene(path, "", true);
            }

            runtimeScenePath = path;
            editScenePath = path;

            DispatchWorldAction(WorldActionType::RecreateEditorOverlay);
            if (app) {
                app->GetUndoRedo().Clear();
                if (app->GetPuzzleSystem()) {
                    app->GetPuzzleSystem()->Reset();
                }
            }

            if (EditorOverlay* ed = app->GetEditorOverlay()) {
                ed->currentLevelPath = path;
                ed->SetIsPlaying(false);
                ed->SetIsPaused(false);
            }

            DispatchWorldAction(WorldActionType::RestorePlayerSnapshot);
            DispatchWorldAction(WorldActionType::ResetHUD);
            if (app) app->ResetPlayerSpeedPresetOutside();

            editorIsPlaying = false;
            editorIsPaused = false;
            playScenePath.clear();
            runtimeScenePath.clear();
        }
    }
}


EditorPlayControlsState SceneManager::GetPlayControlsState() const {
    EditorPlayControlsState s;
    s.isPlaying = editorIsPlaying;
    s.isPaused = editorIsPaused;
    return s;
}

#endif



/**
 * @brief Dispatches world-level actions to the GameApp.
 *
 * This function serves as a bridge between SceneManager and GameApp for
 * operations that require direct manipulation of the game world. It routes
 * action requests to appropriate GameApp methods.
 *
 * Supported action types:
 * - RecreateEditorOverlay: Rebuilds the editor UI overlay (editor-only)
 * - ResetWorld: Resets gameplay world state via GameApp::ResetGameplayWorld()
 * - RestorePlayerSnapshot: Restores player position via GameApp::RestorePlayerSnapshot()
 *
 * @param[in] act The type of world action to perform.
 */
void SceneManager::DispatchWorldAction(WorldActionType act) {
    if (!app) return;

    switch (act) {
    case WorldActionType::RecreateEditorOverlay:
#if ENABLE_EDITOR
        app->RecreateEditorOverlay();
#endif
        break;

    case WorldActionType::ResetWorld:
        app->ResetGameplayWorld();
        break;

    case WorldActionType::RestorePlayerSnapshot:
        app->RestorePlayerSnapshot();
        break;

    case WorldActionType::ResetHUD:
        app->playerHUD.Reset();
        if (GameApp::playerEntity != INVALID_ENTITY) {
            if (PlayerController* pc = app->GetController(GameApp::playerEntity)) {
                pc->Reset();
            }
        }
        break;

    default:
        break;
    }
}

#if ENABLE_EDITOR
void SceneManager::ToggleEditorsForActiveScene() {
    if (!app) return;
    EditorOverlay* editor = app->GetEditorOverlay();
    RoomEditor* roomEd = app->GetRoomEditor();
    UiEditor* uiEd = app->GetUiEditor();

    const std::string& sp = GetActiveScenePath();
    const bool inRoom = (sp.find("entities_Level") != std::string::npos);
    const bool inLab  = (sp.find("labyrinth.json") != std::string::npos);
    const bool inUi   = (sp.find("Menu.json") != std::string::npos) || 
                        (sp.find("menu.json") != std::string::npos) || 
                        (sp.find("Dialog.json") != std::string::npos) || 
                        (sp.find("dialog.json") != std::string::npos) || 
                        (sp.find("tut") != std::string::npos);

    if (inRoom) {
        if (roomEd) roomEd->ToggleVisible();
        if (editor && editor->IsVisible()) editor->ToggleVisible();
        if (uiEd) uiEd->SetVisible(false);
    } else if (inLab) {
        if (editor) editor->ToggleVisible();
        if (roomEd) roomEd->SetVisible(false);
        if (uiEd) uiEd->SetVisible(false);
    } else if (inUi) {
        if (uiEd) uiEd->ToggleVisible();
        if (editor && editor->IsVisible()) editor->ToggleVisible();
        if (roomEd) roomEd->SetVisible(false);
    } else {
        // Fallback for unknown scene types (e.g. tutorials)
        if (editor && editor->IsVisible()) editor->ToggleVisible();
        if (roomEd) roomEd->SetVisible(false);
        if (uiEd) uiEd->SetVisible(false);
    }
}
#endif
#if ENABLE_EDITOR
void SceneManager::DrawEditorFrame(Renderer& renderer, float dt, bool usingEditor) {
    if (!app) return;
    HandleDeferredEditorActions();
    EditorOverlay* editor = app->GetEditorOverlay();
    RoomEditor* roomEd = app->GetRoomEditor();
    UiEditor* uiEd = app->GetUiEditor();

    if (editor && editor->IsVisible())
        Utility::ProvideEditorSceneTexture(editor, renderer, usingEditor);
    if (roomEd && roomEd->IsVisible())
        Utility::ProvideEditorSceneTexture(roomEd, renderer, usingEditor);
    if (uiEd && uiEd->IsVisible())
        Utility::ProvideEditorSceneTexture(uiEd, renderer, usingEditor);
    
    if (!ImGuiHost::Get().IsReady()) return;
    const bool anyEditorVisible =
        (editor && editor->IsVisible()) ||
        (roomEd && roomEd->IsVisible()) ||
        (uiEd && uiEd->IsVisible());

    if (!anyEditorVisible) return;
    ImGuiHost::Get().BeginFrame();
    if (editor && editor->IsVisible())
        editor->Draw(dt);
    if (roomEd && roomEd->IsVisible()) {
        roomEd->Update(dt);
        roomEd->Draw();
    }
    if (uiEd && uiEd->IsVisible()) {
        uiEd->Update(dt);
        uiEd->Draw();
    }
    ImGuiHost::Get().EndFrame();
}
#endif
#if ENABLE_EDITOR
void SceneManager::CommitEditorChangesNow()
{
    // happens during Update()
    HandleDeferredEditorActions();
    CommitPendingSceneLoad();
}
#endif
