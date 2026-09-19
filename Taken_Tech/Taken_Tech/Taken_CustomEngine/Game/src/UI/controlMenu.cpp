/**
 * @file      controlMenu.cpp
 * @author    Woh Kye le
 * @co-author Sng Swee Yong Dillon
 * @email     w.kyele, sweeyongdillon.sng
 * @date      2026-02-19
 * @brief     Implements the ControlMenu class functionality.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "UI/controlMenu.h"
#include "Core/utils.h"
#include "Graphics/renderer.h"
#include "Core/resourceManager.h"
#include "Input/DebugConsole.hpp"
#include "Core/assetsPath.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream> 
#include <json.hpp>

using json = nlohmann::json;

void ControlMenu::Build(GuiSystem& gui, std::function<void()> onExit) {
    onExitCallback = onExit;

    // Create background mesh (fullscreen quad [-1,1])
    bgMesh = Utility::BuildQuad(
        { {-1,-1}, {1,-1}, {1,1}, {-1,1} },
        { {1,1,1}, {1,1,1}, {1,1,1}, {1,1,1} }
    );

    // Load background texture
    bgTex = ResourceManager::GetTexture(layout.bgTexture);

    // Debug: Check if background texture loaded
    if (bgTex != 0) {
        DebugConsole::Get().Info("[ControlMenu] Background texture loaded successfully: " + layout.bgTexture + "\n");
    }
    else {
        DebugConsole::Get().Error("[ControlMenu] Failed to load background texture: " + layout.bgTexture + "\n");
    }

    // Create Exit button
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);

    auto& btns = gui.GetButtons();
    exitIndex = (int)btns.size();

    gui.AddButton(GuiButton(
        layout.exitRelPos,
        layout.btnRelSize,
        layout.exitText,
        [onExit]() { if (onExit) onExit(); },
        buttonTex
    ));

    if (exitIndex >= 0 && exitIndex < (int)btns.size()) {
        btns[exitIndex].id = "control.exit";
    }

    // Build controls display
    BuildControlsDisplay(gui);

    // Initial positioning
    int ww = 1280, wh = 720;
    if (GLFWwindow* win = glfwGetCurrentContext()) {
        glfwGetFramebufferSize(win, &ww, &wh);
    }
    UpdateLayoutPositions(gui, ww, wh);
}

void ControlMenu::UpdateLayoutPositions(GuiSystem& gui, int ww, int wh) {
    // Update exit button position
    if (GuiButton* b = gui.FindButton("control.exit")) {
        float x = layout.exitRelPos.x * ww;
        float y = layout.exitRelPos.y * wh;
        float w = layout.btnRelSize.x * ww;
        float h = layout.btnRelSize.y * wh;

        b->pos = Vector2(x - w * 0.5f, y - h * 0.5f);
        b->size = Vector2(w, h);
    }

    // Update title position (1/4 left, 1/6 top)
    if (GuiText* t = gui.FindText("control.title")) {
        t->pos = Vector2(layout.titleRelPos.x * ww, layout.titleRelPos.y * wh);
    }

    // Update control items (icons and descriptions)
    for (size_t i = 0; i < layout.controls.size(); i++) {
        const auto& ctrl = layout.controls[i];

        if (GuiPanel* p = gui.FindPanel("control.icon." + std::to_string(i))) {
            p->pos = Vector2(ctrl.texturesRelPos.x * ww, ctrl.texturesRelPos.y * wh);
            p->size = Vector2(ctrl.texturesRelSize.x * ww, ctrl.texturesRelSize.y * wh);
        }

        if (GuiText* t = gui.FindText("control.desc." + std::to_string(i))) {
            t->pos = Vector2(ctrl.textRelPos.x * ww, ctrl.textRelPos.y * wh);
            t->size = ctrl.textFontSize;
            t->baseSize = ctrl.textFontSize;
        }
    }
}

void ControlMenu::BuildControlsDisplay(GuiSystem& gui) {
    // Main Title
    GuiText title(Vector2(0, 0), layout.titleText, layout.titleFontSize, layout.titleColor, layout.font);
    title.id = "control.title";
    gui.AddText(title);

    // Add all controls from layout
    for (size_t i = 0; i < layout.controls.size(); i++) {
        const auto& ctrl = layout.controls[i];
        
        // Try to load icon texture
        unsigned int iconTex = ResourceManager::GetTexture(ctrl.textures);

        // Create panel for icon (even if texture is not loaded yet)
        GuiPanel iconPanel(
            Vector2(0, 0),
            Vector2(100, 100),
            iconTex
        );
        iconPanel.id = std::string("control.icon.") + std::to_string(i);
        gui.AddPanel(iconPanel);

        // Add description text
        GuiText descText(Vector2(0, 0), ctrl.text, ctrl.textFontSize, layout.textColor, layout.font);
        descText.id = std::string("control.desc.") + std::to_string(i);
        gui.AddText(descText);
    }
}


bool ControlMenu::LoadLayout(const std::string& path, GuiSystem& gui) {
    const std::string fullPath = AssetPath(path);
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[ControlMenu] Failed to open layout: ", fullPath, "\n");
        return false;
    }

    try {
        json j;
        file >> j;

        if (j.contains("btnRelSize") && j["btnRelSize"].is_array()) {
            layout.btnRelSize.x = j["btnRelSize"][0];
            layout.btnRelSize.y = j["btnRelSize"][1];
        }
        if (j.contains("exitRelPos") && j["exitRelPos"].is_array()) {
            layout.exitRelPos.x = j["exitRelPos"][0];
            layout.exitRelPos.y = j["exitRelPos"][1];
        }
        if (j.contains("titleRelPos") && j["titleRelPos"].is_array()) {
            layout.titleRelPos.x = j["titleRelPos"][0];
            layout.titleRelPos.y = j["titleRelPos"][1];
        }
        if (j.contains("texturesRelSize") && j["texturesRelSize"].is_number())
            layout.texturesRelSize = j["texturesRelSize"];
        else if (j.contains("iconRelSize") && j["iconRelSize"].is_number())
            layout.texturesRelSize = j["iconRelSize"];
        
        if (j.contains("bgTexture") && j["bgTexture"].is_string())
            layout.bgTexture = j["bgTexture"];
        if (j.contains("buttonTexture") && j["buttonTexture"].is_string())
            layout.buttonTexture = j["buttonTexture"];
        if (j.contains("guideTexture") && j["guideTexture"].is_string())
            layout.guideTexture = j["guideTexture"];
        if (j.contains("exitText") && j["exitText"].is_string())
            layout.exitText = j["exitText"];
        if (j.contains("titleText") && j["titleText"].is_string())
            layout.titleText = j["titleText"];
        if (j.contains("font") && j["font"].is_string())
            layout.font = j["font"];
        if (j.contains("titleFontSize") && j["titleFontSize"].is_number())
            layout.titleFontSize = j["titleFontSize"];
        if (j.contains("textFontSize") && j["textFontSize"].is_number())
            layout.textFontSize = j["textFontSize"];
        else if (j.contains("descFontSize") && j["descFontSize"].is_number())
            layout.textFontSize = j["descFontSize"];
        if (j.contains("titleColor") && j["titleColor"].is_number())
            layout.titleColor = j["titleColor"];
        if (j.contains("textColor") && j["textColor"].is_number())
            layout.textColor = j["textColor"];

        if (j.contains("guideRelPos") && j["guideRelPos"].is_array()) {
            layout.guideRelPos.x = j["guideRelPos"][0];
            layout.guideRelPos.y = j["guideRelPos"][1];
        }
        if (j.contains("guideRelSize") && j["guideRelSize"].is_array()) {
            layout.guideRelSize.x = j["guideRelSize"][0];
            layout.guideRelSize.y = j["guideRelSize"][1];
        }

        if (j.contains("controls") && j["controls"].is_array()) {
            layout.controls.clear();
            for (const auto& item : j["controls"]) {
                if ((item.contains("textures") || item.contains("icon")) && 
                    (item.contains("text") || item.contains("desc"))) {
                    ControlItem ci;
                    
                    if (item.contains("textures"))
                        ci.textures = item["textures"].get<std::string>();
                    else if (item.contains("icon"))
                        ci.textures = item["icon"].get<std::string>();

                    if (item.contains("text"))
                        ci.text = item["text"].get<std::string>();
                    else if (item.contains("desc"))
                        ci.text = item["desc"].get<std::string>();
                    
                    if (item.contains("texturesPos")) {
                        ci.texturesRelPos.x = item["texturesPos"][0].get<float>();
                        ci.texturesRelPos.y = item["texturesPos"][1].get<float>();
                    }
                    else if (item.contains("iconPos")) {
                        ci.texturesRelPos.x = item["iconPos"][0].get<float>();
                        ci.texturesRelPos.y = item["iconPos"][1].get<float>();
                    }

                    if (item.contains("texturesSize") && item["texturesSize"].is_array()) {
                        ci.texturesRelSize.x = item["texturesSize"][0].get<float>();
                        ci.texturesRelSize.y = item["texturesSize"][1].get<float>();
                    }
                    else if (item.contains("iconSize") && item["iconSize"].is_array()) {
                        ci.texturesRelSize.x = item["iconSize"][0].get<float>();
                        ci.texturesRelSize.y = item["iconSize"][1].get<float>();
                    }

                    if (item.contains("textPos")) {
                        ci.textRelPos.x = item["textPos"][0].get<float>();
                        ci.textRelPos.y = item["textPos"][1].get<float>();
                    }
                    else if (item.contains("descPos")) {
                        ci.textRelPos.x = item["descPos"][0].get<float>();
                        ci.textRelPos.y = item["descPos"][1].get<float>();
                    }
                    if (item.contains("textSize") && item["textSize"].is_number()) {
                        ci.textFontSize = item["textSize"].get<float>();
                    }
                    else if (item.contains("descSize") && item["descSize"].is_number()) {
                        ci.textFontSize = item["descSize"].get<float>();
                    }
                    layout.controls.push_back(ci);
                }
            }
        }

        if (exitIndex >= 0) {
            GuiButton* btn = gui.FindButton("control.exit");
            if (btn) {
                if (j.contains("exitFontSize") && j["exitFontSize"].is_number()) {
                    btn->fontSize = j["exitFontSize"].get<float>();
                    btn->baseFontSize = btn->fontSize;
                }
                if (j.contains("exitFont") && j["exitFont"].is_string())
                    btn->font = j["exitFont"].get<std::string>();
            }
        }

        ReloadTextures(gui);

        DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[ControlMenu] Layout loaded from ", fullPath, "\n");
        return true;
    }
    catch (const std::exception& e) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[ControlMenu] JSON parse error: ", e.what(), "\n");
        return false;
    }
}

bool ControlMenu::SaveLayout(const std::string& path, const GuiSystem& gui) const {
    (void)gui;
    json j;
    j["btnRelSize"] = { layout.btnRelSize.x, layout.btnRelSize.y };
    j["exitRelPos"] = { layout.exitRelPos.x, layout.exitRelPos.y };
    j["titleRelPos"] = { layout.titleRelPos.x, layout.titleRelPos.y };
    j["texturesRelSize"] = layout.texturesRelSize;
    j["bgTexture"] = layout.bgTexture;
    j["buttonTexture"] = layout.buttonTexture;
    j["guideTexture"] = layout.guideTexture;
    j["exitText"] = layout.exitText;
    j["titleText"] = layout.titleText;
    j["font"] = layout.font;
    j["titleFontSize"] = layout.titleFontSize;
    j["textFontSize"] = layout.textFontSize;
    j["titleColor"] = layout.titleColor;
    j["textColor"] = layout.textColor;
    j["guideRelPos"] = { layout.guideRelPos.x, layout.guideRelPos.y };
    j["guideRelSize"] = { layout.guideRelSize.x, layout.guideRelSize.y };

    json controlsArray = json::array();
    for (const auto& ctrl : layout.controls) {
        json item;
        item["textures"] = ctrl.textures;
        item["texturesPos"] = { ctrl.texturesRelPos.x, ctrl.texturesRelPos.y };
        item["texturesSize"] = { ctrl.texturesRelSize.x, ctrl.texturesRelSize.y };
        item["text"] = ctrl.text;
        item["textPos"] = { ctrl.textRelPos.x, ctrl.textRelPos.y };
        item["textSize"] = ctrl.textFontSize;
        controlsArray.push_back(item);
    }
    j["controls"] = controlsArray;

    if (const GuiButton* b = gui.FindButton("control.exit")) {
        j["exitFontSize"] = b->baseFontSize;
        j["exitFont"] = b->font;
    }

    const std::string fullPath = AssetPath(path);
    std::ofstream file(fullPath);
    if (!file.is_open()) return false;
    file << j.dump(4);
    return true;
}

void ControlMenu::ReloadTextures(GuiSystem& gui) {
    bgTex = ResourceManager::GetTexture(layout.bgTexture);
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);

    if (GuiButton* b = gui.FindButton("control.exit")) {
        b->texture = buttonTex;
    }

    // Reload control icons
    for (size_t i = 0; i < layout.controls.size(); i++) {
        if (GuiPanel* p = gui.FindPanel("control.icon." + std::to_string(i))) {
            p->texture = ResourceManager::GetTexture(layout.controls[i].textures);
        }
    }
}

void ControlMenu::Update(const eng::Input& in) {
    if (!visible) return;

    if (in.isKeyPressed(GLFW_KEY_ESCAPE)) {
        if (onExitCallback) {
            onExitCallback();
        }
    }
}

void ControlMenu::Draw(Renderer& renderer, GuiSystem& gui) {
    if (!visible) return;

    // Switch to screen-space (no world camera)
    Camera2D* prevCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    // Save previous shader and force default (2D) shader to avoid attribute mismatch
    Shader* prevShader = renderer.GetShader();
    Shader* defaultShader = renderer.GetDefaultShader();
    renderer.SetShader(defaultShader);

    // Reset sprite animation uniforms to default for GUI rendering
    if (defaultShader) {
        defaultShader->Use();
        defaultShader->SetInt("u_Frame", 0);
        defaultShader->SetInt("u_Cols", 1);
        defaultShader->SetVec2("u_FrameSize", Vector2(1.0f, 1.0f));
    }

    // Disable depth test to ensure UI draws on top
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);

    // Enable blending for UI transparency
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    if (!blendEnabled) glEnable(GL_BLEND);
    GLint srcAlpha, dstAlpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw background
    if (bgTex != 0) {
        // Full screen [-1, 1] quad (Utility::BuildQuad already creates it in range [-1, 1])
        Matrix3x3 model; // Identity is sufficient for [-1, 1]
        renderer.DrawMesh(bgMesh, model, Vector3(1.f, 1.f, 1.f), bgTex);
    }

    int ww = 1280, wh = 720;
    if (GLFWwindow* win = glfwGetCurrentContext()) {
        glfwGetFramebufferSize(win, &ww, &wh);
    }

    // Update element positions based on current window size
    UpdateLayoutPositions(gui, ww, wh);

    {
        unsigned int guideTex = ResourceManager::GetTexture(layout.guideTexture);
        if (guideTex != 0 && ww > 0 && wh > 0) {
            float panelWpx = ww * layout.guideRelSize.x;
            float panelHpx = wh * layout.guideRelSize.y;
            float xpx = ww * layout.guideRelPos.x;
            float ypx = wh * layout.guideRelPos.y;

            float W = (panelWpx / ww) * 2.0f;
            float H = (panelHpx / wh) * 2.0f;
            float lbx = (xpx / ww) * 2.0f - 1.0f;
            float lby = (ypx / wh) * 2.0f - 1.0f;
            float cx = lbx + W * 0.5f;
            float cy = lby + H * 0.5f;

            Matrix3x3 model =
                Matrix3x3::BuildTranslation(cx, cy) *
                Matrix3x3::BuildScaling(W * 0.5f, H * 0.5f);

            renderer.DrawMesh(bgMesh, model, Vector3(1.f, 1.f, 1.f), guideTex);
        }
    }

    // Debug: Check elements count
    DebugConsole::Get().Info("[ControlMenu] Drawing GUI: " + 
        std::to_string(gui.GetButtons().size()) + " buttons, " + 
        std::to_string(gui.GetTexts().size()) + " texts, " + 
        std::to_string(gui.GetPanels().size()) + " panels\n");

    // Draw GUI buttons and text
    gui.Draw(renderer);

    // Restore blending state
    if (!blendEnabled) glDisable(GL_BLEND);
    glBlendFunc(srcAlpha, dstAlpha);

    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }

    renderer.SetShader(prevShader);
    renderer.setCamera(prevCam);
}
