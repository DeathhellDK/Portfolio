/**
 * @file      confirmDialog.cpp
 * @author    Sng Swee Yong Dillon
 * @email     sweeyongdillon.sng
 * @date      2026-01-10
 * @brief     Implements ConfirmDialog, a modal confirmation popup that
 *            overlays the game view and blocks normal UI interaction.
 *
 * The dialog is built from GuiSystem widgets (one text label and two buttons),
 * with a rendered screen-dim + centered panel drawn behind the widgets.
 * Layout and styling are data-driven via a JSON layout file:
 *  - Positions/sizes are expressed in relative coordinates (0..1).
 *  - Fonts, font sizes, colors, and textures are configurable.
 *  - Button labels can be overridden per layout entry.
 *
 * ConfirmDialog exposes Show()/Hide() for controlling visibility and stores
 * Yes/No callbacks to execute based on user input. ESC is treated as "No".
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "UI/confirmDialog.h"
#include "UI/guiSys.h"
#include "Input/input.h"
#include "Graphics/renderer.h"
#include "Core/utils.h"
#include "Core/assetsPath.h"
#include "Core/resourceManager.h"

#include <algorithm>
#include <fstream>
#include <filesystem>
#include <json.hpp>

using json = nlohmann::json;

// -------- helper function 
static float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

static void ReadVec2_Any(const json& obj, const char* key, Vector2& out, bool clamp01){
    if (!obj.contains(key)) return;
    const json& v = obj.at(key);

    if (v.is_object()) {
        if (v.contains("x") && v["x"].is_number()) out.x = v["x"].get<float>();
        if (v.contains("y") && v["y"].is_number()) out.y = v["y"].get<float>();
    }
    else if (v.is_array() && v.size() >= 2) {
        out.x = v[0].get<float>();
        out.y = v[1].get<float>();
    }

    if (clamp01) {
        out.x = Clamp01(out.x);
        out.y = Clamp01(out.y);
    }
}

static void ReadString(const json& obj, const char* key, std::string& out){
    if (obj.contains(key) && obj[key].is_string()) out = obj[key].get<std::string>();
}

static void ReadFloat(const json& obj, const char* key, float& out){
    if (obj.contains(key) && obj[key].is_number()) out = obj[key].get<float>();
}

static void ReadUint(const json& obj, const char* key, unsigned& out){
    if (!obj.contains(key) || !obj[key].is_number()) return;
    if (obj[key].is_number_unsigned()) out = obj[key].get<unsigned>();
    else out = static_cast<unsigned>(obj[key].get<int>());
}

static Vector3 ColorU32_ToRGB(unsigned c){
    // 0xAARRGGBB
    unsigned r = (c >> 16) & 0xFF;
    unsigned g = (c >> 8) & 0xFF;
    unsigned b = (c) & 0xFF;
    return Vector3(r / 255.f, g / 255.f, b / 255.f);
}


/**
 * @brief Builds the confirm dialog UI elements.
 *
 * Creates the background panel mesh (once) and registers the dialog
 * text and Yes/No buttons into the provided GuiSystem. All widgets
 * are created hidden by default and only shown when the dialog is active.
 *
 * @param gui GuiSystem that owns the dialog widgets.
 */
void ConfirmDialog::Build(GuiSystem& gui){

    // ---------- build panel quad once ----------
    if (!panelBuilt) {
        std::vector<Vector2> pos = {
            {0,0}, {1,0}, {1,1}, {0,1}
        };
        std::vector<Vector3> col = {
            {1,1,1}, {1,1,1}, {1,1,1}, {1,1,1}
        };
        panelQuad = Utility::BuildQuad(pos, col);
        panelBuilt = true;
    }

    // Create widgets if they don't exist (ID-based)
    if (!gui.FindText(layout.textId)) {
        GuiText t({ 0,0 }, "Confirm?", layout.messageFontSize);
        t.id = layout.textId;
        t.font = layout.messageFont;
        t.color = layout.messageColor;
        gui.AddText(t);
    }

    unsigned btnTex = ResourceManager::GetTexture(layout.buttonTexture);

    if (!gui.FindButton(layout.yesButtonId)) {
        GuiButton yes({ 0,0 }, { 0,0 }, "Yes", [this]() {
            if (onYes) onYes();
            Hide();
            }, btnTex);

        yes.id = layout.yesButtonId;
        yes.font = layout.buttonFont;
        yes.fontSize = layout.buttonFontSize;
        yes.textColor = layout.buttonTextColor;
        gui.AddButton(yes);
    }

    if (!gui.FindButton(layout.noButtonId)) {
        GuiButton no({ 0,0 }, { 0,0 }, "No", [this]() {
            if (onNo) onNo();
            Hide();
            }, btnTex);

        no.id = layout.noButtonId;
        no.font = layout.buttonFont;
        no.fontSize = layout.buttonFontSize;
        no.textColor = layout.buttonTextColor;
        gui.AddButton(no);
    }

    ReloadTextures(gui);
    SetWidgetsVisible(gui, false);
}

/**
 * @brief Displays the confirmation dialog with a message and callbacks.
 *
 * Activates the dialog, updates the displayed message, and stores the
 * callbacks to be executed when the user selects Yes or No.
 *
 * @param message Text displayed in the dialog.
 * @param onYes_  Callback executed when the user confirms.
 * @param onNo_   Callback executed when the user cancels.
 */
void ConfirmDialog::Show(const std::string& message,  std::function<void()> onYes_, std::function<void()> onNo_){
    visible = true;
    msg = message;
    onYes = std::move(onYes_);
    onNo = std::move(onNo_);
}

/**
 * @brief Hides the confirmation dialog.
 *
 * Disables visibility and clears stored callbacks to prevent
 * unintended execution after the dialog is dismissed.
 */
void ConfirmDialog::Hide(){
    visible = false;
    onYes = nullptr;
    onNo = nullptr;
}

bool ConfirmDialog::LoadLayout(const std::string& relPath, GuiSystem& gui){
    const std::string fullPath = AssetPath(relPath);
    std::ifstream ifs(fullPath);
    if (!ifs.is_open()) return false;

    json j;
    try { ifs >> j; }
    catch (...) { return false; }

    // ids
    if (j.contains("ids") && j["ids"].is_object()) {
        const json& ids = j["ids"];
        ReadString(ids, "text", layout.textId);
        ReadString(ids, "yes", layout.yesButtonId);
        ReadString(ids, "no", layout.noButtonId);
    }

    // panel
    if (j.contains("panel") && j["panel"].is_object()) {
        const json& p = j["panel"];
        ReadVec2_Any(p, "relSize", layout.panelRelSize, true);
        ReadUint(p, "color", layout.panelColor);
        ReadUint(p, "dimColor", layout.dimColor);
    }

    // message
    if (j.contains("message") && j["message"].is_object()) {
        const json& m = j["message"];
        ReadVec2_Any(m, "relPos", layout.messageRelPos, true);
        ReadString(m, "font", layout.messageFont);
        ReadFloat(m, "fontSize", layout.messageFontSize);
        ReadUint(m, "color", layout.messageColor);
    }

    // buttons
    if (j.contains("buttons") && j["buttons"].is_object()) {
        const json& b = j["buttons"];
        ReadVec2_Any(b, "relSize", layout.buttonRelSize, true);
        ReadString(b, "font", layout.buttonFont);
        ReadFloat(b, "fontSize", layout.buttonFontSize);
        ReadUint(b, "textColor", layout.buttonTextColor);
        ReadString(b, "texture", layout.buttonTexture);

        // Items allow editor to reposition and rename labels
        if (b.contains("items") && b["items"].is_array()) {
            for (const auto& it : b["items"]) {
                if (!it.is_object()) continue;
                if (!it.contains("id") || !it["id"].is_string()) continue;
                std::string id = it["id"].get<std::string>();

                Vector2 rp{ 0.f,0.f };
                ReadVec2_Any(it, "relPos", rp, true);

                if (id == layout.yesButtonId) layout.yesRelPos = rp;
                if (id == layout.noButtonId)  layout.noRelPos = rp;

                if (GuiButton* btn = gui.FindButton(id)) {
                    if (it.contains("label") && it["label"].is_string()) btn->label = it["label"].get<std::string>();
                    if (it.contains("font") && it["font"].is_string()) btn->font = it["font"].get<std::string>();
                    if (it.contains("fontSize") && it["fontSize"].is_number()) btn->fontSize = it["fontSize"].get<float>();
                    if (it.contains("textColor")) ReadUint(it, "textColor", btn->textColor);
                }
            }
        }
    }

    // Apply styles to existing widgets 
    if (GuiText* t = gui.FindText(layout.textId)) {
        t->font = layout.messageFont;
        t->size = layout.messageFontSize;
        t->color = layout.messageColor;
    }
    if (GuiButton* y = gui.FindButton(layout.yesButtonId)) {
        y->font = layout.buttonFont;
        y->fontSize = layout.buttonFontSize;
        y->textColor = layout.buttonTextColor;
    }
    if (GuiButton* n = gui.FindButton(layout.noButtonId)) {
        n->font = layout.buttonFont;
        n->fontSize = layout.buttonFontSize;
        n->textColor = layout.buttonTextColor;
    }

    ReloadTextures(gui);
    return true;
}

bool ConfirmDialog::SaveLayout(const std::string& relPath, const GuiSystem& gui) const{
    const std::string fullPath = AssetPath(relPath);

    try {
        std::filesystem::path p(fullPath);
        if (p.has_parent_path())
            std::filesystem::create_directories(p.parent_path());
    }
    catch (...) {}

    json j;

    // ids
    j["ids"] = {
        {"text", layout.textId},
        {"yes",  layout.yesButtonId},
        {"no",   layout.noButtonId}
    };

    // panel
    j["panel"] = {
        {"relSize", { {"x", layout.panelRelSize.x}, {"y", layout.panelRelSize.y} }},
        {"color", layout.panelColor},
        {"dimColor", layout.dimColor}
    };

    // message
    json msgj;
    msgj["relPos"] = { {"x", layout.messageRelPos.x}, {"y", layout.messageRelPos.y} };
    msgj["font"] = layout.messageFont;
    msgj["fontSize"] = layout.messageFontSize;
    msgj["color"] = layout.messageColor;
    j["message"] = msgj;

    // buttons
    json btn;
    btn["relSize"] = { {"x", layout.buttonRelSize.x}, {"y", layout.buttonRelSize.y} };
    btn["font"] = layout.buttonFont;
    btn["fontSize"] = layout.buttonFontSize;
    btn["textColor"] = layout.buttonTextColor;
    btn["texture"] = layout.buttonTexture;

    auto makeItem = [&](const char* id, const Vector2& rp, const char* fallbackLabel) -> json {
        json it;
        it["id"] = id;
        it["relPos"] = { {"x", rp.x}, {"y", rp.y} };

        if (const GuiButton* b = gui.FindButton(id)) {
            it["label"] = b->label;
            it["font"] = b->font;
            it["fontSize"] = b->fontSize;
            it["textColor"] = b->textColor;
        }
        else {
            it["label"] = fallbackLabel;
            it["font"] = layout.buttonFont;
            it["fontSize"] = layout.buttonFontSize;
            it["textColor"] = layout.buttonTextColor;
        }
        return it;
        };

    btn["items"] = json::array({
        makeItem(layout.yesButtonId.c_str(), layout.yesRelPos, "Yes"),
        makeItem(layout.noButtonId.c_str(),  layout.noRelPos,  "No")
        });

    j["buttons"] = btn;

    std::ofstream ofs(fullPath);
    if (!ofs.is_open()) return false;

    ofs << j.dump(2);
    return true;
}

void ConfirmDialog::ReloadTextures(GuiSystem& gui){
    unsigned tex = ResourceManager::GetTexture(layout.buttonTexture);

    if (GuiButton* y = gui.FindButton(layout.yesButtonId)) y->texture = tex;
    if (GuiButton* n = gui.FindButton(layout.noButtonId))  n->texture = tex;
}

/**
 * @brief Hides the confirmation dialog.
 *
 * Disables visibility and clears stored callbacks to prevent
 * unintended execution after the dialog is dismissed.
 */
void ConfirmDialog::SetWidgetsVisible(GuiSystem& gui, bool v) const{
    if (GuiText* t = gui.FindText(layout.textId)) t->visible = v;
    if (GuiButton* b = gui.FindButton(layout.yesButtonId)) b->visible = v;
    if (GuiButton* b = gui.FindButton(layout.noButtonId)) b->visible = v;
}

/**
 * @brief Computes and applies layout for the dialog elements.
 *
 * Positions the dialog text and buttons within a centered panel
 * based on the current window dimensions.
 *
 * @param gui GuiSystem containing the dialog widgets.
 * @param ww  Window width in pixels.
 * @param wh  Window height in pixels.
 */
void ConfirmDialog::Layout(GuiSystem& gui, int ww, int wh) const{
    float pw = ww * layout.panelRelSize.x;
    float ph = wh * layout.panelRelSize.y;
    float px = (ww - pw) * 0.5f;
    float py = (wh - ph) * 0.5f;

    // message
    if (GuiText* t = gui.FindText(layout.textId)) {
        t->pos = {
            px + pw * layout.messageRelPos.x,
            py + ph * layout.messageRelPos.y
        };
    }

    float bw = pw * layout.buttonRelSize.x;
    float bh = ph * layout.buttonRelSize.y;

    // buttons
    if (GuiButton* b = gui.FindButton(layout.yesButtonId)) {
        b->pos = { px + pw * layout.yesRelPos.x, py + ph * layout.yesRelPos.y };
        b->size = { bw, bh };
    }

    if (GuiButton* b = gui.FindButton(layout.noButtonId)) {
        b->pos = { px + pw * layout.noRelPos.x, py + ph * layout.noRelPos.y };
        b->size = { bw, bh };
    }
}

/**
 * @brief Computes and applies layout for the dialog elements.
 *
 * Positions the dialog text and buttons within a centered panel
 * based on the current window dimensions.
 *
 * @param gui GuiSystem containing the dialog widgets.
 * @param ww  Window width in pixels.
 * @param wh  Window height in pixels.
 */
void ConfirmDialog::Update(const eng::Input& in, double dt, GuiSystem& gui, Vector2 viewportPos, Vector2 viewportSize){
    if (!visible) return;

    if (in.isKeyPressed(GLFW_KEY_ESCAPE)) {
        if (onNo) onNo();
        Hide();
        return;
    }
    // update dialog widgets
    gui.Update(in, dt, viewportPos, viewportSize);
}

/**
 * @brief Renders the confirmation dialog overlay.
 *
 * Draws a centered background panel using screen-space projection,
 * updates widget layout, and renders the dialog text and buttons
 * on top of the existing scene.
 *
 * @param renderer Renderer used for drawing.
 * @param gui      GuiSystem containing the dialog widgets.
 * @param ww       Window width in pixels.
 * @param wh       Window height in pixels.
 */
void ConfirmDialog::Draw(Renderer& renderer, GuiSystem& gui, int ww, int wh) const{
    if (!visible) return;

    auto* oldCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    Matrix3x3 ui = Matrix3x3::Ortho(0.0f, (float)ww, (float)wh, 0.0f);

    const float oldAlpha = renderer.GetGlobalAlpha();

    // full-screen dim
    const unsigned dim = layout.dimColor;
    const float dimA = ((dim >> 24) & 0xFF) / 255.0f;

    if (dimA > 0.0f) {
        renderer.SetGlobalAlpha(dimA);

        Matrix3x3 dimModel =
            Matrix3x3::BuildTranslation(0.f, 0.f) *
            Matrix3x3::BuildScaling((float)ww, (float)wh);

        renderer.DrawMesh(panelQuad, ui * dimModel, ColorU32_ToRGB(dim), 0);
    }

    const unsigned panel = layout.panelColor;
    const float panelA = ((panel >> 24) & 0xFF) / 255.0f;
    renderer.SetGlobalAlpha(panelA);

    // Panel rectangle
    float pw = ww * layout.panelRelSize.x;
    float ph = wh * layout.panelRelSize.y;
    float px = (ww - pw) * 0.5f;
    float py = (wh - ph) * 0.5f;

    Matrix3x3 panelModel =
        Matrix3x3::BuildTranslation(px, py) *
        Matrix3x3::BuildScaling(pw, ph);

    renderer.DrawMesh(panelQuad, ui * panelModel, ColorU32_ToRGB(panel), 0);

    // Update text content + layout
    if (GuiText* t = gui.FindText(layout.textId)) {
        t->text = msg;
        t->font = layout.messageFont;
        t->size = layout.messageFontSize;
        t->color = layout.messageColor;
    }

    // Make visible & place
    SetWidgetsVisible(gui, true);
    Layout(gui, ww, wh);

    renderer.SetGlobalAlpha(1.0f);

    gui.Draw(renderer);

    renderer.SetGlobalAlpha(oldAlpha);
    renderer.setCamera(oldCam);
}
