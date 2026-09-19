#pragma once
/**
 * @file     guiSys.h
 * @author   Tan Wei Liang Terril
 * @email    t.weiliangterril, jianlin.low, w.kyele
 * @co-author Low JianLin, Woh Kye Le
 * @date     2025-11-07
 * @brief Basic in-game GUI system rendered using the engine's graphics manager.
 *
 *
 * Provides custom 2D clickable UI elements powered by the engine’s Renderer,
 * without using ImGui. UI elements are rendered as textured quads and respond
 * to input events (hover and click) broadcast through the game Input system.
 *
 * Primary Capabilities:
 * - Screen-space GUI quads (buttons)
 * - Event-driven click handling
 * - Managed update & draw pipeline separate from ECS renderinghover and clicks.
 */

#include "Math/vect2.h"
#include "Math/vect3.h"
#include "Graphics/mesh2d.h"
#include "Math/matrix3x3.h"
#include <string>
#include <functional>
#include <vector>

class Renderer;     // from engine
namespace eng { class Input; }

enum class NavMode
{
    Vertical,
    Horizontal,
    Grid
};


/**
    * @class GuiButton
    * @brief Represents a rectangular click-sensitive UI button.
    *
    * Features:
    * - Hover detection and pressed state feedback
    * - Click callback invoked on mouse release inside button bounds
    *
    * State Rules:
    * - hovered: true if cursor is inside rectangle
    * - pressed: true while LMB is held down inside rectangle
    *
*/
class GuiButton {
public:
    std::string id;       // Unique stable ID
    Vector2 pos;          // Top-left position of the button
    Vector2 size;         // Dim of  button

    std::string label;    // Txt displayed on the button

    std::string font = "default";
    float fontSize = 20.f;
    float baseFontSize = 20.f; 
    unsigned textColor = 0xFFFFFFFF;

    std::function<void()> onClick; // Callback invoked when clicked
    std::function<void(int)> onAdjust;
    unsigned texture = 0; // 0 = no texture

    int hoverCooldownFrames = 0;
    bool hovered = false; ///< True if mouse is over the button
    bool wasHovered = false;

    bool pressed = false; ///< True if button is being pressed
    bool visible = true;
    bool mouseInteractable = true;
    bool gamepadSelectable = true;
    bool drawQuad = true;
    bool drawLabel = true;

    /**
     * @brief Constructs a new GUI button.
     *
     * @param p        Top-left pixel position
     * @param s        Width/height in pixels
     * @param text     Display label text
     * @param click    Callback invoked when button is pressed and released
     */
    GuiButton(Vector2 p, Vector2 s, const std::string& text, std::function<void()> click, unsigned tex = 0);

    /**
     * @brief Updates hover/press states and handles click detection.
     * @param in Reference to the engine input system.
     */
    void Update(const eng::Input& in, Vector2 viewportPos = { -1, -1 }, Vector2 viewportSize = { -1, -1 });

    /**
     * @brief Draws the button using the engine's Renderer.
     * @param renderer Reference to the engine Renderer instance.
     */
    void Draw(Renderer& renderer, const Mesh2D& quadMesh) const;
};

/**
 * @class GuiText
 * @brief Represents a non-interactive screen-space text string (title, label, etc.)
 *
 * GuiText is used for static menu labels (e.g., "HOW TO PLAY", "OPTIONS", "PAUSED").
 * It is rendered directly by GuiSystem::Draw()
 */
class GuiText {
public:
    std::string id;
    Vector2 pos;
    std::string text;
    float size = 24.f;
    float baseSize = 24.f; 
    unsigned color = 0xFFFFFFFF;
    std::string font = "default";

    bool visible = true;

    /**
    * @brief Construct a GuiText element.
    *
    * @param p     Pixel position of the text (top-left anchor).
    * @param t     Text content.
    * @param s     Font size (default = 24.f).
    * @param col   Text color (default = white).
    * @param f     Font name (default = "default").
    */
    GuiText(Vector2 p, const std::string& t, float s = 24.f, unsigned col = 0xFFFFFFFF, const std::string& f = "default");
};

/**
 * @class GuiPanel
 * @brief Represents a non-interactive static image/panel.
 */
class GuiPanel {
public:
    std::string id;
    Vector2 pos;
    Vector2 size;
    unsigned texture = 0;
    Vector3 color = { 1.f, 1.f, 1.f }; // Tint color
    bool visible = true;

    // --- Added Label Fields ---
    std::string label;
    std::string font = "default";
    float fontSize = 24.f;
    float baseFontSize = 24.f; 
    unsigned textColor = 0xFFFFFFFF;

    GuiPanel(Vector2 p, Vector2 s, unsigned tex = 0);
    GuiPanel(Vector2 p, Vector2 s, const std::string& text, unsigned tex = 0, float size = 24.f, unsigned color = 0xFFFFFFFF, const std::string& f = "default");

    void Draw(Renderer& renderer, const Mesh2D& quadMesh) const;
};

/**
    * @class GuiSystem
    * @brief Central manager for all screen-space GUI elements.
    *
    * Allows games to register multiple GuiButton elements, then handles their
    * update/draw sequencing each frame. Rendering uses a single quad mesh shared
    * across all buttons to minimize draw overhead.
    *
    * Update Flow:
    * - Poll input for button state changes
    * - Invoke callbacks on press-release events
    * - Maintain UI interactive ordering (simple stack behavior)
*/
class GuiSystem {
private:
    std::vector<GuiButton> buttons; // Active GUI buttons
    std::vector<GuiText> texts;     // text fonts
    std::vector<GuiPanel> panels;   // Static panels
    Mesh2D quadMesh; // Shared mesh for all GUI quads
    bool gamepadConnected = false;
    enum class HoverSource { None, Mouse, Gamepad };
    int hoverIndex = -1;
    HoverSource hoverSource = HoverSource::None;

    void UpdateGamepad(const eng::Input& in, float dt);
public:
    GuiSystem();

    void AddButton(const GuiButton& b);
    void AddText(const GuiText& t);
    void AddPanel(const GuiPanel& p);
    void Update(const eng::Input& in, double dt, Vector2 viewportPos = { -1, -1 }, Vector2 viewportSize = { -1, -1 });
    void Draw(Renderer& renderer);
    const Mesh2D& GetQuadMesh() const { return quadMesh; }
    std::vector<GuiButton>& GetButtons() { return buttons; }
    std::vector<GuiText>& GetTexts() { return texts; }
    std::vector<GuiPanel>& GetPanels() { return panels; }
    const std::vector<GuiButton>& GetButtons() const { return buttons; }
    const std::vector<GuiText>& GetTexts() const { return texts; }
    const std::vector<GuiPanel>& GetPanels() const { return panels; }

    // Finders (key for save/load)
    GuiButton* FindButton(const std::string& id);
    const GuiButton* FindButton(const std::string& id) const;

    GuiText* FindText(const std::string& id);
    const GuiText* FindText(const std::string& id) const;

    GuiPanel* FindPanel(const std::string& id);
    const GuiPanel* FindPanel(const std::string& id) const;

    int selectedIndex = 0;
    float navCooldown = 0.0f;

    NavMode navMode = NavMode::Vertical;
};
