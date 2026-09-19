#pragma once
/**
 * @file      controlMenu.h
 * @author    Woh Kye le
 * @co-author Sng Swee Yong Dillon
 * @email     w.kyele, sweeyongdillon.sng
 * @date      2026-02-19
 * @brief     Declares the ControlMenu class, which manages the controls screen UI.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include <functional>
#include <string>
#include "Math/vect2.h"
#include "UI/guiSys.h"
#include "Input/input.h"
#include "Graphics/mesh2d.h"

class Renderer;

struct ControlItem {
    std::string textures;
    Vector2 texturesRelPos{ 0.0f, 0.0f };
    Vector2 texturesRelSize{ 0.09f, 0.09f };
    
    std::string text;
    Vector2 textRelPos{ 0.0f, 0.0f };
    float textFontSize{ 36.0f };
};

struct ControlMenuLayout {
    Vector2 btnRelSize{ 0.20f, 0.08f };
    Vector2 exitRelPos{ 0.85f, 0.1f };
    Vector2 titleRelPos{ 0.25f, 0.8333f };
    float texturesRelSize = 0.09f;

    std::string bgTexture = "ControlBackground";
    std::string buttonTexture = "menu_button";
    std::string exitText = "Back";
    std::string titleText = "Basic Controls";
    
    std::string font = "default";
    float titleFontSize = 83.2f;
    float textFontSize = 36.0f;
    unsigned int titleColor = 0xFFDDA0FF; // Light gold
    unsigned int textColor = 0xFFFFFFFF; // White

    std::string guideTexture = "controllerGuide";
    Vector2     guideRelPos{ 0.50f, 0.09f };
    Vector2     guideRelSize{ 0.42f, 0.82f };

    std::vector<ControlItem> controls;
};

/**
 * @class ControlMenu
 * @brief Manages the Control Menu screen using GuiSystem buttons and text.
 */
class ControlMenu {
public:
    ControlMenu() = default;

    void Build(GuiSystem& gui, std::function<void()> onExit);
    void Show() { visible = true; }
    void Hide() { visible = false; }
    bool IsVisible() const { return visible; }
    bool LoadLayout(const std::string& path, GuiSystem& gui);
    bool SaveLayout(const std::string& path, const GuiSystem& gui) const;
    void ReloadTextures(GuiSystem& gui);
    void Update(const eng::Input& in);
    void Draw(Renderer& renderer, GuiSystem& gui);

private:
    bool visible = false;
    ControlMenuLayout layout;
    std::function<void()> onExitCallback;

    Mesh2D bgMesh;
    unsigned int bgTex = 0;

    int exitIndex = -1;

    void BuildControlsDisplay(GuiSystem& gui);
    void UpdateLayoutPositions(GuiSystem& gui, int ww, int wh);
};
