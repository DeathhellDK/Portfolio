#pragma once
/**
 * @file      tutorialMenu.h
 * @author    Woh Kye le
 * @email     w.kyele
 * @date      2026-02-19
 * @brief     Declares the TutorialMenu class, which manages the tutorial screen UI and a playable demo character for testing inputs.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include <functional>
#include "UI/guiSys.h"
#include "Input/input.h"
#include "Math/vect2.h"
#include "Graphics/camera2d.h"
#include "Graphics/mesh2d.h"
#include "Core/componentcontext.h"
#include "UI/playerHUD.h"

class Renderer;
class GameApp;

/**
 * @class TutorialMenu
 * @brief Manages the tutorial menu UI and the demo player within it.
 *
 * This class handles the display and logic for the tutorial menu, including
 * rendering the GUI, handling user input, and managing a demo player entity
 * that showcases player movement and animation.
 */
class TutorialMenu {
public:
    /**
     * @enum TutorialStep
     * @brief Represents the different steps/pages in the tutorial.
     */
    enum class TutorialStep {
        Tut0,
        Tut1,
        Tut2,
        Tut3,
        Tut4,
        Tut5,
        Tut6,
        Tut7,
        Count
    };

    /**
     * @struct TutButtonConfig
     * @brief Button layout configuration for tutorial navigation.
     */
    struct TutButtonConfig {
        std::string id;
        Vector2 relPos{ 0.0f, 0.0f };
        Vector2 relSize{ 0.14f, 0.05f };
        float fontSize{ 25.0f };
    };

    /**
     * @struct Layout
     * @brief Internal layout configuration for the tutorial UI.
     */
    struct Layout {
        Vector2 textboxRelPos{ 0.5f, 0.85f };
        Vector2 textboxRelSize{ 0.6f, 0.2f };
        std::string textboxTexture = "Text_box";

        Vector2 textRelPos{ 0.5f, 0.85f };
        float textScale{ 60.f };
        std::string currentText = "";

        // Button Layout
        float btnBaseFontSize = 25.0f;
        std::vector<TutButtonConfig> buttons;
    };

    /**
     * @brief Constructs a new TutorialMenu object.
     *
     * Initializes the tutorial menu camera with default dimensions.
     */
    TutorialMenu();

    /**
     * @brief Builds the tutorial menu with necessary dependencies.
     *
     * @param gui              Reference to the GuiSystem.
     * @param app              Reference to the GameApp instance.
     * @param onEscapeToPause  Callback function to be executed when the escape key is pressed.
     * @param onPlay           Callback function to be executed when the play button is pressed (only on last tutorial level).
     */
    void Build(GuiSystem& gui, GameApp& app, std::function<void()> onEscapeToPause, std::function<void()> onPlay);

    /**
     * @brief Shows the tutorial menu.
     *
     * Sets the visibility flag to true, enabling updates and rendering.
     */
    void Show() { visible = true; }

    /**
     * @brief Hides the tutorial menu.
     *
     * Sets the visibility flag to false and destroys the demo player entity.
     */
    void Hide();

    /**
     * @brief Checks if the tutorial menu is currently visible.
     *
     * @return true if the menu is visible, false otherwise.
     */
    bool IsVisible() const { return visible; }

    /**
     * @brief Updates the tutorial menu logic.
     *
     * Handles input processing, demo player spawning/updates, and camera adjustments.
     *
     * @param in Reference to the input system.
     * @param gui Reference to the GuiSystem.
     */
    void Update(const eng::Input& in, double dt, GuiSystem& gui, Vector2 viewportPos = { -1, -1 }, Vector2 viewportSize = { -1, -1 });

    /**
     * @brief Draws the tutorial menu and its components.
     *
     * Renders the demo player and the GUI elements.
     *
     * @param renderer Reference to the Renderer.
     * @param gui      Reference to the GuiSystem.
     */
    void Draw(Renderer& renderer, GuiSystem& gui);

    /**
     * @brief Loads the tutorial menu layout from a JSON file.
     * @param path Path to the JSON layout file.
     * @param gui  Reference to the GuiSystem.
     * @return True if loading was successful, false otherwise.
     */
    bool LoadLayout(const std::string& path, GuiSystem& gui);

    /**
     * @brief Reloads textures for the tutorial menu elements.
     * @param gui Reference to the GuiSystem.
     */
    void ReloadTextures(GuiSystem& gui);

    /**
     * @brief Sets the current tutorial step.
     * 
     * Updates the UI text, layout, and spawned entities for the specified step.
     * 
     * @param step The tutorial step to switch to.
     * @param gui  Reference to the GuiSystem.
     */
    void SetTutorialStep(TutorialStep step, GuiSystem& gui);

private:
    /**
     * @brief Spawns the demo player entity.
     *
     * Creates a player entity with necessary components (Controller, Physics, Visuals)
     * for demonstration purposes within the menu.
     */
    void SpawnDemoPlayer();

    /**
     * @brief Destroys the demo player entity and restores previous state.
     */
    void DestroyDemoPlayer();

    /**
     * @brief Advances to the next tutorial step.
     * @param gui Reference to the GuiSystem.
     */
    void NextTutorial(GuiSystem& gui);

    /**
     * @brief Goes back to the previous tutorial step.
     * @param gui Reference to the GuiSystem.
     */
    void PrevTutorial(GuiSystem& gui);
    void LayoutUI(GuiSystem& gui);

    /**
     * @brief Saves carry-over state (player/enemy positions, ability) before a step change.
     * @param nextStep The upcoming tutorial step.
     */
    void SaveCarryoverForStepChange(TutorialStep nextStep);

    /**
     * @brief Resolves the JSON scene path for a tutorial step.
     * @param step The tutorial step.
     * @return Asset-relative path to the JSON scene file.
     */
    std::string GetStepScenePath(TutorialStep step) const;

    /**
     * @brief Loads layout and scene entities for the given path.
     * @param path Asset-relative JSON path.
     * @param gui  GUI system for layout widgets.
     */
    void LoadStepSceneAndEntities(const std::string& path, GuiSystem& gui);

    /**
     * @brief Ensures the tutorial textbox panel exists and is updated.
     * @param gui GUI system.
     */
    void EnsureTextboxPanel(GuiSystem& gui);

    /**
     * @brief Spawns demo player if needed and applies carried player state.
     */
    void RespawnDemoAndApplyCarryover();

    /**
     * @brief Applies carried enemy position to the corpse in Tut3 after scene load.
     */
    void ApplyCorpsePositionIfTut3();

    /**
     * @brief Handles auto transitions for Tut2 and Tut3 based on gameplay events.
     * @param gui GUI system for invoking next/prev transitions.
     */
    void HandleAutoTransitions(GuiSystem& gui);

    PlayerHUD hud;
    bool visible = false;
    GameApp* app = nullptr;
    std::function<void()> escapeCallback;
    std::function<void()> playCallback;

    Camera2D ctrlCam;

    Entity demoEntity = 0;
    Entity previousPlayerEntity = 0;
    bool demoSpawned = false;

    Vector2 carriedPlayerPos{ 0.0f, 0.0f };
    Vector2 carriedMobPos{ 0.0f, 0.0f };
    PlayerAbility carriedAbility = PlayerAbility::DEFAULT;
    bool hasCarriedPos = false;
    Vector2 lastTut2EnemyPos{ 0.0f, 0.0f };
    bool hasTut2EnemyPos = false;

    TutorialStep currentStep = TutorialStep::Tut0;
    bool justChangedStep = false;
    bool tut2EnemySpawned = false;

    unsigned textboxTex = 0;
    Mesh2D textboxMesh;
    Layout layout;
};
