/**
 * @file      creditsMenu.cpp
 * @author    Jethro Sung
 * @email     sung.h
 * @date      2026-04-4
 * @brief     Implements the CreditMenu class functionality.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "UI/creditsMenu.h"

#include "Core/utils.h"
#include "Core/resourceManager.h"
#include "Core/assetsPath.h"
#include "Graphics/renderer.h"
#include "Graphics/camera2d.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

void CreditsMenu::Build(GuiSystem& gui, std::function<void()> onExit) {
    onExitCallback = std::move(onExit);

    bgMesh = Utility::BuildQuad(
        { {-1,-1}, {1,-1}, {1,1}, {-1,1} },
        { {1,1,1}, {1,1,1}, {1,1,1}, {1,1,1} }
    );

    bgTex = ResourceManager::GetTexture(layout.bgTexture);
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);

    auto& btns = gui.GetButtons();
    backIndex = (int)btns.size();
    gui.AddButton(GuiButton(
        layout.backRelPos,
        layout.btnRelSize,
        layout.backText,
        [this]() { if (onExitCallback) onExitCallback(); },
        buttonTex
    ));
    if (backIndex >= 0 && backIndex < (int)btns.size()) {
        btns[backIndex].id = "credits.back";
    }

    BuildCreditsDisplay(gui);

    int ww = 1280, wh = 720;
    if (GLFWwindow* win = glfwGetCurrentContext()) {
        glfwGetFramebufferSize(win, &ww, &wh);
    }
    UpdateLayoutPositions(gui, ww, wh);
}

void CreditsMenu::BuildCreditsDisplay(GuiSystem& gui) {
    GuiText title(Vector2(0, 0), layout.titleText, layout.titleFontSize, layout.titleColor, layout.font);
    title.id = "credits.title";
    gui.AddText(title);

    for (size_t i = 0; i < layout.lines.size(); i++) {
        const CreditsLine& ln = layout.lines[i];
        GuiText t(Vector2(0, 0), ln.text, ln.fontSize, ln.color, layout.font);
        t.id = "credits.line." + std::to_string(i);
        gui.AddText(t);
    }
}

void CreditsMenu::UpdateLayoutPositions(GuiSystem& gui, int ww, int wh) {
    if (GuiButton* b = gui.FindButton("credits.back")) {
        float x = layout.backRelPos.x * ww;
        float y = layout.backRelPos.y * wh;
        float w = layout.btnRelSize.x * ww;
        float h = layout.btnRelSize.y * wh;
        b->pos = Vector2(x - w * 0.5f, y - h * 0.5f);
        b->size = Vector2(w, h);
    }

    if (GuiText* t = gui.FindText("credits.title")) {
        t->pos = Vector2(layout.titleRelPos.x * ww, layout.titleRelPos.y * wh);
    }

    for (size_t i = 0; i < layout.lines.size(); i++) {
        if (GuiText* t = gui.FindText("credits.line." + std::to_string(i))) {
            t->pos = Vector2(layout.lines[i].relPos.x * ww, layout.lines[i].relPos.y * wh);
            t->size = layout.lines[i].fontSize;
            t->baseSize = layout.lines[i].fontSize;
            t->color = layout.lines[i].color;
            t->font = layout.font;
        }
    }
}

bool CreditsMenu::LoadLayout(const std::string& path, GuiSystem& gui) {
    (void)gui;
    const std::string fullPath = AssetPath(path);
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        return false;
    }

    try {
        json j;
        file >> j;

        if (j.contains("btnRelSize") && j["btnRelSize"].is_array()) {
            layout.btnRelSize.x = j["btnRelSize"][0];
            layout.btnRelSize.y = j["btnRelSize"][1];
        }
        if (j.contains("backRelPos") && j["backRelPos"].is_array()) {
            layout.backRelPos.x = j["backRelPos"][0];
            layout.backRelPos.y = j["backRelPos"][1];
        }
        if (j.contains("titleRelPos") && j["titleRelPos"].is_array()) {
            layout.titleRelPos.x = j["titleRelPos"][0];
            layout.titleRelPos.y = j["titleRelPos"][1];
        }
        if (j.contains("bgTexture") && j["bgTexture"].is_string())
            layout.bgTexture = j["bgTexture"];
        if (j.contains("buttonTexture") && j["buttonTexture"].is_string())
            layout.buttonTexture = j["buttonTexture"];
        if (j.contains("backText") && j["backText"].is_string())
            layout.backText = j["backText"];
        if (j.contains("titleText") && j["titleText"].is_string())
            layout.titleText = j["titleText"];
        if (j.contains("font") && j["font"].is_string())
            layout.font = j["font"];
        if (j.contains("titleFontSize") && j["titleFontSize"].is_number())
            layout.titleFontSize = j["titleFontSize"];
        if (j.contains("titleColor") && j["titleColor"].is_number())
            layout.titleColor = j["titleColor"];

        if (j.contains("lines") && j["lines"].is_array()) {
            layout.lines.clear();
            for (const auto& item : j["lines"]) {
                if (!item.is_object()) continue;
                if (!item.contains("text") || !item["text"].is_string()) continue;
                CreditsLine ln;
                ln.text = item["text"].get<std::string>();
                if (item.contains("pos") && item["pos"].is_array()) {
                    ln.relPos.x = item["pos"][0].get<float>();
                    ln.relPos.y = item["pos"][1].get<float>();
                }
                if (item.contains("size") && item["size"].is_number())
                    ln.fontSize = item["size"].get<float>();
                if (item.contains("color") && item["color"].is_number())
                    ln.color = item["color"].get<unsigned>();
                layout.lines.push_back(std::move(ln));
            }
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

bool CreditsMenu::SaveLayout(const std::string& path, const GuiSystem& gui) const {
    (void)gui;
    json j;
    j["btnRelSize"] = { layout.btnRelSize.x, layout.btnRelSize.y };
    j["backRelPos"] = { layout.backRelPos.x, layout.backRelPos.y };
    j["titleRelPos"] = { layout.titleRelPos.x, layout.titleRelPos.y };
    j["bgTexture"] = layout.bgTexture;
    j["buttonTexture"] = layout.buttonTexture;
    j["backText"] = layout.backText;
    j["titleText"] = layout.titleText;
    j["font"] = layout.font;
    j["titleFontSize"] = layout.titleFontSize;
    j["titleColor"] = layout.titleColor;

    json lines = json::array();
    for (const auto& ln : layout.lines) {
        json it;
        it["text"] = ln.text;
        it["pos"] = { ln.relPos.x, ln.relPos.y };
        it["size"] = ln.fontSize;
        it["color"] = ln.color;
        lines.push_back(std::move(it));
    }
    j["lines"] = std::move(lines);

    const std::string fullPath = AssetPath(path);
    std::ofstream file(fullPath);
    if (!file.is_open()) return false;
    file << j.dump(4);
    return true;
}

void CreditsMenu::ReloadTextures(GuiSystem& gui) {
    (void)gui;
    bgTex = ResourceManager::GetTexture(layout.bgTexture);
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);
    if (GuiButton* b = gui.FindButton("credits.back")) {
        b->texture = buttonTex;
    }
}

void CreditsMenu::Update(const eng::Input& in) {
    if (!visible) return;
    if (in.isKeyPressed(GLFW_KEY_ESCAPE)) {
        if (onExitCallback) onExitCallback();
    }
}

void CreditsMenu::Draw(Renderer& renderer, GuiSystem& gui) {
    if (!visible) return;

    Camera2D* prevCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    Shader* prevShader = renderer.GetShader();
    Shader* defaultShader = renderer.GetDefaultShader();
    renderer.SetShader(defaultShader);

    if (defaultShader) {
        defaultShader->Use();
        defaultShader->SetInt("u_Frame", 0);
        defaultShader->SetInt("u_Cols", 1);
        defaultShader->SetVec2("u_FrameSize", Vector2(1.0f, 1.0f));
    }

    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);

    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    if (!blendEnabled) glEnable(GL_BLEND);
    GLint srcAlpha, dstAlpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (bgTex != 0) {
        Matrix3x3 model;
        renderer.DrawMesh(bgMesh, model, Vector3(1.f, 1.f, 1.f), bgTex);
    }

    int ww = 1280, wh = 720;
    if (GLFWwindow* win = glfwGetCurrentContext()) {
        glfwGetFramebufferSize(win, &ww, &wh);
    }
    UpdateLayoutPositions(gui, ww, wh);

    gui.Draw(renderer);

    if (!blendEnabled) glDisable(GL_BLEND);
    glBlendFunc(srcAlpha, dstAlpha);

    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }

    renderer.SetShader(prevShader);
    renderer.setCamera(prevCam);
}
