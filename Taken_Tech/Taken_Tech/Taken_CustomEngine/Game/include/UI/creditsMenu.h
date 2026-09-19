/**
 * @file      creditsMenu.h
 * @author    Jethro Sung
 * @email     sung.h
 * @date      2026-04-4
 * @brief     Declares the CreditMenu class, which manages the credits screen UI.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#pragma once

#include <functional>
#include <string>
#include <vector>
#include "Math/vect2.h"
#include "UI/guiSys.h"
#include "Input/input.h"
#include "Graphics/mesh2d.h"

class Renderer;

struct CreditsLine {
    std::string text;
    Vector2 relPos{ 0.0f, 0.0f };
    float fontSize{ 32.0f };
    unsigned color{ 0xFFFFFFFF };
};

struct CreditsMenuLayout {
    Vector2 btnRelSize{ 0.20f, 0.08f };
    Vector2 backRelPos{ 0.85f, 0.1f };
    Vector2 titleRelPos{ 0.25f, 0.84f };

    std::string bgTexture = "ControlBackground";
    std::string buttonTexture = "menu_button";
    std::string backText = "Back";
    std::string titleText = "Credits";

    std::string font = "default";
    float titleFontSize = 72.0f;
    unsigned titleColor = 0xFFDDA0FF;

    std::vector<CreditsLine> lines{
        { "Taken Tech", { 0.25f, 0.68f }, 42.0f, 0xFFFFFFFF },
        { "Developed by", { 0.25f, 0.58f }, 32.0f, 0xFFDDA0FF },
        { "Sng Swee Yong Dillon", { 0.25f, 0.48f }, 30.0f, 0xFFFFFFFF },
        { "Woh Kye Le", { 0.25f, 0.40f }, 30.0f, 0xFFFFFFFF },
        { "Low JianLin", { 0.25f, 0.32f }, 30.0f, 0xFFFFFFFF },
        { "Jethro Sung", { 0.25f, 0.24f }, 30.0f, 0xFFFFFFFF },
        { "Tan Wei Liang Terril", { 0.25f, 0.16f }, 30.0f, 0xFFFFFFFF }
    };
};

class CreditsMenu {
public:
    CreditsMenu() = default;

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
    CreditsMenuLayout layout;
    std::function<void()> onExitCallback;

    Mesh2D bgMesh;
    unsigned int bgTex = 0;

    int backIndex = -1;

    void BuildCreditsDisplay(GuiSystem& gui);
    void UpdateLayoutPositions(GuiSystem& gui, int ww, int wh);
};
