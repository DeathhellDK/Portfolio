/**
 * @file      mainMenu.cpp
 * @author    Sng Swee Yong Dillon
 * @co-author Woh Kye Le
 * @email     sweeyongdillon.sng, w.kyele
 * @date      2025-11-29
 *
 * @brief implements the visual and interactive logic for the game�s main menu, providing a responsive, screen-space UI that adapts to any window resolution. 
 * The system constructs a full-screen textured background, dynamically positions Play, Settings, and Quit buttons using normalized coordinates, and delegates their 
 * callbacks to the gameplay state controller.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "UI/mainMenu.h"

#include "Graphics/renderer.h"  
#include "Graphics/camera2d.h"  
#include "Graphics/vertex2d.h"

#include "Core/assetsPath.h"
#include "Core/resourceManager.h"
#include "Math/vect3.h"
#include "Math/matrix3x3.h"

#include <GLFW/glfw3.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <json.hpp>

using json = nlohmann::json;

[[maybe_unused]] static bool IsGamepadConnected(int jid = GLFW_JOYSTICK_1)
{
    return glfwJoystickPresent(jid) && glfwJoystickIsGamepad(jid);
}

/**
    * @brief Builds the main menu background and GUI buttons.
    *
    * Creates a full-screen quad mesh in normalized [0,1] � [0,1] space for the
    * main menu background, fetches the background and shared button textures
    * from the ResourceManager, and computes normalized positions for the Play,
    * Settings, and Quit buttons. Each button is then added to the GuiSystem as
    * a "dummy" entry with an initial zero-size rect; their indices are cached
    * so that Draw() can later update their pixel positions and sizes per frame.
    *
    * @param gui        GUI system used to register buttons.
    * @param onPlay     Callback invoked when the Play button is clicked.
    * @param onSettings Callback invoked when the Settings button is clicked.
    * @param onQuit     Callback invoked when the Quit button is clicked.
*/
void MainMenu::Build(
    GuiSystem & gui,
    std::function<void()> onPlay,
    std::function<void()> onTutorial,
    std::function<void()> onSettings,
    std::function<void()> onCredits,
    std::function<void()> onQuit)
{
    // --- Background quad in [0,1]x[0,1] space ---
    {
        Vector3 white(1.f, 1.f, 1.f);
        std::vector<Vertex2D> verts = {
            Vertex2D(Vector2(0.f, 0.f), Vector2(0.f, 0.f), white), // bottom-left
            Vertex2D(Vector2(1.f, 0.f), Vector2(1.f, 0.f), white), // bottom-right
            Vertex2D(Vector2(1.f, 1.f), Vector2(1.f, 1.f), white), // top-right
            Vertex2D(Vector2(0.f, 1.f), Vector2(0.f, 1.f), white)  // top-left
        };
        std::vector<unsigned int> inds = { 0,1,2, 0,2,3 };

        bgMesh.Reserve(verts.size(), inds.size());
        bgMesh.SetVertices(verts);
        bgMesh.SetIndices(inds);
    }

    bgTex = ResourceManager::GetTexture(layout.bgTexture);
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);

    // --- Register buttons and cache indices ---
    auto& btns = gui.GetButtons();

    playIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0,0), Vector2(0,0), "Play", [onPlay]() { if (onPlay) onPlay(); }, buttonTex));
    btns[playIndex].id = "main.play";

    settingsIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0,0), Vector2(0,0), "Settings", [onSettings]() { if (onSettings) onSettings(); }, buttonTex));
    btns[settingsIndex].id = "main.settings";

    tutorialIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0,0), Vector2(0,0), "Tutorial", [onTutorial]() { if (onTutorial) onTutorial(); }, buttonTex));
    btns[tutorialIndex].id = "main.tutorial";

    creditsIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0, 0), Vector2(0, 0), "Credits", [onCredits]() { if (onCredits) onCredits(); }, buttonTex));
    btns[creditsIndex].id = "main.credits";

    quitIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0,0), Vector2(0,0), "Quit Game", [onQuit]() { if (onQuit) onQuit(); }, buttonTex));
    btns[quitIndex].id = "main.quit";
}

/**
 * @brief Loads main menu layout from a JSON configuration file.
 *
 * This function reads a JSON file containing the main menu's layout configuration,
 * including background texture, button positions, sizes, and optional styling.
 * It supports both legacy flat formats and newer editor-friendly formats with
 * nested items arrays or style mappings.
 *
 * @param path Path to the JSON layout file (relative to asset directory).
 * @param gui Reference to the GUI system for applying button styles.
 * @return bool True if the layout was successfully loaded, false otherwise.
 */
bool MainMenu::LoadLayout(const std::string& path, GuiSystem& gui){
    const std::string fullPath = AssetPath(path);

    std::ifstream ifs(fullPath);
    if (!ifs.is_open())
        return false;

    json j;
    try {
        ifs >> j;
    }
    catch (...) {
        return false;
    }

    auto clamp01 = [](float v) { return std::clamp(v, 0.0f, 1.0f); };

    auto readVec2 = [&](const json& obj, const char* key, Vector2& out, bool clamp) {
        if (!obj.contains(key) || !obj[key].is_object()) return;
        const json& v = obj[key];
        if (v.contains("x") && v["x"].is_number()) out.x = v["x"].get<float>();
        if (v.contains("y") && v["y"].is_number()) out.y = v["y"].get<float>();
        if (clamp) {
            out.x = clamp01(out.x);
            out.y = clamp01(out.y);
        }
        };

    auto readString = [&](const json& obj, const char* key, std::string& out) {
        if (obj.contains(key) && obj[key].is_string())
            out = obj[key].get<std::string>();
        };

    auto readFloat = [&](const json& obj, const char* key, float& out) {
        if (obj.contains(key) && obj[key].is_number())
            out = obj[key].get<float>();
        };

    auto readUint = [&](const json& obj, const char* key, unsigned& out) {
        // allow either unsigned int or signed int in json
        if (obj.contains(key) && obj[key].is_number_unsigned())
            out = obj[key].get<unsigned>();
        else if (obj.contains(key) && obj[key].is_number_integer())
            out = static_cast<unsigned>(obj[key].get<int>());
        };

    // --- background texture ---
    if (j.contains("background") && j["background"].is_object())
        readString(j["background"], "texture", layout.bgTexture);
    else
        readString(j, "bgTexture", layout.bgTexture);

    // --- buttons root ---
    const json* buttonsRoot = nullptr;
    if (j.contains("buttons") && j["buttons"].is_object())
        buttonsRoot = &j["buttons"];

    // --- button texture ---
    if (buttonsRoot)
        readString(*buttonsRoot, "texture", layout.buttonTexture);
    else
        readString(j, "buttonTexture", layout.buttonTexture);

    // --- base layout fields (support both nested and flat formats) ---
    // Prefer j["buttons"] if present, otherwise top-level
    const json* layoutRoot = buttonsRoot ? buttonsRoot : &j;

    readVec2(*layoutRoot, "btnRelSize", layout.btnRelSize, true);
    readVec2(*layoutRoot, "playRelPos", layout.playRelPos, true);
    readVec2(*layoutRoot, "tutorialRelPos", layout.tutorialRelPos, true);
    readVec2(*layoutRoot, "settingsRelPos", layout.settingsRelPos, true);
    readVec2(*layoutRoot, "creditsRelPos", layout.creditsRelPos, true);
    readVec2(*layoutRoot, "quitRelPos", layout.quitRelPos, true);

    // --- OPTIONAL: editor-friendly items array ---
    // Format:
    // "buttons": { "items": [ { "id": "...", "label": "...", "relPos":{x,y}, "font":"", "fontSize":, "textColor": } ] }
    if (buttonsRoot && buttonsRoot->contains("items") && (*buttonsRoot)["items"].is_array())
    {
        const json& items = (*buttonsRoot)["items"];

        for (const auto& it : items)
        {
            if (!it.is_object()) continue;
            if (!it.contains("id") || !it["id"].is_string()) continue;

            const std::string id = it["id"].get<std::string>();
            GuiButton* b = gui.FindButton(id);
            if (!b) continue;

            // label
            if (it.contains("label") && it["label"].is_string())
                b->label = it["label"].get<std::string>();

            // text style
            if (it.contains("font") && it["font"].is_string())
                b->font = it["font"].get<std::string>();

            if (it.contains("fontSize") && it["fontSize"].is_number()) {
                b->fontSize = it["fontSize"].get<float>();
                b->baseFontSize = b->fontSize;
            }

            if (it.contains("textColor"))
                readUint(it, "textColor", b->textColor);

            // relative position -> apply to your known layout slots too (optional but helpful)
            if (it.contains("relPos") && it["relPos"].is_object())
            {
                Vector2 rp(0.f, 0.f);
                readVec2(it, "relPos", rp, true);

                if (id == "main.play")        layout.playRelPos = rp;
                else if (id == "main.tutorial") layout.tutorialRelPos = rp;
                else if (id == "main.settings") layout.settingsRelPos = rp;
                else if (id == "main.credits") layout.creditsRelPos = rp;
                else if (id == "main.quit")     layout.quitRelPos = rp;
            }
        }
    }
    else
    {
        // --- No items array: still allow per-button style via a simple mapping object ---
        // Example:
        // "buttons": { "style": { "main.play": { ... }, ... } }
        if (buttonsRoot && buttonsRoot->contains("style") && (*buttonsRoot)["style"].is_object())
        {
            const json& style = (*buttonsRoot)["style"];
            auto applyStyle = [&](const char* keyId) {
                if (!style.contains(keyId) || !style[keyId].is_object()) return;
                GuiButton* b = gui.FindButton(keyId);
                if (!b) return;

                const json& s = style[keyId];

                if (s.contains("label") && s["label"].is_string()) b->label = s["label"].get<std::string>();
                if (s.contains("font") && s["font"].is_string())   b->font = s["font"].get<std::string>();
                if (s.contains("fontSize") && s["fontSize"].is_number()) {
                    b->fontSize = s["fontSize"].get<float>();
                    b->baseFontSize = b->fontSize;
                }
                if (s.contains("textColor")) readUint(s, "textColor", b->textColor);
                };

            applyStyle("main.play");
            applyStyle("main.tutorial");
            applyStyle("main.settings");
            applyStyle("main.credits");
            applyStyle("main.quit");
        }
    }

    return true;
}

/**
 * @brief Saves the current main menu layout to a JSON configuration file.
 *
 * This function writes the current layout configuration (background texture,
 * button positions, sizes, and styles) to a JSON file. It saves in an
 * editor-friendly "items" format for easy editing while maintaining backward
 * compatibility with legacy flat formats.
 *
 * @param path Path to save the JSON layout file (relative to asset directory).
 * @param gui Reference to the GUI system for reading current button styles.
 * @return bool True if the layout was successfully saved, false otherwise.
 */
bool MainMenu::SaveLayout(const std::string& path, const GuiSystem& gui) const{
    const std::string fullPath = AssetPath(path);

    try {
        std::filesystem::path p(fullPath);
        if (p.has_parent_path())
            std::filesystem::create_directories(p.parent_path());
    }
    catch (...) {
        // directory creation failure is non-fatal
    }

    json j;
    j["background"] = { { "texture", layout.bgTexture } };

    // Save using editor-friendly "items" format
    json buttons;
    buttons["texture"] = layout.buttonTexture;
    buttons["btnRelSize"] = { { "x", layout.btnRelSize.x }, { "y", layout.btnRelSize.y } };

    auto makeItem = [&](const char* id, const Vector2& relPos) -> json {
        json it;
        it["id"] = id;
        it["relPos"] = { { "x", relPos.x }, { "y", relPos.y } };

        if (const GuiButton* b = gui.FindButton(id)) {
            it["label"] = b->label;
            it["font"] = b->font;
            it["fontSize"] = b->fontSize;
            it["textColor"] = b->textColor;
        }
        else {
            // fallback if not found
            it["label"] = "";
            it["font"] = "default";
            it["fontSize"] = 20.0;
            it["textColor"] = 0xFFFFFFFF;
        }

        return it;
        };

    buttons["items"] = json::array({
        makeItem("main.play", layout.playRelPos),
        makeItem("main.tutorial", layout.tutorialRelPos),
        makeItem("main.settings", layout.settingsRelPos),
        makeItem("main.credits", layout.creditsRelPos),
        makeItem("main.quit", layout.quitRelPos)
        });

    // Also keep legacy keys for backwards compatibility (optional but useful)
    buttons["playRelPos"] = { { "x", layout.playRelPos.x },     { "y", layout.playRelPos.y } };
    buttons["tutorialRelPos"] = { { "x", layout.tutorialRelPos.x }, { "y", layout.tutorialRelPos.y } };
    buttons["settingsRelPos"] = { { "x", layout.settingsRelPos.x }, { "y", layout.settingsRelPos.y } };
    buttons["creditsRelPos"] = { { "x", layout.creditsRelPos.x }, { "y", layout.creditsRelPos.y } };
    buttons["quitRelPos"] = { { "x", layout.quitRelPos.x },     { "y", layout.quitRelPos.y } };

    j["buttons"] = buttons;

    std::ofstream ofs(fullPath);
    if (!ofs.is_open())
        return false;

    ofs << j.dump(2);
    return true;
}

/**
 * @brief Reloads textures for the main menu from the ResourceManager.
 *
 * This function refreshes the main menu's background and button textures
 * by fetching the latest versions from the ResourceManager. Useful when
 * textures have been modified or reloaded during runtime.
 *
 * @param gui Reference to the GUI system to update button textures.
 */
void MainMenu::ReloadTextures(GuiSystem& gui){
    bgTex = ResourceManager::GetTexture(layout.bgTexture);
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);

    if (GuiButton* b = gui.FindButton("main.play"))     b->texture = buttonTex;
    if (GuiButton* b = gui.FindButton("main.tutorial")) b->texture = buttonTex;
    if (GuiButton* b = gui.FindButton("main.settings")) b->texture = buttonTex;
    if (GuiButton* b = gui.FindButton("main.credits"))  b->texture = buttonTex;
    if (GuiButton* b = gui.FindButton("main.quit"))     b->texture = buttonTex;
}



/**
    * @brief Renders the main menu in screen space and lays out its buttons.
    *
    * When the menu is visible, this function switches the renderer to a
    * null camera (pure screen-space), draws the full-screen textured quad
    * using the current window size, and converts the stored normalized
    * button positions/sizes into pixel-space rectangles. It then updates
    * the corresponding GuiSystem buttons and delegates the actual button
    * rendering to gui.Draw(). Finally, it restores the previous camera so
    * subsequent game rendering is unaffected.
    *
    * @param renderer Renderer used to draw the background quad.
    * @param gui      GUI system containing the main menu buttons.
*/
void MainMenu::Draw(Renderer& renderer, GuiSystem& gui) const {
    if (!visible) return;

    // Switch to screen-space (no world camera)
    Camera2D* prevCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    // Get current window size
    GLFWwindow* win = glfwGetCurrentContext();
    int ww = 1280, wh = 720;
    if (win) {
        glfwGetWindowSize(win, &ww, &wh);
    }

    // draw the text
    if (bgTex != 0) {
        // Fullscreen quad: pixel rect = whole window
        float ndcX = -1.f;
        float ndcY = -1.f;
        float ndcW = 2.f;
        float ndcH = 2.f;

        Matrix3x3 model =
            Matrix3x3::BuildTranslation(ndcX, ndcY) *
            Matrix3x3::BuildScaling(ndcW, ndcH);


        renderer.DrawMesh(bgMesh, model, Vector3(1.f, 1.f, 1.f), bgTex);
    }

    auto& buttons = gui.GetButtons();

    // Convert relative size -> pixel size
    const Vector2 btnSizePx(
        layout.btnRelSize.x * ww,
        layout.btnRelSize.y * wh
    );

    // center-half offset 
    float centerXOffset = 0.0f;

    // PLAY
    if (playIndex >= 0 && playIndex < static_cast<int>(buttons.size())) {
        buttons[playIndex].pos = Vector2(
            (layout.playRelPos.x * ww + centerXOffset) - (btnSizePx.x * 0.5f),
            layout.playRelPos.y * wh
        );
        buttons[playIndex].size = btnSizePx;
    }

    // TUTORIAL
    if (tutorialIndex >= 0 && tutorialIndex < static_cast<int>(buttons.size())) {
        buttons[tutorialIndex].pos = Vector2(
            (layout.tutorialRelPos.x * ww + centerXOffset) - (btnSizePx.x * 0.5f),
            layout.tutorialRelPos.y * wh
        );
        buttons[tutorialIndex].size = btnSizePx;
    }

    // SETTINGS
    if (settingsIndex >= 0 && settingsIndex < static_cast<int>(buttons.size())) {
        buttons[settingsIndex].pos = Vector2(                                                                      
            (layout.settingsRelPos.x * ww + centerXOffset) - (btnSizePx.x * 0.5f),
            layout.settingsRelPos.y * wh
        );
        buttons[settingsIndex].size = btnSizePx;
    }

    // CREDITS 
    if (creditsIndex >= 0 && creditsIndex < static_cast<int>(buttons.size())) {
        buttons[creditsIndex].pos = Vector2(
            (layout.creditsRelPos.x * ww + centerXOffset) - (btnSizePx.x * 0.5f),
            layout.creditsRelPos.y * wh
        );
        buttons[creditsIndex].size = btnSizePx;
    }

    // QUIT
    if (quitIndex >= 0 && quitIndex < static_cast<int>(buttons.size())) {
        buttons[quitIndex].pos = Vector2(
            (layout.quitRelPos.x * ww + centerXOffset) - (btnSizePx.x * 0.5f),
            layout.quitRelPos.y * wh
        );
        buttons[quitIndex].size = btnSizePx;
    }


    // Draw the menu buttons
    gui.Draw(renderer);

    // Restore previous camera
    renderer.setCamera(prevCam);
}
