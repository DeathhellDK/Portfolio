/**
* @file     settingsMenu.cpp
* @author   Woh Kye Le
* @co-author Low Jianlin
* @email    w.kyele, jianlin.low
* @date     2025-12-06
*
* @brief
* Implements the SettingsMenu class, which provides a minimal screen-space UI
* for configuring game options. The menu currently contains a single “Back”
* button that returns the player to the Main Menu. The menu operates entirely
* in pixel-space via a GuiSystem and uses normalized coordinates for layout.
*
* @version 1.0
* @copyright
* Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "Core/engine.hpp"
#include "Core/resourceManager.h"   
#include "Core/assetsPath.h"

#include "Graphics/renderer.h"
#include "Graphics/camera2d.h"
#include "Graphics/mesh2d.h"
#include "Math/matrix3x3.h"

#include "Font/FontSystem.h"
#include "Font/FontRenderer.h"

#include "UI/settingsMenu.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <json.hpp>

using json = nlohmann::json;

// -------- Helper function ---------

/**
 * @brief Clamps a float value between 0.0 and 1.0 inclusive.
 *
 * @param v The float value to clamp.
 * @return The clamped value in the range [0.0, 1.0].
 */
static float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

/**
 * @brief Helper function to read a 2D vector from a JSON object.
 *
 * Reads x and y values from the JSON object at the specified key and stores them
 * in the provided glm::vec2 reference. If the key doesn't exist or the value isn't
 * an array with at least 2 elements, the vec2 remains unchanged.
 *
 * @param j The JSON object to read from.
 * @param key The key containing the array [x, y].
 * @param outVec Reference to the glm::vec2 where values will be stored.
 */
static void ReadVec2_Any(const json& obj, const char* key, Vector2& out, bool clamp01) {
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

/**
 * @brief Writes a 2D vector to a JSON object as an array.
 *
 * @param j The JSON object to write to.
 * @param key The key under which to store the vector.
 * @param v The Vector2 to write.
 */
 //static void WriteVec2(json& j, const char* key, const Vector2& v){
 //    j[key] = { v.x, v.y };
 //}

 /**
  * @brief Reads a string value from a JSON object.
  *
  * @param obj The JSON object to read from.
  * @param key The key containing the string value.
  * @param out Reference to the string where the value will be stored.
  */
static void ReadString(const json& obj, const char* key, std::string& out) {
    if (obj.contains(key) && obj[key].is_string()) out = obj[key].get<std::string>();
}

/**
 * @brief Reads a float value from a JSON object.
 *
 * @param obj The JSON object to read from.
 * @param key The key containing the float value.
 * @param out Reference to the float where the value will be stored.
 */
 //static void ReadFloat(const json& obj, const char* key, float& out){
 //    if (obj.contains(key) && obj[key].is_number()) out = obj[key].get<float>();
 //}

 /**
  * @brief Reads an unsigned integer value from a JSON object.
  *
  * Supports both unsigned and signed integer types in the JSON.
  * If the key doesn't exist or the value isn't a number, the output remains unchanged.
  *
  * @param obj The JSON object to read from.
  * @param key The key containing the unsigned integer value.
  * @param out Reference to the unsigned integer where the value will be stored.
  */
static void ReadUint(const json& obj, const char* key, unsigned& out) {
    if (!obj.contains(key) || !obj[key].is_number()) return;
    if (obj[key].is_number_unsigned()) out = obj[key].get<unsigned>();
    else out = static_cast<unsigned>(obj[key].get<int>());
}

/**
 * @brief Converts a 32-bit unsigned integer color value (0xAARRGGBB format) to RGB Vector3.
 *
 * The function extracts the red, green, and blue components from the 32-bit color
 * and converts them to floating-point values in the range [0.0, 1.0].
 * The alpha channel is ignored.
 *
 * @param c The 32-bit unsigned integer color in 0xAARRGGBB format.
 * @return Vector3 containing the RGB components in range [0.0, 1.0].
 */
static Vector3 ColorU32_ToRGB(unsigned c) {
    // Assuming 0xAARRGGBB (consistent with 0xFFFFFFFF = white)
    unsigned r = (c >> 16) & 0xFF;
    unsigned g = (c >> 8) & 0xFF;
    unsigned b = (c) & 0xFF;
    return Vector3(r / 255.f, g / 255.f, b / 255.f);
}

/**
 * @brief Loads the menu layout from a JSON file.
 *
 * This function attempts to load layout parameters (button sizes, navigation positions,
 * slider dimensions, etc.) from a JSON file at the specified relative path.
 * If the file doesn't exist or contains invalid JSON, the function fails silently
 * and returns false. Missing fields in the JSON file are replaced with the current
 * layout's default values.
 *
 * @param relPath The relative path to the JSON layout file.
 * @param gui Reference to the GuiSystem to apply texture changes.
 * @return true if the layout was successfully loaded from the file.
 * @return false if the file couldn't be opened, contained invalid JSON, or couldn't be parsed.
 *
 * @note The function uses safe defaults via `j.value()` for missing fields.
 * @see SaveLayout()
 */
bool SettingsMenu::LoadLayout(const std::string& relPath, GuiSystem& gui)
{
    const std::string fullPath = AssetPath(relPath);
    std::ifstream ifs(fullPath);
    if (!ifs.is_open()) return false;

    json j;
    try { ifs >> j; }
    catch (...) { return false; }

    // background (optional)
    if (j.contains("background") && j["background"].is_object())
        ReadString(j["background"], "texture", layout.bgTexture);
    else
        ReadString(j, "bgTexture", layout.bgTexture);

    // buttons root
    const json* buttonsRoot = nullptr;
    if (j.contains("buttons") && j["buttons"].is_object())
        buttonsRoot = &j["buttons"];

    // textures
    if (buttonsRoot) ReadString(*buttonsRoot, "texture", layout.buttonTexture);
    else ReadString(j, "buttonTexture", layout.buttonTexture);

    // base layout fields
    const json* layoutRoot = buttonsRoot ? buttonsRoot : &j;

    ReadVec2_Any(*layoutRoot, "btnRelSize", layout.btnRelSize, true);
    ReadVec2_Any(*layoutRoot, "gameplayRelPos", layout.gameplayRelPos, true);
    ReadVec2_Any(*layoutRoot, "controlsRelPos", layout.controlsRelPos, true);
    ReadVec2_Any(*layoutRoot, "audioRelPos", layout.audioRelPos, true);
    ReadVec2_Any(*layoutRoot, "backRelPos", layout.backRelPos, true);

    // slider fields
    if (j.contains("audio") && j["audio"].is_object()) {
        const json& a = j["audio"];
        if (a.contains("slider") && a["slider"].is_object()) {
            const json& s = a["slider"];
            ReadVec2_Any(s, "relPos", layout.sliderRelPos, true);
            ReadVec2_Any(s, "relSize", layout.sliderRelSize, true);

            ReadUint(s, "barColor", layout.sliderBarColor);
            ReadUint(s, "knobColor", layout.sliderKnobColor);
            ReadUint(s, "knobDragColor", layout.sliderKnobDragColor);
        }

        if (a.contains("label") && a["label"].is_object()) {
            const json& lab = a["label"];
            ReadVec2_Any(lab, "offsetRel", layout.audioLabelOffsetRel, false);

            // apply label style to GuiText
            if (GuiText* t = gui.FindText("Settings.Audio.Label")) {
                if (lab.contains("text") && lab["text"].is_string()) t->text = lab["text"].get<std::string>();
                if (lab.contains("font") && lab["font"].is_string()) t->font = lab["font"].get<std::string>();
                if (lab.contains("fontSize") && lab["fontSize"].is_number()) {
                    t->size = lab["fontSize"].get<float>();
                    t->baseSize = t->size;
                }
                if (lab.contains("color")) ReadUint(lab, "color", t->color);
            }
        }
    }
    else {
        // legacy compatibility
        ReadVec2_Any(j, "sliderPosRel", layout.sliderRelPos, true);
        ReadVec2_Any(j, "sliderSizeRel", layout.sliderRelSize, true);
        ReadVec2_Any(j, "audioLabelOffsetRel", layout.audioLabelOffsetRel, false);
    }

    // Apply per-button styles via editor-friendly items[]
    // Format:
    // "buttons": { "items": [ { "id","label","relPos":{x,y},"font","fontSize","textColor" } ] }
    if (buttonsRoot && buttonsRoot->contains("items") && (*buttonsRoot)["items"].is_array()) {
        for (const auto& it : (*buttonsRoot)["items"]) {
            if (!it.is_object()) continue;
            if (!it.contains("id") || !it["id"].is_string()) continue;

            const std::string id = it["id"].get<std::string>();
            GuiButton* b = gui.FindButton(id);
            if (!b) continue;

            if (it.contains("label") && it["label"].is_string()) b->label = it["label"].get<std::string>();
            if (it.contains("font") && it["font"].is_string()) b->font = it["font"].get<std::string>();
            if (it.contains("fontSize") && it["fontSize"].is_number()) {
                b->fontSize = it["fontSize"].get<float>();
                b->baseFontSize = b->fontSize;
            }
            if (it.contains("textColor")) ReadUint(it, "textColor", b->textColor);

            // relPos -> push into layout slots
            if (it.contains("relPos")) {
                Vector2 rp = { 0.f, 0.f };
                ReadVec2_Any(it, "relPos", rp, true);

                if (id == "Settings.Nav.Gameplay") layout.gameplayRelPos = rp;
                else if (id == "Settings.Nav.Controls") layout.controlsRelPos = rp;
                else if (id == "Settings.Nav.Audio") layout.audioRelPos = rp;
                else if (id == "Settings.Back") layout.backRelPos = rp;
            }
        }
    }

    // Refresh textures onto widgets (so JSON texture changes take effect)
    ReloadTextures(gui);

    return true;
}

/**
 * @brief Saves the current menu layout to a JSON file.
 *
 * This function serializes the current layout settings (button sizes, navigation positions,
 * slider dimensions, etc.) to a JSON file at the specified relative path.
 * The parent directory is created if it doesn't exist. The output JSON is formatted
 * with indentation for readability.
 *
 * @param relPath The relative path where the JSON layout file will be saved.
 * @param gui Reference to the GuiSystem to read current button/label styles.
 * @return true if the layout was successfully saved to the file.
 * @return false if the file couldn't be opened or created for writing.
 *
 * @note Creates necessary directories automatically using `std::filesystem::create_directories`.
 * @see LoadLayout()
 */
bool SettingsMenu::SaveLayout(const std::string& relPath, const GuiSystem& gui) const {
    const std::string fullPath = AssetPath(relPath);

    try {
        std::filesystem::path p(fullPath);
        if (p.has_parent_path())
            std::filesystem::create_directories(p.parent_path());
    }
    catch (...) {}

    json j;

    // background (optional)
    j["background"] = { {"texture", layout.bgTexture} };

    // buttons block (editor friendly)
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
        makeBtnItem("Settings.Nav.Gameplay", layout.gameplayRelPos),
        makeBtnItem("Settings.Nav.Controls", layout.controlsRelPos),
        makeBtnItem("Settings.Nav.Audio",    layout.audioRelPos),
        makeBtnItem("Settings.Back",         layout.backRelPos)
        });

    // backwards compatibility
    buttons["gameplayRelPos"] = { {"x", layout.gameplayRelPos.x}, {"y", layout.gameplayRelPos.y} };
    buttons["controlsRelPos"] = { {"x", layout.controlsRelPos.x}, {"y", layout.controlsRelPos.y} };
    buttons["audioRelPos"] = { {"x", layout.audioRelPos.x},    {"y", layout.audioRelPos.y} };
    buttons["backRelPos"] = { {"x", layout.backRelPos.x},     {"y", layout.backRelPos.y} };

    j["buttons"] = buttons;

    // audio section (slider + label style)
    json audio;

    json slider;
    slider["relPos"] = { {"x", layout.sliderRelPos.x}, {"y", layout.sliderRelPos.y} };
    slider["relSize"] = { {"x", layout.sliderRelSize.x}, {"y", layout.sliderRelSize.y} };
    slider["barColor"] = layout.sliderBarColor;
    slider["knobColor"] = layout.sliderKnobColor;
    slider["knobDragColor"] = layout.sliderKnobDragColor;
    audio["slider"] = slider;

    json label;
    label["id"] = "Settings.Audio.Label";
    label["offsetRel"] = { {"x", layout.audioLabelOffsetRel.x}, {"y", layout.audioLabelOffsetRel.y} };
    if (const GuiText* t = gui.FindText("Settings.Audio.Label")) {
        label["text"] = t->text;
        label["font"] = t->font;
        label["fontSize"] = t->size;
        label["color"] = t->color;
    }
    audio["label"] = label;

    j["audio"] = audio;

    std::ofstream ofs(fullPath);
    if (!ofs.is_open()) return false;

    ofs << j.dump(2);
    return true;
}

/**
 * @brief Reloads textures for all navigation buttons from the layout settings.
 *
 * This function applies the current button texture from the layout to all
 * navigation buttons in the GUI system. This is useful after loading a new
 * layout from JSON to ensure visual consistency.
 *
 * @param gui Reference to the GuiSystem containing the buttons.
 */
void SettingsMenu::ReloadTextures(GuiSystem& gui) {
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);

    if (GuiButton* b = gui.FindButton("Settings.Nav.Gameplay"))
        b->texture = buttonTex;

    if (GuiButton* b = gui.FindButton("Settings.Nav.Controls"))
        b->texture = buttonTex;

    if (GuiButton* b = gui.FindButton("Settings.Nav.Audio"))
        b->texture = buttonTex;

    if (GuiButton* b = gui.FindButton("Settings.Back"))
        b->texture = buttonTex;
}

/**
    * @brief Initializes all UI elements used by the Settings menu.
    *
    * This function constructs the left-side navigation buttons
    * (Gameplay, Controls, Audio) and registers the "Exit" button
    * that returns the user to the Main Menu. It also sets up all
    * UI widgets required by the Audio section (slider label, etc.).
    *
    * The menu begins on the Gameplay section.
    *
    * @param gui    Reference to the GuiSystem where widgets are registered.
    * @param onBack Callback executed when the Exit button is clicked.
*/
void SettingsMenu::Build(GuiSystem& gui, std::function<void()> onBack, std::function<void()> onOpenControls) {
    currentSection = SettingsSection::Gameplay;
    audioModeActive = false;
    onBackCallback = onBack;

    BuildNavigation(gui, onBack, onOpenControls);
    BuildAudioUI(gui);

    // Fetch current Audio levels and map them to 0.0 - 1.0
    masterVolume = DbToSlider(Audio::GetBusDb(Audio::Bus::Master));
    bgmVolume = DbToSlider(Audio::GetBusDb(Audio::Bus::Music));
    sfxVolume = DbToSlider(Audio::GetBusDb(Audio::Bus::Sfx));

    ReloadTextures(gui);
}

/**
    * @brief Updates interactive logic for the currently selected settings section.
    *
    * Only the active section receives input processing.
    * Currently, only the Audio section contains interactive elements
    * (the volume slider), so input is forwarded to UpdateAudioSlider()
    * only when Audio is active.
    *
    * @param in Reference to the engine-wide Input system.
*/
void SettingsMenu::Update(const eng::Input& in, GuiSystem& gui) {
    if (!visible) return;
    
    if (in.isKeyPressed(GLFW_KEY_ESCAPE)) {
        if (onBackCallback) onBackCallback();
        return;
    }

    if (audioModeActive && in.isGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_B)) {
        audioModeActive = false;
        draggingMaster = false;
        draggingBgm = false;
        draggingSfx = false;
        if (audioFirstSliderIndex >= 0 && audioFirstSliderIndex + 2 < (int)gui.GetButtons().size()) {
            for (int i = 0; i < 3; i++) {
                gui.GetButtons()[audioFirstSliderIndex + i].visible = false;
                gui.GetButtons()[audioFirstSliderIndex + i].gamepadSelectable = false;
            }
        }
        if (GuiButton* b = gui.FindButton("Settings.Nav.Gameplay")) b->gamepadSelectable = true;
        if (GuiButton* b = gui.FindButton("Settings.Nav.Controls")) b->gamepadSelectable = true;
        if (GuiButton* b = gui.FindButton("Settings.Nav.Audio")) b->gamepadSelectable = true;
        if (GuiButton* b = gui.FindButton("Settings.Back")) b->gamepadSelectable = true;
        if (audioNavButtonIndex >= 0 && audioNavButtonIndex < (int)gui.GetButtons().size())
            gui.selectedIndex = audioNavButtonIndex;
    }

    if (audioModeActive && currentSection == SettingsSection::Audio)
        UpdateAudioSlider(in);
}


/**
    * @brief Renders the Settings menu and all visible UI elements in screen space.
    *
    * This function:
    *  1. Switches the renderer to a null camera (pure screen-space rendering).
    *  2. Computes pixel layout for navigation buttons and per-section widgets.
    *  3. Draws the content of the active section (e.g., audio slider).
    *  4. Delegates the actual mesh/text drawing to GuiSystem::Draw().
    *  5. Restores the previous camera.
    *
    * If the menu is not visible, the function exits immediately.
    *
    * @param renderer Renderer instance used for drawing quads and meshes.
    * @param gui      GuiSystem containing all registered settings UI widgets.
*/
void SettingsMenu::Draw(Renderer& renderer, GuiSystem& gui) const {
    if (!visible) return;

    Camera2D* prev = renderer.getCamera();
    renderer.setCamera(nullptr);

    int ww = 1280, wh = 720;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &ww, &wh);

    // Left panel layout
    LayoutNavigation(gui, ww, wh);

    // Audio UI layout (only positions label)
    LayoutAudioUI(gui, ww, wh);

    // Draw per-section content
    if (audioModeActive && currentSection == SettingsSection::Audio)
        DrawAudio(renderer, gui, ww, wh);

    // Draw all GUI buttons + texts
    gui.Draw(renderer);
    renderer.setCamera(prev);
}

/**
    * @brief Registers all left-panel navigation buttons (Gameplay / Controls / Audio)
    *        and the bottom-left Exit button.
    *
    * Each navigation button switches the active settings section when clicked.
    * All buttons share the same texture and are stored inside the GuiSystem.
    *
    * @param gui    Reference to the GuiSystem for registering UI buttons.
    * @param onBack Callback executed when the Exit button is pressed.
*/
void SettingsMenu::BuildNavigation(GuiSystem& gui, std::function<void()> onBack, std::function<void()> onOpenControls) {
    unsigned buttonTex = ResourceManager::GetTexture(layout.buttonTexture);
    GuiSystem* guiPtr = &gui;

    {
        GuiButton b({ 0,0 }, { 0,0 }, "Gameplay", [this, guiPtr]() {
            currentSection = SettingsSection::Gameplay;
            audioModeActive = false;
            draggingMaster = false;
            draggingBgm = false;
            draggingSfx = false;
            if (guiPtr && audioFirstSliderIndex >= 0 && audioFirstSliderIndex + 2 < (int)guiPtr->GetButtons().size()) {
                for (int i = 0; i < 3; i++) {
                    guiPtr->GetButtons()[audioFirstSliderIndex + i].visible = false;
                    guiPtr->GetButtons()[audioFirstSliderIndex + i].gamepadSelectable = false;
                }
            }
            if (guiPtr) {
                if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Gameplay")) b2->gamepadSelectable = true;
                if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Controls")) b2->gamepadSelectable = true;
                if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Audio")) b2->gamepadSelectable = true;
                if (GuiButton* b2 = guiPtr->FindButton("Settings.Back")) b2->gamepadSelectable = true;
            }
            }, buttonTex);
        b.id = "Settings.Nav.Gameplay";
        gui.AddButton(b);
    }
    {
        GuiButton b({ 0,0 }, { 0,0 }, "Controls", [this, guiPtr, onOpenControls]() {
            audioModeActive = false;
            draggingMaster = false;
            draggingBgm = false;
            draggingSfx = false;
            if (guiPtr && audioFirstSliderIndex >= 0 && audioFirstSliderIndex + 2 < (int)guiPtr->GetButtons().size()) {
                for (int i = 0; i < 3; i++) {
                    guiPtr->GetButtons()[audioFirstSliderIndex + i].visible = false;
                    guiPtr->GetButtons()[audioFirstSliderIndex + i].gamepadSelectable = false;
                }
            }
            if (guiPtr) {
                if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Gameplay")) b2->gamepadSelectable = true;
                if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Controls")) b2->gamepadSelectable = true;
                if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Audio")) b2->gamepadSelectable = true;
                if (GuiButton* b2 = guiPtr->FindButton("Settings.Back")) b2->gamepadSelectable = true;
            }
            if (onOpenControls) onOpenControls();
            }, buttonTex);
        b.id = "Settings.Nav.Controls";
        gui.AddButton(b);
    }
    {
        audioNavButtonIndex = (int)gui.GetButtons().size();
        GuiButton b({ 0,0 }, { 0,0 }, "Audio", [this, guiPtr]() {
            currentSection = SettingsSection::Audio;
            if (audioModeActive) {
                audioModeActive = false;
                draggingMaster = false;
                draggingBgm = false;
                draggingSfx = false;
                if (guiPtr && audioFirstSliderIndex >= 0 && audioFirstSliderIndex + 2 < (int)guiPtr->GetButtons().size()) {
                    for (int i = 0; i < 3; i++) {
                        guiPtr->GetButtons()[audioFirstSliderIndex + i].visible = false;
                        guiPtr->GetButtons()[audioFirstSliderIndex + i].gamepadSelectable = false;
                    }
                }
                if (guiPtr) {
                    if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Gameplay")) b2->gamepadSelectable = true;
                    if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Controls")) b2->gamepadSelectable = true;
                    if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Audio")) b2->gamepadSelectable = true;
                    if (GuiButton* b2 = guiPtr->FindButton("Settings.Back")) b2->gamepadSelectable = true;
                    guiPtr->selectedIndex = audioNavButtonIndex;
                }
            }
            else {
                audioModeActive = true;
                if (guiPtr && audioFirstSliderIndex >= 0 && audioFirstSliderIndex + 2 < (int)guiPtr->GetButtons().size()) {
                    for (int i = 0; i < 3; i++) {
                        guiPtr->GetButtons()[audioFirstSliderIndex + i].visible = true;
                        guiPtr->GetButtons()[audioFirstSliderIndex + i].gamepadSelectable = true;
                    }
                    guiPtr->selectedIndex = audioFirstSliderIndex;
                }
                if (guiPtr) {
                    if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Gameplay")) b2->gamepadSelectable = false;
                    if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Controls")) b2->gamepadSelectable = false;
                    if (GuiButton* b2 = guiPtr->FindButton("Settings.Nav.Audio")) b2->gamepadSelectable = true;
                    if (GuiButton* b2 = guiPtr->FindButton("Settings.Back")) b2->gamepadSelectable = false;
                }
            }
            }, buttonTex);
        b.id = "Settings.Nav.Audio";
        gui.AddButton(b);
    }
    {
        GuiButton b({ 0,0 }, { 0,0 }, "Back", [onBack]() { if (onBack) onBack(); }, buttonTex);
        b.id = "Settings.Back";
        gui.AddButton(b);
    }
}


/**
    * @brief Registers all text/UI elements belonging to the Audio section.
    *
    * Currently adds the "Sound SFX" label. These elements are not positioned here;
    * their position and visibility are handled in LayoutAudioUI().
    *
    * @param gui Reference to the GuiSystem for registering UI texts.
*/
void SettingsMenu::BuildAudioUI(GuiSystem& gui) {
    GuiText t1({ 0,0 }, "Master Volume", 28.f); t1.id = "Settings.Audio.Master";
    GuiText t2({ 0,0 }, "Music Volume", 28.f);  t2.id = "Settings.Audio.Music";
    GuiText t3({ 0,0 }, "SFX Volume", 28.f);    t3.id = "Settings.Audio.SFX";

    gui.AddText(t1);
    gui.AddText(t2);
    gui.AddText(t3);

    auto makeSlider = [&](const char* id, float* value, bool* dragging, Audio::Bus bus)
        {
            GuiButton b({ 0,0 }, { 0,0 }, "", {}, 0);
            b.id = id;
            b.visible = false;
            b.mouseInteractable = false;
            b.gamepadSelectable = false;
            b.drawQuad = false;
            b.drawLabel = false;
            b.onAdjust = [this, value, dragging, bus](int dir) {
                if (dragging) *dragging = false;
                float step = 0.03f;
                *value = std::clamp(*value + (dir < 0 ? -step : step), 0.0f, 1.0f);
                Audio::SetBusDb(bus, SliderToDb(*value));
                };
            gui.AddButton(b);
        };

    audioFirstSliderIndex = (int)gui.GetButtons().size();
    makeSlider("Settings.Audio.Slider.Master", &masterVolume, &draggingMaster, Audio::Bus::Master);
    makeSlider("Settings.Audio.Slider.Music", &bgmVolume, &draggingBgm, Audio::Bus::Music);
    makeSlider("Settings.Audio.Slider.Sfx", &sfxVolume, &draggingSfx, Audio::Bus::Sfx);
}



/**
    * @brief Handles mouse interaction for the Audio volume slider.
    *
    * This includes:
    *  - Detecting whether the mouse is inside the slider bar
    *  - Starting / stopping slider drag
    *  - Updating volumeValue (clamped to 0..1) as the knob is dragged
    *
    * The function assumes slider UI should only process input when Audio
    * section is active (ensured by Update()).
    *
    * @param in Reference to the engine's Input handler.
*/
void SettingsMenu::UpdateAudioSlider(const eng::Input& in) {

    GLFWwindow* win = glfwGetCurrentContext();
    double mx, my;
    glfwGetCursorPos(win, &mx, &my);

    int ww, wh, fbw, fbh;
    glfwGetWindowSize(win, &ww, &wh);
    glfwGetFramebufferSize(win, &fbw, &fbh);

    float scaleX = (ww > 0) ? (float)fbw / (float)ww : 1.f;
    float scaleY = (wh > 0) ? (float)fbh / (float)wh : 1.f;
    float px = (float)mx * scaleX;
    float py = (float)((wh - my) * scaleY);

    float spacing = 0.15f;

    // Helper lambda to process a specific slider
    auto processSlider = [&](int index, bool& dragging, float& volVal, Audio::Bus bus) {
        float sx = layout.sliderRelPos.x * fbw;
        float sy = (layout.sliderRelPos.y - index * spacing) * fbh;
        float sw = layout.sliderRelSize.x * fbw;
        float sh = layout.sliderRelSize.y * fbh;

        bool inside = (px >= sx && px <= sx + sw && py >= sy && py <= sy + sh);

        if (inside && in.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
            dragging = true;

        if (!in.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
            dragging = false;

        if (dragging) {
            float travel = std::max(1.0f, sw - sh);
            float knobHalf = sh * 0.5f;
            volVal = std::clamp((px - (sx + knobHalf)) / travel, 0.f, 1.f);

            // Apply volume to audio engine instantly
            Audio::SetBusDb(bus, SliderToDb(volVal));
        }
        };

    // Process all three sliders independently
    processSlider(0, draggingMaster, masterVolume, Audio::Bus::Master);
    processSlider(1, draggingBgm, bgmVolume, Audio::Bus::Music);
    processSlider(2, draggingSfx, sfxVolume, Audio::Bus::Sfx);
}

/**
    * @brief Computes pixel positions and sizes for all left-panel navigation buttons
    *        and the bottom-left Exit button.
    *
    * This function performs layout only; it does not draw anything.
    * It uses a fixed vertical spacing and relative sizing based on window resolution.
    *
    * @param gui Reference to the GuiSystem containing the navigation buttons.
    * @param ww  Window width in pixels.
    * @param wh  Window height in pixels.
*/
void SettingsMenu::LayoutNavigation(GuiSystem& gui, int ww, int wh) const {
    const float bw = layout.btnRelSize.x * ww;
    const float bh = layout.btnRelSize.y * wh;

    auto placeBtn = [&](const char* id, const Vector2& rp)
        {
            if (GuiButton* b = gui.FindButton(id)) {
                b->pos = { rp.x * ww, rp.y * wh };
                b->size = { bw, bh };
            }
        };

    placeBtn("Settings.Nav.Gameplay", layout.gameplayRelPos);
    placeBtn("Settings.Nav.Controls", layout.controlsRelPos);
    placeBtn("Settings.Nav.Audio", layout.audioRelPos);
    placeBtn("Settings.Back", layout.backRelPos);
}

/**
    * @brief Computes position and visibility for all Audio-section UI widgets.
    *
    * If the Audio section is not active, all widgets belonging to it are hidden.
    * If active, the "Sound SFX" label is positioned relative to the slider.
    *
    * @param gui Reference to the GuiSystem containing text elements.
    * @param ww  Window width in pixels.
    * @param wh  Window height in pixels.
*/
void SettingsMenu::LayoutAudioUI(GuiSystem& gui, int ww, int wh) const {
    GuiText* l1 = gui.FindText("Settings.Audio.Master");
    GuiText* l2 = gui.FindText("Settings.Audio.Music");
    GuiText* l3 = gui.FindText("Settings.Audio.SFX");

    bool vis = (audioModeActive && currentSection == SettingsSection::Audio);
    if (l1) l1->visible = vis;
    if (l2) l2->visible = vis;
    if (l3) l3->visible = vis;

    GuiButton* s1 = gui.FindButton("Settings.Audio.Slider.Master");
    GuiButton* s2 = gui.FindButton("Settings.Audio.Slider.Music");
    GuiButton* s3 = gui.FindButton("Settings.Audio.Slider.Sfx");
    if (s1) { s1->visible = vis; s1->gamepadSelectable = vis; }
    if (s2) { s2->visible = vis; s2->gamepadSelectable = vis; }
    if (s3) { s3->visible = vis; s3->gamepadSelectable = vis; }

    if (GuiButton* b = gui.FindButton("Settings.Nav.Gameplay")) b->gamepadSelectable = !audioModeActive;
    if (GuiButton* b = gui.FindButton("Settings.Nav.Controls")) b->gamepadSelectable = !audioModeActive;
    if (GuiButton* b = gui.FindButton("Settings.Nav.Audio")) b->gamepadSelectable = true;
    if (GuiButton* b = gui.FindButton("Settings.Back")) b->gamepadSelectable = !audioModeActive;

    if (!vis) return;

    float spacing = 0.15f; // 15% vertical spacing between sliders
    float sx = layout.sliderRelPos.x * ww;
    float sw = layout.sliderRelSize.x * ww;
    float sh = layout.sliderRelSize.y * wh;

    auto placeSliderBtn = [&](GuiButton* b, int i)
        {
            if (!b) return;
            float sy = (layout.sliderRelPos.y - i * spacing) * wh;
            b->pos = { sx, sy };
            b->size = { sw, sh };
        };

    placeSliderBtn(s1, 0);
    placeSliderBtn(s2, 1);
    placeSliderBtn(s3, 2);

    // Position each label down vertically
    for (int i = 0; i < 3; ++i) {
        GuiText* label = (i == 0) ? l1 : ((i == 1) ? l2 : l3);
        if (label) {
            float sy = (layout.sliderRelPos.y - i * spacing) * wh; // Stack downward

            label->pos = {
                sx + layout.audioLabelOffsetRel.x * ww,
                sy + layout.audioLabelOffsetRel.y * wh
            };
        }
    }

    unsigned normal = 0xFFFFFFFF;
    unsigned highlight = layout.sliderKnobDragColor;
    if (l1) l1->color = (draggingMaster || (s1 && s1->hovered)) ? highlight : normal;
    if (l2) l2->color = (draggingBgm || (s2 && s2->hovered)) ? highlight : normal;
    if (l3) l3->color = (draggingSfx || (s3 && s3->hovered)) ? highlight : normal;
}

/**
    * @brief Renders the Audio section UI elements (slider bar and knob).
    *
    * The slider is rendered manually here using the renderer:
    *  - A horizontal bar (gray background)
    *  - A movable knob whose position represents volumeValue
    *
    * The function assumes that visibility checks have already been handled.
    *
    * @param renderer Renderer used for drawing quads in NDC.
    * @param gui      GuiSystem providing the shared quad mesh.
    * @param ww       Window width in pixels.
    * @param wh       Window height in pixels.
*/
void SettingsMenu::DrawAudio(Renderer& renderer, GuiSystem& gui, int ww, int wh) const {
    float spacing = 0.15f;

    bool s1Sel = false, s2Sel = false, s3Sel = false;
    if (const GuiButton* b = gui.FindButton("Settings.Audio.Slider.Master")) s1Sel = b->hovered;
    if (const GuiButton* b = gui.FindButton("Settings.Audio.Slider.Music")) s2Sel = b->hovered;
    if (const GuiButton* b = gui.FindButton("Settings.Audio.Slider.Sfx")) s3Sel = b->hovered;

    auto drawSlider = [&](int index, float volVal, bool dragging, bool selected) {
        float sx = layout.sliderRelPos.x * ww;
        float sy = (layout.sliderRelPos.y - index * spacing) * wh;
        float sw = layout.sliderRelSize.x * ww;
        float sh = layout.sliderRelSize.y * wh;

        // Convert to NDC
        float ndcX = (sx / ww) * 2.f - 1.f;
        float ndcY = (sy / wh) * 2.f - 1.f;
        float ndcW = (sw / ww) * 2.f;
        float ndcH = (sh / wh) * 2.f;

        // Draw Slider bar
        Matrix3x3 bar = Matrix3x3::BuildTranslation(ndcX, ndcY) * Matrix3x3::BuildScaling(ndcW, ndcH);
        renderer.DrawMesh(gui.GetQuadMesh(), bar, ColorU32_ToRGB(layout.sliderBarColor));

        // Draw Slider knob
        float knobPixelX = sx + volVal * (sw - sh);
        float knobNdcX = (knobPixelX / ww) * 2.f - 1.f;
        float knobNdcW = (sh / ww) * 2.f;
        float knobNdcH = (sh / wh) * 2.f;

        Matrix3x3 knob = Matrix3x3::BuildTranslation(knobNdcX, ndcY) * Matrix3x3::BuildScaling(knobNdcW, knobNdcH);
        unsigned knobColor = (dragging || selected) ? layout.sliderKnobDragColor : layout.sliderKnobColor;
        renderer.DrawMesh(gui.GetQuadMesh(), knob, ColorU32_ToRGB(knobColor));
        };

    drawSlider(0, masterVolume, draggingMaster, s1Sel);
    drawSlider(1, bgmVolume, draggingBgm, s2Sel);
    drawSlider(2, sfxVolume, draggingSfx, s3Sel);
}

/**
    * @brief Converts a normalized slider position (0..1) into an audio
    *        decibel value using a symmetric range: -40 dB → 0 dB → +6 dB.
    *
    * Slider mapping:
    *  - slider = 0.0 → -40 dB  (minimum volume)
    *  - slider = 0.5 →  0 dB   (default / neutral volume)
    *  - slider = 1.0 → +6 dB   (boosted volume)
    *
    * This allows the middle of the UI slider to represent standard 0 dB,
    * giving intuitive control over both attenuation and amplification.
    *
    * @param slider Normalized slider value in the range [0, 1].
    * @return Corresponding decibel value in the range [-40, +6].
*/
float SettingsMenu::SliderToDb(float slider) {
    slider = std::clamp(slider, 0.0f, 1.0f);
    if (slider <= 0.01f) return -80.0f; // Effectively mute it completely at 0%
    return -40.f * (1.f - slider);      // 0..1 maps to -40..0
}

/**
    * @brief Converts an audio decibel value into a normalized slider
    *        position (0..1) suitable for UI representation.
    *
    * Decibel mapping:
    *  - -40 dB → 0.0 (left edge of slider)
    *  -   0 dB → 0.5 (center of slider)
    *  -  +6 dB → 1.0 (right edge of slider)
    *
    * Values outside the expected range are clamped before mapping.
    *
    * @param db Decibel value to convert (typically between -40 and +6 dB).
    * @return Slider position in the range [0, 1] for UI display.
*/
float SettingsMenu::DbToSlider(float db) {
    if (db <= -79.0f) return 0.0f;
    db = std::clamp(db, -40.f, 0.f);
    return 1.f - (db / -40.f);          // -40..0 maps to 0..1
}
