/**
 * @file       loseMenu.cpp
 * @author     Jethro Sung
 * @co-author  Woh Kye Le
 * @email      sung.h, w.kyele
 * @date       2026-02-05
 *
 * @brief     Implements LoseMenu, a screen-space UI that is displayed when the
 *            player loses (e.g., HP reaches 0).
 *
 * LoseMenu is responsible for building and rendering the "Lose" screen UI:
 *  - A full-screen background image (bgMesh + bgTex).
 *  - A centered title text label.
 *  - Three buttons: Retry, Main Menu, and Quit.
 *
 * The menu uses GuiSystem for text/button widgets, while the background image
 * is drawn directly through Renderer in NDC space so it always covers the
 * entire screen regardless of world camera settings.
 *
 * Layout is computed using the current window size:
 *  - Title is horizontally centered by measuring text width.
 *  - Buttons are placed using relative coordinates stored in layout.
 *
 * Callbacks for each button are injected during Build(), allowing GameApp /
 * SceneManager to define the behavior for retrying, returning to main menu,
 * or quitting the game.
 *
 * @version 1.0
 * @copyright Copyright (C) 2026
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */

#include "UI/loseMenu.h"

#include "Graphics/renderer.h"
#include "Graphics/camera2d.h"
#include "Graphics/vertex2d.h"

#include "Core/assetsPath.h"
#include "Core/resourceManager.h"
#include "Math/vect3.h"
#include "Math/matrix3x3.h"

#include "Font/FontRenderer.h"

#include <GLFW/glfw3.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <json.hpp>

using json = nlohmann::json;

static GuiText* FindTextByIdFallback(GuiSystem& gui, const std::string& id)
{
    auto& texts = gui.GetTexts();
    for (auto& t : texts)
        if (t.id == id) return &t;
    return nullptr;
}

void LoseMenu::Build(
    GuiSystem& gui,
    std::function<void()> onRetry,
    std::function<void()> onMainMenu,
    std::function<void()> onQuit)
{
    //Background quad in [0,1]x[0,1] space
    {
        Vector3 white(1.f, 1.f, 1.f);
        std::vector<Vertex2D> verts = {
            Vertex2D(Vector2(0.f, 0.f), Vector2(0.f, 0.f), white),
            Vertex2D(Vector2(1.f, 0.f), Vector2(1.f, 0.f), white),
            Vertex2D(Vector2(1.f, 1.f), Vector2(1.f, 1.f), white),
            Vertex2D(Vector2(0.f, 1.f), Vector2(0.f, 1.f), white)
        };
        std::vector<unsigned int> inds = { 0,1,2, 0,2,3 };

        bgMesh.Reserve(verts.size(), inds.size());
        bgMesh.SetVertices(verts);
        bgMesh.SetIndices(inds);
    }

    // textures will be resolved in ReloadTextures()
    bgTex = 0;

    // Register title text
    gui.AddText(GuiText(Vector2(0,0), layout.titleText, layout.titleSize, layout.titleColor, layout.titleFont));
    titleIndex = (int)gui.GetTexts().size() - 1;
    gui.GetTexts()[titleIndex].id = layout.titleId;

    // Register buttons & cache indices
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);
    auto& btns = gui.GetButtons();

    retryIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0, 0), Vector2(0, 0), layout.retryText,
        [onRetry]() { if (onRetry) onRetry(); }, buttonTex));
    btns[retryIndex].id = "lose.retry";

    mainIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0, 0), Vector2(0, 0), layout.mainText,
        [onMainMenu]() { if (onMainMenu) onMainMenu(); }, buttonTex));
    btns[mainIndex].id = "lose.main";

    quitIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0, 0), Vector2(0, 0), layout.quitText,
        [onQuit]() { if (onQuit) onQuit(); }, buttonTex));
    btns[quitIndex].id = "lose.quit";
}

bool LoseMenu::LoadLayout(const std::string& path, GuiSystem& gui)
{
    const std::string fullPath = AssetPath(path);

    std::ifstream ifs(fullPath);
    if (!ifs.is_open())
        return false;

    json j;
    try { ifs >> j; }
    catch (...) { return false; }

    auto clamp01 = [](float v) { return std::clamp(v, 0.0f, 1.0f); };

    auto readVec2 = [&](const json& obj, const char* key, Vector2& out, bool clamp) {
        if (!obj.contains(key) || !obj[key].is_object()) return;
        const json& v = obj[key];
        if (v.contains("x") && v["x"].is_number()) out.x = v["x"].get<float>();
        if (v.contains("y") && v["y"].is_number()) out.y = v["y"].get<float>();
        if (clamp) { out.x = clamp01(out.x); out.y = clamp01(out.y); }
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
        if (obj.contains(key) && obj[key].is_number_unsigned())
            out = obj[key].get<unsigned>();
        else if (obj.contains(key) && obj[key].is_number_integer())
            out = static_cast<unsigned>(obj[key].get<int>());
        };

    // ---------- background ----------
    if (j.contains("background") && j["background"].is_object())
        readString(j["background"], "texture", layout.bgTexture);
    else
        readString(j, "bgTexture", layout.bgTexture);

    // ---------- title ----------
    if (j.contains("title") && j["title"].is_object())
    {
        const json& t = j["title"];
        readString(t, "id", layout.titleId);
        readString(t, "text", layout.titleText);
        readString(t, "font", layout.titleFont);
        readFloat(t, "size", layout.titleSize);
        readUint(t, "color", layout.titleColor);
        readFloat(t, "relY", layout.titleRelY);
        layout.titleRelY = clamp01(layout.titleRelY);

        // apply to existing GuiText if present
        GuiText* gt = FindTextByIdFallback(gui, layout.titleId);
        if (gt)
        {
            gt->id = layout.titleId;
            gt->text = layout.titleText;
            gt->font = layout.titleFont;
            gt->size = layout.titleSize;
            gt->baseSize = gt->size;
            gt->color = layout.titleColor;
        }
        else if (titleIndex >= 0 && titleIndex < (int)gui.GetTexts().size())
        {
            // fallback: update by cached index
            auto& tt = gui.GetTexts()[titleIndex];
            tt.id = layout.titleId;
            tt.text = layout.titleText;
            tt.font = layout.titleFont;
            tt.size = layout.titleSize;
            tt.baseSize = tt.size;
            tt.color = layout.titleColor;
        }
    }
    else
    {
        // legacy fallback
        readString(j, "titleId", layout.titleId);
        readString(j, "titleText", layout.titleText);
        readString(j, "titleFont", layout.titleFont);
        readFloat(j, "titleSize", layout.titleSize);
        readUint(j, "titleColor", layout.titleColor);
        readFloat(j, "titleRelY", layout.titleRelY);
        layout.titleRelY = clamp01(layout.titleRelY);
    }

    // ---------- buttons root ----------
    const json* buttonsRoot = nullptr;
    if (j.contains("buttons") && j["buttons"].is_object())
        buttonsRoot = &j["buttons"];

    // button texture
    if (buttonsRoot) readString(*buttonsRoot, "texture", layout.buttonTexture);
    else             readString(j, "buttonTexture", layout.buttonTexture);

    // base layout fields
    const json* layoutRoot = buttonsRoot ? buttonsRoot : &j;
    readVec2(*layoutRoot, "btnRelSize", layout.btnRelSize, true);
    readVec2(*layoutRoot, "retryRelPos", layout.retryRelPos, true);
    readVec2(*layoutRoot, "mainRelPos", layout.mainRelPos, true);
    readVec2(*layoutRoot, "quitRelPos", layout.quitRelPos, true);

    // editor-friendly items array
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

            if (it.contains("label") && it["label"].is_string())
                b->label = it["label"].get<std::string>();
            if (it.contains("font") && it["font"].is_string())
                b->font = it["font"].get<std::string>();
            if (it.contains("fontSize") && it["fontSize"].is_number()) {
                b->fontSize = it["fontSize"].get<float>();
                b->baseFontSize = b->fontSize;
            }
            if (it.contains("textColor"))
                readUint(it, "textColor", b->textColor);

            // relPos updates layout slots also
            if (it.contains("relPos") && it["relPos"].is_object())
            {
                Vector2 rp(0.f, 0.f);
                readVec2(it, "relPos", rp, true);

                if (id == "lose.retry") layout.retryRelPos = rp;
                else if (id == "lose.main") layout.mainRelPos = rp;
                else if (id == "lose.quit") layout.quitRelPos = rp;
            }
        }
    }
    else
    {
        if (buttonsRoot && buttonsRoot->contains("style") && (*buttonsRoot)["style"].is_object())
        {
            const json& style = (*buttonsRoot)["style"];
            auto applyStyle = [&](const char* keyId)
                {
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

            applyStyle("lose.retry");
            applyStyle("lose.main");
            applyStyle("lose.quit");
        }
    }

    ReloadTextures(gui);
    return true;
}

bool LoseMenu::SaveLayout(const std::string& path, const GuiSystem& gui) const
{
    const std::string fullPath = AssetPath(path);

    try {
        std::filesystem::path p(fullPath);
        if (p.has_parent_path())
            std::filesystem::create_directories(p.parent_path());
    }
    catch (...) {

    }

    json j;
    j["background"] = { {"texture", layout.bgTexture} };

    // Title block
    j["title"] = {
        {"id", layout.titleId},
        {"text", layout.titleText},
        {"font", layout.titleFont},
        {"size", layout.titleSize},
        {"color", layout.titleColor},
        {"relY", layout.titleRelY}
    };

    // Buttons block 
    json buttons;
    buttons["texture"] = layout.buttonTexture;
    buttons["btnRelSize"] = { {"x", layout.btnRelSize.x}, {"y", layout.btnRelSize.y} };

    auto makeItem = [&](const char* id, const Vector2& relPos) -> json {
        json it;
        it["id"] = id;
        it["relPos"] = { {"x", relPos.x}, {"y", relPos.y} };

        if (const GuiButton* b = gui.FindButton(id)) {
            it["label"] = b->label;
            it["font"] = b->font;
            it["fontSize"] = b->baseFontSize;
            it["textColor"] = b->textColor;
        }
        else {
            it["label"] = "";
            it["font"] = "default";
            it["fontSize"] = 20.0;
            it["textColor"] = 0xFFFFFFFF;
        }
        return it;
        };

    buttons["items"] = json::array({
        makeItem("lose.retry", layout.retryRelPos),
        makeItem("lose.main",  layout.mainRelPos),
        makeItem("lose.quit",  layout.quitRelPos)
        });

    // legacy keys (optional but useful)
    buttons["retryRelPos"] = { {"x", layout.retryRelPos.x}, {"y", layout.retryRelPos.y} };
    buttons["mainRelPos"] = { {"x", layout.mainRelPos.x},  {"y", layout.mainRelPos.y} };
    buttons["quitRelPos"] = { {"x", layout.quitRelPos.x},  {"y", layout.quitRelPos.y} };

    j["buttons"] = buttons;

    std::ofstream ofs(fullPath);
    if (!ofs.is_open())
        return false;

    ofs << j.dump(2);
    return true;
}

void LoseMenu::ReloadTextures(GuiSystem& gui)
{
    bgTex = ResourceManager::GetTexture(layout.bgTexture);
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);

    if (GuiButton* b = gui.FindButton("lose.retry")) b->texture = buttonTex;
    if (GuiButton* b = gui.FindButton("lose.main"))  b->texture = buttonTex;
    if (GuiButton* b = gui.FindButton("lose.quit"))  b->texture = buttonTex;
}


void LoseMenu::Draw(Renderer& renderer, GuiSystem& gui) const
{
    if (!visible) return;

    Camera2D* prevCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    GLFWwindow* win = glfwGetCurrentContext();
    int ww = 1280, wh = 720;
    if (win) glfwGetWindowSize(win, &ww, &wh);

    // background
    if (bgTex != 0)
    {
        Matrix3x3 model =
            Matrix3x3::BuildTranslation(-1.f, -1.f) *
            Matrix3x3::BuildScaling(2.f, 2.f);
        renderer.DrawMesh(bgMesh, model, Vector3(1.f, 1.f, 1.f), bgTex);
    }

    // title centered
    if (titleIndex >= 0 && titleIndex < (int)gui.GetTexts().size())
    {
        float scale = (float)wh / 1080.f;

        auto& t = gui.GetTexts()[titleIndex];
        t.text = layout.titleText;
        t.font = layout.titleFont;
        t.baseSize = layout.titleSize;
        t.color = layout.titleColor;

        float currentSize = t.baseSize * scale;
        float tw = EngineCore::FontRenderer::ComputeTextWidth(t.font, currentSize, t.text);
        float cx = ww * 0.5f;
        t.pos = Vector2(cx - tw * 0.5f, layout.titleRelY * wh);
    }

    // buttons
    auto& buttons = gui.GetButtons();
    const Vector2 btnSizePx(layout.btnRelSize.x * ww, layout.btnRelSize.y * wh);


    auto placeBtn = [&](int idx, const Vector2& relPos)
        {
            if (idx < 0 || idx >= (int)buttons.size()) return;
            buttons[idx].pos = Vector2(relPos.x * ww, relPos.y * wh);
            buttons[idx].size = btnSizePx;
        };

    placeBtn(retryIndex, layout.retryRelPos);
    placeBtn(mainIndex, layout.mainRelPos);
    placeBtn(quitIndex, layout.quitRelPos);

    gui.Draw(renderer);
    renderer.setCamera(prevCam);
}