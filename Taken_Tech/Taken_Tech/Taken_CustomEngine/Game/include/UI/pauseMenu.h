#pragma once
/**
 * @file      pauseMenu.h
 * @author    Lim Zhi Jie
 * @co-author Woh Kye Le
 * @email     zhijie.lim, w.kyele
 * @date      2025-11-29
 *
 * @brief declares the PauseMenu class, which encapsulates the layout, state, and interactive elements of the game's in-game pause interface. 
 * It provides functions to build the menu UI (Build()), control its visibility (Show(), Hide(), IsVisible()), and render it (Draw()).
 *
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include <functional>
#include <vector>
#include <string>

#include "Math/vect2.h"
#include "UI/guiSys.h"
#include "Graphics/mesh2d.h"

class Renderer;
class Camera2D;

struct PauseSlideDef{
    std::string texture = "";          
    std::string caption = "";

    // Caption styling (used in DrawSlideshow)
    std::string captionFont = "default";
    float       captionFontSizeRel = 0.10f; // relative to slideH
    unsigned    captionColor = 0xFFFFFFFF;
};


struct PauseMenuLayout {
    // Panel rect (relative to window)
    float panelWRel = 0.70f;
    float panelHRel = 0.50f;

    // Slideshow area (relative to panel)
    float slideWRel = 0.60f;
    float slideHRel = 0.40f;
    float slideXRel = 0.20f;
    float slideYRel = 0.42f;

    // Slides (editable in JSON)
    std::vector<PauseSlideDef> slides;

    // Buttons (relative to panel)
    Vector2 btnRelSize = Vector2(0.25f, 0.20f);

    // NOTE: both X and Y are used (so editor can freely position)
    Vector2 resumeRelPos = Vector2(0.12f, 0.15f);
    Vector2 mainRelPos = Vector2(0.375f, 0.15f);
    Vector2 quitRelPos = Vector2(0.63f, 0.15f);

    // Shared textures (editable in JSON)
    std::string buttonTexture = "menu_button";
    std::string arrowTexture = "menu_button";

    // Arrow buttons (pixel size + offsets relative to slideshow rect)
    Vector2 arrowSizePx = Vector2(40.f, 40.f);
    Vector2 leftArrowOffsetPx = Vector2(-50.f, -20.f);
    Vector2 rightArrowOffsetPx = Vector2(10.f, -20.f);

    // Title placement + style 
    std::string titleId = "pause.title";
    std::string titleText = "HOW TO PLAY";
    std::string titleFont = "default";
    unsigned    titleColor = 0xFFFFFFFF;
    float       titleYRel = 0.95f;   // relative to panel height
    float       titleSize = 32.0f;
};

class PauseMenu {
public:
    PauseMenu() = default;

    void Build(
        GuiSystem& gui,
        std::function<void()> onResume,
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
    bool visible = false;

    PauseMenuLayout layout;

    // store button indices inside gui
    int resumeIndex = -1;
    int mainIndex = -1;
    int quitIndex = -1;


    // "slide show" of texture + textfont panel
    struct HowToSlide {
        std::vector<unsigned> textures;  // textures
        std::string caption;             // text description
    };

    Mesh2D slideshowMesh;               // mesh for slideshow panel
    std::vector<HowToSlide> slides;     // all slides
    int currentSlide = 0;               // which slide is active

    // GUI indices for arrow buttons
    int leftArrowIndex = -1;
    int rightArrowIndex = -1;

    // Title text index
    int howToPlayTextIndex = -1;

    void BuildTexts(GuiSystem& gui);
    void BuildSlideshowMesh();
    void BuildSlides();
    void BuildArrows(GuiSystem& gui);
    void BuildButtons(GuiSystem& gui, std::function<void()> onResume, std::function<void()> onMainMenu, std::function<void()> onQuit);

    Camera2D* BeginScreenSpace(Renderer& renderer) const;
    void EndScreenSpace       (Renderer& renderer, Camera2D* prevCam) const;
    void ComputePanelRect     (int ww, int wh, float& x, float& y, float& w, float& h) const;
    void DrawPanel    (Renderer& renderer, float px, float py, float pw, float ph, int ww, int wh) const;
    void DrawSlideshow(Renderer& renderer, float px, float py, float pw, float ph,int ww, int wh) const;
    void LayoutArrows (GuiSystem& gui, float px, float py, float pw, float ph) const;
    void LayoutButtons(GuiSystem& gui, float px, float py, float pw, float ph) const;
    void LayoutTitle  (GuiSystem& gui, float px, float py, float pw, float ph) const;
    void DrawGUI      (Renderer& renderer, GuiSystem& gui) const;

};