/**
 * @file      pauseMenu.cpp
 * @author    Lim Zhi Jie
 * @co-author Woh Kye Le
 * @email     zhijie.lim, w.kyele
 * @date      2025-11-29
 *
 * @brief implements the pause menu interface, combining a centered semi-transparent panel, an instructional slideshow, and standard Resume, Main Menu, and Quit actions. 
 * It builds reusable meshes for the panel and slideshow areas, dynamically sizes and positions UI components based on the current window resolution, and uses the engine’s 
 * FontRenderer for wrapped instructional captions. The slideshow supports multi-page navigation through left and right arrow buttons, while all menu buttons reactively reposition 
 * each frame to maintain clean alignment.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "UI/pauseMenu.h"
#include "Graphics/renderer.h"  
#include "Graphics/camera2d.h"   
#include "Graphics/mesh2d.h"
#include "Graphics/vertex2d.h"
#include "Core/resourceManager.h"
#include "Math/vect3.h"
#include "Math/matrix3x3.h"
#include "Core/utils.h"
#include "Core/assetsPath.h"
#include "Core/engine.hpp"
#include "Font/FontSystem.h"
#include "Font/FontRenderer.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <json.hpp>

using json = nlohmann::json;

/**
    * @brief Builds the pause menu UI, including "How To Play" slideshow and buttons.
    *
    * Registers a "HOW TO PLAY" title text in the GuiSystem, constructs a quad
    * mesh used to display slideshow textures, and populates the slides vector
    * with three instructional pages (texture + caption). It also adds left and
    * right arrow buttons that change the currentSlide index, and creates the
    * standard pause-menu buttons (Resume, Main Menu, Quit) with their callbacks.
    * Button positions are defined in normalized panel space and later converted
    * to pixel-space in Draw().
    *
    * @param gui        GUI system used to register texts and buttons.
    * @param onResume   Callback invoked when the Resume button is clicked.
    * @param onMainMenu Callback invoked when the Main Menu button is clicked.
    * @param onQuit     Callback invoked when the Quit button is clicked.
*/
void PauseMenu::Build(
    GuiSystem& gui,
    std::function<void()> onResume,
    std::function<void()> onMainMenu,
    std::function<void()> onQuit) {

    BuildTexts(gui);
    BuildSlideshowMesh();
    BuildSlides();
    BuildArrows(gui);
    BuildButtons(gui, onResume, onMainMenu, onQuit);
}

/**
 * @brief Loads the pause menu layout from a JSON configuration file.
 * Reads layout parameters from a JSON file at the specified path, applying
 * automatic clamping and validation where appropriate
 *
 * @param path Relative path to the JSON layout file.
 * @return true if the layout was successfully loaded (even with partial data).
 * @return false if the file could not be opened or contained invalid JSON.
 *
 *
 * @note The JSON structure is expected to have sections: "panel", "slideshow",
 * "buttons", "arrows", and "title". Missing sections are silently ignored.
 *
 */
bool PauseMenu::LoadLayout(const std::string& path, GuiSystem& gui)
{
    const std::string fullPath = AssetPath(path);

    std::ifstream ifs(fullPath);
    if (!ifs.is_open()) return false;

    json j;
    try { ifs >> j; }
    catch (...) { return false; }

    auto clamp01 = [](float v) { return std::clamp(v, 0.0f, 1.0f); };

    auto readFloat = [&](const json& obj, const char* key, float& out, bool clamp) {
        if (obj.contains(key) && obj[key].is_number()) {
            out = obj[key].get<float>();
            if (clamp) out = clamp01(out);
        }
        };

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

    auto readUint = [&](const json& obj, const char* key, unsigned& out) {
        if (obj.contains(key) && obj[key].is_number_unsigned())
            out = obj[key].get<unsigned>();
        else if (obj.contains(key) && obj[key].is_number_integer())
            out = (unsigned)obj[key].get<int>();
        };

    // ---- panel ----
    if (j.contains("panel") && j["panel"].is_object()) {
        readFloat(j["panel"], "wRel", layout.panelWRel, true);
        readFloat(j["panel"], "hRel", layout.panelHRel, true);
    }

    // ---- slideshow ----
    if (j.contains("slideshow") && j["slideshow"].is_object()) {
        const json& s = j["slideshow"];
        readFloat(s, "wRel", layout.slideWRel, true);
        readFloat(s, "hRel", layout.slideHRel, true);
        readFloat(s, "xRel", layout.slideXRel, true);
        readFloat(s, "yRel", layout.slideYRel, true);

        // slides array (optional)
        if (s.contains("slides") && s["slides"].is_array()) {
            layout.slides.clear();
            for (const auto& it : s["slides"]) {
                if (!it.is_object()) continue;
                PauseSlideDef def;

                readString(it, "texture", def.texture);
                readString(it, "caption", def.caption);
                readString(it, "captionFont", def.captionFont);
                if (it.contains("captionFontSizeRel") && it["captionFontSizeRel"].is_number())
                    def.captionFontSizeRel = it["captionFontSizeRel"].get<float>();
                readUint(it, "captionColor", def.captionColor);

                layout.slides.push_back(def);
            }
        }
    }

    // ---- title ----
    if (j.contains("title") && j["title"].is_object()) {
        const json& t = j["title"];
        readString(t, "id", layout.titleId);
        readString(t, "text", layout.titleText);
        readString(t, "font", layout.titleFont);
        readUint(t, "color", layout.titleColor);

        readFloat(t, "yRel", layout.titleYRel, true);
        readFloat(t, "size", layout.titleSize, false);
        layout.titleSize = std::max(1.0f, layout.titleSize);

        // apply immediately if text already exists
        auto& texts = gui.GetTexts();
        for (auto& tx : texts) {
            if (tx.id == layout.titleId) {
                tx.text = layout.titleText;
                tx.size = layout.titleSize;
                tx.font = layout.titleFont;
                tx.color = layout.titleColor;
                break;
            }
        }
    }

    // ---- buttons ----
    const json* buttonsRoot = nullptr;
    if (j.contains("buttons") && j["buttons"].is_object())
        buttonsRoot = &j["buttons"];

    if (buttonsRoot) {
        readString(*buttonsRoot, "texture", layout.buttonTexture);
        readVec2(*buttonsRoot, "btnRelSize", layout.btnRelSize, true);

        // legacy support (optional)
        readVec2(*buttonsRoot, "resumeRelPos", layout.resumeRelPos, true);
        readVec2(*buttonsRoot, "mainRelPos", layout.mainRelPos, true);
        readVec2(*buttonsRoot, "quitRelPos", layout.quitRelPos, true);

        // items array (editor-friendly)
        if (buttonsRoot->contains("items") && (*buttonsRoot)["items"].is_array()) {
            for (const auto& it : (*buttonsRoot)["items"]) {
                if (!it.is_object()) continue;
                if (!it.contains("id") || !it["id"].is_string()) continue;

                const std::string id = it["id"].get<std::string>();
                GuiButton* b = gui.FindButton(id);
                if (b) {
                    if (it.contains("label") && it["label"].is_string()) b->label = it["label"].get<std::string>();
                    if (it.contains("font") && it["font"].is_string())   b->font = it["font"].get<std::string>();
                    if (it.contains("fontSize") && it["fontSize"].is_number()) {
                        b->fontSize = it["fontSize"].get<float>();
                        b->baseFontSize = b->fontSize;
                    }
                    if (it.contains("textColor")) readUint(it, "textColor", b->textColor);
                }

                // relPos -> map to layout slots too
                if (it.contains("relPos") && it["relPos"].is_object()) {
                    Vector2 rp(0,0);
                    readVec2(it, "relPos", rp, true);

                    if (id == "pause.resume") layout.resumeRelPos = rp;
                    else if (id == "pause.main") layout.mainRelPos = rp;
                    else if (id == "pause.quit") layout.quitRelPos = rp;
                }
            }
        }
    }

    // ---- arrows ----
    const json* arrowsRoot = nullptr;
    if (j.contains("arrows") && j["arrows"].is_object())
        arrowsRoot = &j["arrows"];

    if (arrowsRoot) {
        readString(*arrowsRoot, "texture", layout.arrowTexture);

        readVec2(*arrowsRoot, "sizePx", layout.arrowSizePx, false);
        readVec2(*arrowsRoot, "leftOffsetPx", layout.leftArrowOffsetPx, false);
        readVec2(*arrowsRoot, "rightOffsetPx", layout.rightArrowOffsetPx, false);

        layout.arrowSizePx.x = std::max(0.0f, layout.arrowSizePx.x);
        layout.arrowSizePx.y = std::max(0.0f, layout.arrowSizePx.y);

        if (arrowsRoot->contains("items") && (*arrowsRoot)["items"].is_array()) {
            for (const auto& it : (*arrowsRoot)["items"]) {
                if (!it.is_object()) continue;
                if (!it.contains("id") || !it["id"].is_string()) continue;

                const std::string id = it["id"].get<std::string>();
                GuiButton* b = gui.FindButton(id);
                if (!b) continue;

                if (it.contains("label") && it["label"].is_string()) b->label = it["label"].get<std::string>();
                if (it.contains("font") && it["font"].is_string())   b->font = it["font"].get<std::string>();
                if (it.contains("fontSize") && it["fontSize"].is_number()) {
                    b->fontSize = it["fontSize"].get<float>();
                    b->baseFontSize = b->fontSize;
                }
                if (it.contains("textColor")) readUint(it, "textColor", b->textColor);
            }
        }
    }

    ReloadTextures(gui);

    return true;
}

/**
 * @brief Saves the current pause menu layout to a JSON configuration file.
 *
 * Serializes the current layout settings to a JSON file at the specified path,
 * creating parent directories if necessary. The output JSON is formatted with
 * indentation for human readability.
 *
 * @param path Relative path where the JSON layout file will be saved.
 * @return true if the layout was successfully written to disk.
 * @return false if the file could not be opened for writing.
 *
 *
 * @note The JSON structure follows the same organization as LoadLayout():
 * "panel", "slideshow", "buttons", "arrows", and "title" sections.
 */
bool PauseMenu::SaveLayout(const std::string& path, const GuiSystem& gui) const
{
    const std::string fullPath = AssetPath(path);

    try {
        std::filesystem::path p(fullPath);
        if (p.has_parent_path())
            std::filesystem::create_directories(p.parent_path());
    }
    catch (...) {}

    json j;

    j["panel"] = { {"wRel", layout.panelWRel}, {"hRel", layout.panelHRel} };

    // slideshow
    json ss;
    ss["wRel"] = layout.slideWRel;
    ss["hRel"] = layout.slideHRel;
    ss["xRel"] = layout.slideXRel;
    ss["yRel"] = layout.slideYRel;

    ss["slides"] = json::array();
    for (const auto& def : layout.slides) {
        ss["slides"].push_back({
            {"texture", def.texture},
            {"caption", def.caption},
            {"captionFont", def.captionFont},
            {"captionFontSizeRel", def.captionFontSizeRel},
            {"captionColor", def.captionColor}
            });
    }
    j["slideshow"] = ss;

    // title
    j["title"] = {
        {"id", layout.titleId},
        {"text", layout.titleText},
        {"font", layout.titleFont},
        {"size", layout.titleSize},
        {"color", layout.titleColor},
        {"yRel", layout.titleYRel}
    };

    // buttons
    json buttons;
    buttons["texture"] = layout.buttonTexture;
    buttons["btnRelSize"] = { {"x", layout.btnRelSize.x}, {"y", layout.btnRelSize.y} };

    auto makeBtnItem = [&](const char* id, const Vector2& relPos) -> json {
        json it;
        it["id"] = id;
        it["relPos"] = { {"x", relPos.x}, {"y", relPos.y} };

        if (const GuiButton* b = gui.FindButton(id)) {
            it["label"] = b->label;
            it["font"] = b->font;
            it["fontSize"] = b->fontSize;
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
        makeBtnItem("pause.resume", layout.resumeRelPos),
        makeBtnItem("pause.main",   layout.mainRelPos),
        makeBtnItem("pause.quit",   layout.quitRelPos)
        });

    // keep legacy keys (optional)
    buttons["resumeRelPos"] = { {"x", layout.resumeRelPos.x}, {"y", layout.resumeRelPos.y} };
    buttons["mainRelPos"] = { {"x", layout.mainRelPos.x},   {"y", layout.mainRelPos.y} };
    buttons["quitRelPos"] = { {"x", layout.quitRelPos.x},   {"y", layout.quitRelPos.y} };

    j["buttons"] = buttons;

    // arrows
    json arrows;
    arrows["texture"] = layout.arrowTexture;
    arrows["sizePx"] = { {"x", layout.arrowSizePx.x}, {"y", layout.arrowSizePx.y} };
    arrows["leftOffsetPx"] = { {"x", layout.leftArrowOffsetPx.x}, {"y", layout.leftArrowOffsetPx.y} };
    arrows["rightOffsetPx"] = { {"x", layout.rightArrowOffsetPx.x}, {"y", layout.rightArrowOffsetPx.y} };

    auto makeArrowItem = [&](const char* id) -> json {
        json it;
        it["id"] = id;
        if (const GuiButton* b = gui.FindButton(id)) {
            it["label"] = b->label;
            it["font"] = b->font;
            it["fontSize"] = b->fontSize;
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

    arrows["items"] = json::array({
        makeArrowItem("pause.arrowLeft"),
        makeArrowItem("pause.arrowRight")
        });

    j["arrows"] = arrows;

    std::ofstream ofs(fullPath);
    if (!ofs.is_open()) return false;

    ofs << j.dump(2);
    return true;
}


void PauseMenu::ReloadTextures(GuiSystem& gui){
    // --- Button textures ---
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);

    if (GuiButton* b = gui.FindButton("pause.resume"))
        b->texture = buttonTex;

    if (GuiButton* b = gui.FindButton("pause.main"))  
        b->texture = buttonTex;

    if (GuiButton* b = gui.FindButton("pause.quit"))   
        b->texture = buttonTex;

    // --- Arrow buttons ---
    unsigned arrowTex = ResourceManager::GetTexture(layout.arrowTexture);

    if (GuiButton* b = gui.FindButton("pause.arrowLeft"))
        b->texture = arrowTex;

    if (GuiButton* b = gui.FindButton("pause.arrowRight"))
        b->texture = arrowTex;

    // --- Slideshow textures ---
    slides.clear();
    for (const auto& s : layout.slides)
    {
        slides.push_back({
            { ResourceManager::GetTexture(s.texture) },
            s.caption
            });
    }
}




/**
    * @brief Renders the pause menu panel, slideshow, captions, and buttons.
    *
    * If the pause menu is visible, this function switches to screen-space
    * rendering (no world camera), lazily builds reusable meshes for the
    * central panel and slideshow outline, and then:
    *  - Computes a centered panel rectangle based on current window size.
    *  - Draws the dark panel background.
    *  - Lays out the slideshow region, draws the current slide texture, and
    *    renders its caption text using FontRenderer with wrapping and centering.
    *  - Draws an outline around the slideshow area.
    *  - Positions the left/right arrow buttons beside the slideshow.
    *  - Positions the Resume, Main Menu, and Quit buttons along the bottom
    *    of the panel using the stored relative positions.
    *  - Recenters the "HOW TO PLAY" title above the panel based on measured
    *    text width.
    * Finally, it calls gui.Draw() to render all GUI elements and restores
    * the previous camera.
    *
    * @param renderer Renderer used to draw meshes and text.
    * @param gui      GUI system that owns the buttons and texts.
*/
void PauseMenu::Draw(Renderer& renderer, GuiSystem& gui) const {
    if (!visible) return;

    Camera2D* prev = BeginScreenSpace(renderer);

    int ww, wh;
    glfwGetWindowSize(glfwGetCurrentContext(), &ww, &wh);

    float x, y, w, h;
    ComputePanelRect(ww, wh, x, y, w, h);

    DrawPanel(renderer, x, y, w, h, ww, wh);
    DrawSlideshow(renderer, x, y, w, h, ww, wh);
    LayoutArrows(gui, x, y, w, h);
    LayoutButtons(gui, x, y, w, h);
    LayoutTitle(gui, x, y, w, h);

    DrawGUI(renderer, gui);

    EndScreenSpace(renderer, prev);


    /* this is the visualisation per mini slide on how to play */
    // +------------------------------ slideW ------------------------------+
    // |   leftW (caption)   |          rightW (texture)                    |
    // +----------------------+---------------------------------------------+
  
}

/**
 * @brief Registers the "HOW TO PLAY" title text in the GUI.
 *
 * @param gui GUI system where the text is stored.
 */
void PauseMenu::BuildTexts(GuiSystem& gui){
    gui.AddText(GuiText(Vector2(0,0), layout.titleText, layout.titleSize));
    howToPlayTextIndex = (int)gui.GetTexts().size() - 1;

    auto& t = gui.GetTexts()[howToPlayTextIndex];
    t.id = layout.titleId;
    t.font = layout.titleFont;
    t.color = layout.titleColor;
}

/**
 * @brief Creates a unit quad mesh used for displaying slide textures.
 *
 * The quad is later scaled and positioned in NDC space.
 */
void PauseMenu::BuildSlideshowMesh(){
    slideshowMesh = Utility::BuildQuad(
        { Vector2(0,0), Vector2(1,0), Vector2(1,1), Vector2(0,1) },
        { Vector3(1,1,1), Vector3(1,1,1), Vector3(1,1,1), Vector3(1,1,1) }
    );
}

/**
 * @brief Populates the slideshow with three slides.
 *
 * Each slide contains:
 *  - One texture (loaded from ResourceManager)
 *  - A caption string
 */
void PauseMenu::BuildSlides(){
    slides.clear();

    // default slide if its not defined in json file 
    if (layout.slides.empty()){
        layout.slides = {
            { "how_to_play_WASD",     "Control player using WASD keys.", "default", 0.10f, 0xFFFFFFFF },
            { "how_to_play_navigate", "Navigate through doors.",         "default", 0.10f, 0xFFFFFFFF },
            { "how_to_play_enemies",  "Defeat enemies!",                 "default", 0.10f, 0xFFFFFFFF }
        };
    }

    for (const auto& def : layout.slides){
        HowToSlide s;
        s.textures.push_back(ResourceManager::GetTexture(def.texture));
        s.caption = def.caption;
        slides.push_back(std::move(s));
    }

    if (slides.empty()) currentSlide = 0;
    else currentSlide = std::clamp(currentSlide, 0, (int)slides.size() - 1);
}

/**
 * @brief Creates left and right arrow GUI buttons for slideshow navigation.
 *
 * Left arrow:  decreases currentSlide (if > 0)
 * Right arrow: increases currentSlide (if < slideCount-1)
 *
 * @param gui GUI system used to register arrow buttons.
 */
void PauseMenu::BuildArrows(GuiSystem& gui){
    unsigned tex = ResourceManager::GetTexture(layout.arrowTexture);

    auto& btns = gui.GetButtons();

    leftArrowIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0,0), Vector2(0,0), "<", [this]() {
        if (!slides.empty() && currentSlide > 0) currentSlide--;
        }, tex));
    btns[leftArrowIndex].id = "pause.arrowLeft";

    rightArrowIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0,0), Vector2(0,0), ">", [this]() {
        if (!slides.empty() && currentSlide < (int)slides.size() - 1) currentSlide++;
        }, tex));
    btns[rightArrowIndex].id = "pause.arrowRight";
}


/**
 * @brief Registers the Resume, Main Menu, and Quit buttons in the GUI.
 *
 * @param gui        GUI system where the buttons are stored.
 * @param onResume   Callback for the Resume action.
 * @param onMainMenu Callback for the Main Menu action.
 * @param onQuit     Callback for the Quit action.
 */
void PauseMenu::BuildButtons(
    GuiSystem& gui,
    std::function<void()> onResume,
    std::function<void()> onMainMenu,
    std::function<void()> onQuit){

    unsigned tex = ResourceManager::GetTexture(layout.buttonTexture);

    auto& btns = gui.GetButtons();

    resumeIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0,0), Vector2(0,0), "Resume", [onResume]() {
        if (onResume) onResume();
        }, tex));
    btns[resumeIndex].id = "pause.resume";

    mainIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0,0), Vector2(0,0), "Main Menu", [onMainMenu]() {
        if (onMainMenu) onMainMenu();
        }, tex));
    btns[mainIndex].id = "pause.main";

    quitIndex = (int)btns.size();
    gui.AddButton(GuiButton(Vector2(0,0), Vector2(0,0), "Quit", [onQuit]() {
        if (onQuit) onQuit();
        }, tex));
    btns[quitIndex].id = "pause.quit";
}

/**
 * @brief Switches renderer into screen-space mode by disabling the active camera.
 *
 * @param renderer Rendering interface.
 * @return Pointer to the previously active camera (to restore later).
 */
Camera2D* PauseMenu::BeginScreenSpace(Renderer& renderer) const{
    Camera2D* prev = renderer.getCamera();
    renderer.setCamera(nullptr);
    return prev;
}

/**
 * @brief Restores the previously active camera after screen-space drawing.
 *
 * @param renderer Rendering interface.
 * @param prev     The camera returned by BeginScreenSpace().
 */
void PauseMenu::EndScreenSpace(Renderer& renderer, Camera2D* prev) const{
    renderer.setCamera(prev);
}

/**
 * @brief Computes a centered panel rectangle for the pause menu.
 *
 * Panel width = 70% of screen width
 * Panel height = 50% of screen height
 *
 * @param ww Window width.
 * @param wh Window height.
 * @param x  Output X coordinate of the panel (pixels).
 * @param y  Output Y coordinate of the panel (pixels).
 * @param w  Output width  of the panel.
 * @param h  Output height of the panel.
 */
void PauseMenu::ComputePanelRect(int ww, int wh, float& x, float& y, float& w, float& h) const{
    w = ww * layout.panelWRel;
    h = wh * layout.panelHRel;
    x = (ww - w) * 0.5f;
    y = (wh - h) * 0.5f;
}

/**
 * @brief Draws the dark semi-transparent background panel.
 *
 * Creates the mesh once (static) and reuses it each frame.
 *
 * @param renderer Renderer used for drawing.
 * @param x        Panel X position in pixels.
 * @param y        Panel Y position in pixels.
 * @param w        Panel width  in pixels.
 * @param h        Panel height in pixels.
 * @param ww       Screen width.
 * @param wh       Screen height.
 */
void PauseMenu::DrawPanel(Renderer& renderer, float x, float y, float w, float h, int ww, int wh) const{
    static Mesh2D panel;
    static bool init = false;

    if (!init){
        Vector3 white(1, 1, 1);
        std::vector<Vertex2D> verts = {
            {{0,0},{0,0},white},
            {{1,0},{1,0},white},
            {{1,1},{1,1},white},
            {{0,1},{0,1},white}
        };
        std::vector<unsigned> inds = { 0,1,2, 0,2,3 };

        panel.Reserve(verts.size(), inds.size());
        panel.SetVertices(verts);
        panel.SetIndices(inds);
        init = true;
    }

    float ndcX = (x / ww) * 2.f - 1.f;
    float ndcY = (y / wh) * 2.f - 1.f;
    float ndcW = (w / ww) * 2.f;
    float ndcH = (h / wh) * 2.f;

    Matrix3x3 m =
        Matrix3x3::BuildTranslation(ndcX, ndcY) *
        Matrix3x3::BuildScaling(ndcW, ndcH);

    renderer.DrawMesh(panel, m, { 0.1f,0.1f,0.1f });
}

/**
 * @brief Draws the slideshow image and its caption text.
 *
 * Computes:
 *  - Right half rectangle for the texture
 *  - Left half rectangle for caption text
 *
 * @param renderer Rendering interface.
 * @param px Panel X position.
 * @param py Panel Y position.
 * @param pw Panel width.
 * @param ph Panel height.
 * @param ww Window width.
 * @param wh Window height.
 */
void PauseMenu::DrawSlideshow(Renderer& renderer, float px, float py, float pw, float ph, int ww, int wh) const{
    if (slides.empty() || currentSlide < 0 || currentSlide >= (int)slides.size())
        return;

    float slideW = pw * layout.slideWRel;
    float slideH = ph * layout.slideHRel;
    float slideX = px + layout.slideXRel * pw;
    float slideY = py + ph * layout.slideYRel;

    float rightX = slideX + slideW * 0.50f;
    float rightW = slideW * 0.50f;

    float ndcX = (rightX / ww) * 2.f - 1.f;
    float ndcY = (slideY / wh) * 2.f - 1.f;
    float ndcW = (rightW / ww) * 2.f;
    float ndcH = (slideH / wh) * 2.f;

    Matrix3x3 m =
        Matrix3x3::BuildTranslation(ndcX, ndcY) *
        Matrix3x3::BuildScaling(ndcW, ndcH);

    renderer.DrawMesh(slideshowMesh, m, { 1,1,1 }, slides[currentSlide].textures[0]);

    // caption area (left half)
    float leftW = slideW * 0.50f;
    float cx = slideX;
    float cy = slideY + slideH * 0.5f;

    // caption style from JSON layout (if you added layout.slides defs)
    const PauseSlideDef* def = nullptr;
    if (currentSlide >= 0 && currentSlide < (int)layout.slides.size())
        def = &layout.slides[currentSlide];

    const std::string font = def ? def->captionFont : "default";
    const float fontSize = (def ? def->captionFontSizeRel : 0.10f) * slideH;
    const unsigned color = def ? def->captionColor : 0xFFFFFFFF;

    float scale = (float)wh / 1080.f;

    EngineCore::FontRenderer::DrawText(
        renderer,
        font,
        fontSize,
        cx,
        cy,
        color,
        slides[currentSlide].caption,
        leftW - (40.f * scale),
        FontSys::Align::Center
    );
}

/**
 * @brief Positions left/right arrow buttons relative to slideshow area.
 *
 * @param gui GUI system whose buttons will be updated.
 * @param px Panel X.
 * @param py Panel Y.
 * @param pw Panel width.
 * @param ph Panel height.
 */
void PauseMenu::LayoutArrows(GuiSystem& gui, float px, float py, float pw, float ph) const{
    int ww, wh;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &ww, &wh);
    float scale = (float)wh / 1080.f;

    // Slideshow rect inside the panel (relative → pixel)
    const float slideW = pw * layout.slideWRel;
    const float slideH = ph * layout.slideHRel;

    // slideXRel is relative to panel width
    const float slideX = px + layout.slideXRel * pw;
    const float slideY = py + layout.slideYRel * ph;

    auto& btns = gui.GetButtons();

    // Left arrow
    if (leftArrowIndex >= 0) {
        btns[leftArrowIndex].pos = {
            slideX + (layout.leftArrowOffsetPx.x * scale),
            slideY + slideH * 0.5f + (layout.leftArrowOffsetPx.y * scale)
        };
        btns[leftArrowIndex].size = { layout.arrowSizePx.x * scale, layout.arrowSizePx.y * scale };
    }

    // Right arrow
    if (rightArrowIndex >= 0) {
        btns[rightArrowIndex].pos = {
            slideX + slideW + (layout.rightArrowOffsetPx.x * scale),
            slideY + slideH * 0.5f + (layout.rightArrowOffsetPx.y * scale)
        };
        btns[rightArrowIndex].size = { layout.arrowSizePx.x * scale, layout.arrowSizePx.y * scale };
    }
}

/**
 * @brief Positions Resume / Main Menu / Quit buttons along bottom of panel.
 *
 * Uses previously stored normalized positions and size.
 *
 * @param gui GUI system whose button transforms are modified.
 * @param px Panel X.
 * @param py Panel Y.
 * @param pw Panel width.
 * @param ph Panel height.
 */
void PauseMenu::LayoutButtons(GuiSystem& gui, float px, float py, float pw, float ph) const {
    auto& btns = gui.GetButtons();

    auto apply = [&](int idx, const Vector2& relPos) {
        if (idx < 0 || idx >= (int)btns.size()) return;

        // relPos is in panel space (0..1)
        btns[idx].pos = Vector2(px + relPos.x * pw, py + relPos.y * ph);
        btns[idx].size = Vector2(layout.btnRelSize.x * pw, layout.btnRelSize.y * ph);
        };

    apply(resumeIndex, layout.resumeRelPos);
    apply(mainIndex, layout.mainRelPos);
    apply(quitIndex, layout.quitRelPos);    
}


/**
 * @brief Positions the “HOW TO PLAY” title above the panel.
 *
 * Uses FontRenderer::ComputeTextWidth() to center text horizontally.
 *
 * @param gui GUI system managing the text record.
 * @param px Panel X.
 * @param py Panel Y.
 * @param pw Panel width.
 * @param ph Panel height.
 */
void PauseMenu::LayoutTitle(GuiSystem& gui, float px, float py, float pw, float ph) const{
    if (howToPlayTextIndex < 0) return;

    int ww, wh;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &ww, &wh);
    float scale = (float)wh / 1080.f;

    auto& t = gui.GetTexts()[howToPlayTextIndex];
    t.baseSize = layout.titleSize; // Update base size from layout
    float currentSize = t.baseSize * scale;
    float tw = EngineCore::FontRenderer::ComputeTextWidth(layout.titleFont, currentSize, t.text);
    float cx = px + pw * 0.5f;

    t.size = currentSize;
    t.pos = Vector2(cx - tw * 0.5f, py + ph * layout.titleYRel);
}

/**
 * @brief Draws all GUI elements registered in GuiSystem.
 *
 * @param renderer Renderer to issue draw calls.
 * @param gui      GUI system to draw.
 */
void PauseMenu::DrawGUI(Renderer& renderer, GuiSystem& gui) const{
    gui.Draw(renderer);
}