#pragma once
/**
 * @file     settingsMenu.h
 * @author   Woh Kye Le 
 * @co-author Low Jianlin
 * @email    w.kyele, jianlin.low
 * @date     2025-12-06
 *
 * @brief
 * Declares the SettingsMenu class, a lightweight UI controller responsible for
 * displaying and managing the game's Settings screen. The menu uses normalized
 * layout coordinates and relies on a GuiSystem for button registration and
 * per-frame GUI rendering.
 *
 * Features:
 *  - One-button Settings interface (�Back�)
 *  - Enable/disable visibility state
 *  - Screen-space rendering via Renderer
 *
 * All scene-state transitions are delegated to the SceneManager via callbacks.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include <functional>
#include "Math/vect2.h"
#include "UI/guiSys.h"
#include "Input/input.h"

class Renderer;

struct SettingsMenuLayout {
    // Shared button sizing
    Vector2 btnRelSize{ 0.18f, 0.07f };

    // Button positions (0..1 of window)
    Vector2 gameplayRelPos{ 0.05f, 0.80f };
    Vector2 controlsRelPos{ 0.05f, 0.70f };
    Vector2 audioRelPos{ 0.05f, 0.60f };
    Vector2 backRelPos{ 0.05f, 0.05f };

    // Optional background 
    std::string bgTexture = "";           
    std::string buttonTexture = "menu_button";

    // --- Audio slider layout (0..1 of window) ---
    Vector2 sliderRelPos{ 0.55f, 0.55f };
    Vector2 sliderRelSize{ 0.30f, 0.04f };

    // Label placement relative to slider origin (in rel-space)
    Vector2 audioLabelOffsetRel{ -0.12f, 0.007f };

    // Slider colors (RGBA packed)
    unsigned sliderBarColor = 0xFF595959; 
    unsigned sliderKnobColor = 0xFFFFFFFF;
    unsigned sliderKnobDragColor = 0xFFFF9933; // orange
};

class SettingsMenu{
public:
    SettingsMenu() = default;

    /**
     * @brief Builds and registers the settings menu widgets into the given GuiSystem.
     * @param gui The GUI system to add elements to.
     * @param onBack Callback invoked when the 'Back' button is pressed.
     * @param onOpenControls Callback invoked to open the controls submenu.
     */
    void Build(GuiSystem& gui, std::function<void()> onBack, std::function<void()> onOpenControls);

    /**
     * @brief Makes the settings menu visible and resets its state.
     */
    void Show() { visible = true; currentSection = SettingsSection::Gameplay; audioModeActive = false; draggingMaster = false; draggingBgm = false; draggingSfx = false; }

    /**
     * @brief Hides the settings menu and resets its state.
     */
    void Hide() { visible = false; currentSection = SettingsSection::Gameplay; audioModeActive = false; draggingMaster = false; draggingBgm = false; draggingSfx = false; }

    /**
     * @brief Checks if the settings menu is currently visible.
     * @return True if visible, false otherwise.
     */
    bool IsVisible() const { return visible; }

    /**
     * @brief Loads the menu layout from a JSON configuration file.
     * @param path The path to the layout JSON file.
     * @param gui The GuiSystem to apply loaded properties to.
     * @return True if loaded successfully.
     */
    bool LoadLayout(const std::string& path, GuiSystem& gui);

    /**
     * @brief Saves the current menu layout to a JSON configuration file.
     * @param path The path to the layout JSON file.
     * @param gui The GuiSystem to read properties from.
     * @return True if saved successfully.
     */
    bool SaveLayout(const std::string& path, const GuiSystem& gui) const;

    /**
     * @brief Reloads textures for the GUI elements based on the layout configuration.
     * @param gui The GuiSystem containing the elements.
     */
    void ReloadTextures(GuiSystem& gui);

    /**
     * @brief Updates menu logic, including slider interactions and navigation.
     * @param in Input state for the current frame.
     * @param gui GuiSystem for updating elements.
     */
    void Update(const eng::Input& in, GuiSystem& gui);

    /**
     * @brief Draws the settings menu and its sub-components.
     * @param renderer Renderer used for drawing.
     * @param gui GuiSystem containing the elements.
     */
    void Draw(Renderer& renderer, GuiSystem& gui) const;
    

private:

    enum class SettingsSection {
        Gameplay,
        Controls,
        Audio
    };

    SettingsSection currentSection = SettingsSection::Gameplay;
    bool audioModeActive = false;
    int audioNavButtonIndex = -1;
    int audioFirstSliderIndex = -1;

    void BuildNavigation(GuiSystem& gui, std::function<void()> onBack, std::function<void()> onOpenControls);
    void BuildAudioUI(GuiSystem& gui);

    void UpdateAudioSlider(const eng::Input& in);

    void LayoutNavigation(GuiSystem& gui, int ww, int wh) const;
    void LayoutAudioUI(GuiSystem& gui, int ww, int wh) const;

    void DrawAudio(Renderer& renderer, GuiSystem& gui, int ww, int wh) const;

    float SliderToDb(float slider);
    float DbToSlider(float db);

    bool visible = false;

    SettingsMenuLayout layout;

    // audio slider 
    //float volumeValue = 0.5f; // slider value 0..1
    //bool draggingSlider = false;

    // --- Audio Slider States ---
    float masterVolume = 1.0f;
    float bgmVolume = 1.0f;
    float sfxVolume = 1.0f;

    bool draggingMaster = false;
    bool draggingBgm = false;
    bool draggingSfx = false;
    
    std::function<void()> onBackCallback;

};
