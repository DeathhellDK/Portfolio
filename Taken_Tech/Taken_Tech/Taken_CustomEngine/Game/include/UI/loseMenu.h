#pragma once
/**
 * @file       loseMenu.h
 * @author     Jethro Sung
 * @co-author  Woh Kye Le
 * @email      sung.h, w.kyele
 * @date       2026-02-05
 *
 * @brief     Declares LoseMenu and LoseMenuLayout, the UI shown when the player
 *            loses (e.g., HP reaches 0).
 *
 * LoseMenu is a lightweight screen-space menu that renders:
 *  - A full-screen background texture.
 *  - A title text label (e.g., "YOU DIED").
 *  - Three action buttons: Retry, Main Menu, and Quit Game.
 *
 * The menu uses GuiSystem for widget ownership and input handling, while the
 * background is drawn directly via Mesh2D/Renderer so it always covers the
 * screen independent of the world camera.
 *
 * Layout is data-driven via LoseMenuLayout:
 *  - Relative positions/sizes are expressed in normalized window coordinates.
 *  - Text fonts, sizes, colors, and textures can be changed without modifying
 *    the rendering logic.
 *
 * @version 1.0
 * @copyright Copyright (C) 2026
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */

#include <functional>
#include <string>

#include "Math/vect2.h"
#include "UI/guiSys.h"
#include "Graphics/mesh2d.h"

class Renderer;

struct LoseMenuLayout
{
    // Background
    std::string bgTexture = "LoseScreen";

    // Shared button texture
    std::string buttonTexture = "menu_button";

    // Title text
    std::string titleId = "lose.title";
    std::string titleText = "YOU DIED";
    std::string titleFont = "default";
    float       titleSize = 72.f;
    unsigned    titleColor = 0xFFFFFFFF;

    // Relative title Y position (0..1 of window height)
    float titleRelY = 0.18f;

    // Buttons: relative size (0..1)
    Vector2 btnRelSize{ 0.20f, 0.08f };

    // Buttons: relative positions (0..1)
    Vector2 retryRelPos{ 0.4f, 0.48f };
    Vector2 mainRelPos{ 0.4f, 0.36f };
    Vector2 quitRelPos{ 0.4f, 0.24f };

    // Button labels
    std::string retryText = "Retry";
    std::string mainText = "Main Menu";
    std::string quitText = "Quit Game";
};

class LoseMenu
{
public:
    LoseMenu() = default;

    void Build(
        GuiSystem& gui,
        std::function<void()> onRetry,
        std::function<void()> onMainMenu,
        std::function<void()> onQuit);

    void Show() { visible = true; }
    void Hide() { visible = false; }
    bool IsVisible() const { return visible; }

    bool LoadLayout(const std::string& path, GuiSystem& gui);
    bool SaveLayout(const std::string& path, const GuiSystem& gui) const;
    void ReloadTextures(GuiSystem& gui);

    void Draw(Renderer& renderer, GuiSystem& gui) const;

private:
    bool visible = true;
    LoseMenuLayout layout;

    // Background quad
    Mesh2D bgMesh;
    unsigned int bgTex = 0;

    // Cached indices in GuiSystem arrays
    int titleIndex = -1;
    int retryIndex = -1;
    int mainIndex = -1;
    int quitIndex = -1;
};