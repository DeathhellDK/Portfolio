#pragma once
/**
 * @file      ainMenu.h
 * @author    Sng Swee Yong Dillon
 * @co-author Woh Kye Le
 * @email     sweeyongdillon.sng, w.kyele 
 * @date      2025-11-29
 *
 * @brief declares the MainMenu class, which encapsulates the layout, state, and behavior of the game�s main menu interface. 
 * Defines all data needed to build and display a full-screen UI consisting of a textured background and interactable Play, Settings, and Quit buttons
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
#include "Graphics/mesh2d.h"

class Renderer;

struct MainMenuLayout {
    // Relative size of all buttons (0..1 of window)
    Vector2 btnRelSize = Vector2(0.20f, 0.08f);

    // Relative positions (0..1 of window)
    Vector2 playRelPos = Vector2(0.4f,0.404f);
    Vector2 tutorialRelPos = Vector2(0.4f, 0.35f);
    Vector2 settingsRelPos = Vector2(0.4f,0.3f);
    Vector2 creditsRelPos = Vector2(0.4f, 0.25f);
    Vector2 quitRelPos = Vector2(0.4f, 0.196f);

    std::string bgTexture = "mainmenu_bg";
    std::string buttonTexture = "menu_button";

    // button text (label)
    std::string playText = "Play";
    std::string tutorialText = "Tutorial";
    std::string settingsText = "Settings";
    std::string creditsText = "Credits";
    std::string quitText = "Quit Game";
};

class MainMenu
{
public:
    MainMenu() = default;

    void Build(
        GuiSystem& gui,
        std::function<void()> onPlay,
        std::function<void()> onTutorial,
        std::function<void()> onSettings,
        std::function<void()> onCredits,
        std::function<void()> onQuit);

    void Show() { visible = true; }
    void Hide() { visible = false; }
    bool IsVisible() const { return visible; }

    bool LoadLayout(const std::string& path, GuiSystem& gui);
    bool SaveLayout(const std::string& path, const GuiSystem& gui) const;
    void ReloadTextures(GuiSystem& gui);

    void Draw(Renderer& renderer, GuiSystem& gui) const;

    mutable int selectedIndex = 0;
    mutable float navCooldown = 0.0f;

    //void UpdateGamepad(GuiSystem& gui, double dt);
    /*float navDelay = 0.0f;
    float stickY = 0.0f;
    float moveTimer = 0.0f;*/

private:
    bool visible = true;
    // --- layout ---
    MainMenuLayout layout;

    // --- background ---
    Mesh2D bgMesh;      // full-screen quad
    unsigned int bgTex = 0;

    // indices into GuiSystem's button array
    int playIndex = -1;
    int tutorialIndex = -1;
    int settingsIndex = -1;
    int creditsIndex = -1;
    int quitIndex = -1;
};