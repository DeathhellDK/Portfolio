/**
 * @file     guiSys.cpp
 * @author   Tan Wei Liang Terril
 * @email    t.weiliangterril, jianlin.low, w.kyele
 * @co-author Low JianLin, Woh Kye Le
 * @date     2025-11-7
 *
 * @brief Implements a lightweight in-game GUI button rendering system.
 *
 * This GUI system is entirely custom (no ImGui). Buttons operate directly in
 * pixel / screen-space and are rendered using Mesh2D + camera-less drawing.
 *
 * Features:
 *  - Mouse hover + click detection with Y-flip fix to match rendering coords
 *  - Visual state feedback (default / hover / pressed)
 *  - Click sound through ResourceManager + Audio
 *  - Shared quad mesh for efficient rendering
 *  - Labels rendered using FontRenderer in screen-space
 *
 * Used for gameplay UI such as:
 *  - Pause / Exit button
 *  - Audio mute toggle
 *  - In-game interaction controls
 */
#include "UI/guiSys.h"
#include "Input/input.h"
#include "Graphics/renderer.h"
#include "Math/matrix3x3.h"
#include "Math/vect3.h"
#include "Graphics/vertex2d.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <GLFW/glfw3.h>
#include "Font/FontRenderer.h"
#include "Core/resourceManager.h"
#include "Input/DebugConsole.hpp"

 /*
  * @brief Ctor a GUI button.
  *
  * @param p Top-left position of the button in screen-space (pixels).
  * @param s Button size in pixels.
  * @param text Label displayed on the button.
  * @param click Callback function executed when the button is clicked.
 */
GuiButton::GuiButton(Vector2 p, Vector2 s, const std::string& text, std::function<void()> click, unsigned tex)
    : pos(p), size(s), label(text), onClick(std::move(click)), texture(tex), fontSize(20.f), baseFontSize(20.f) {
}

/*
 * @brief Update the button�s state based on curr mouse input.
 *
 * Detects hover, press, and click states and executes the bound callback.
 * Mouse Y-coordinate is flipped to match render-space convention.
 *
 * @param in Input handler reference for querying mouse position/buttons.
*/
void GuiButton::Update(const eng::Input& in, Vector2 viewportPos, Vector2 viewportSize) {
    if (!visible || !mouseInteractable) return;

    // --- 1) Get TRUE mouse position in WINDOW PIXELS ---
    double mx, my;
    glfwGetCursorPos(glfwGetCurrentContext(), &mx, &my);

    // --- 2) Convert to same coordinate system as your buttons (pixels from bottom-left) ---
    int ww, wh;
    glfwGetWindowSize(glfwGetCurrentContext(), &ww, &wh);

    float px = (float)mx;
    float py = (float)(wh - my);

    // --- 2.1) Viewport mapping if provided ---
    if (viewportPos.x >= 0 && viewportSize.x > 0) {
        // Map mouse from window screen space (top-left origin) to normalized viewport space (0..1)
        float u = (px - viewportPos.x) / viewportSize.x;
        float v = ((float)my - viewportPos.y) / viewportSize.y;

        // Map normalized coordinates back to game screen space (ww x wh)
        // Note: py must be bottom-left origin for the hover test
        px = u * (float)ww;
        py = (1.0f - v) * (float)wh;
    }

    // --- 3) Hover test in pixel-space ---
    hovered =
        (px >= pos.x && px <= pos.x + size.x &&
            py >= pos.y && py <= pos.y + size.y);

    wasHovered = hovered;

    // --- 5) Press logic ---
    if (hovered && in.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        pressed = true;
        if (onClick)
            ResourceManager::PlaySfx("buttonClick", 0.65f);
    }
    else if (in.isMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT))
    {
        if (pressed && hovered)
        {
            if (onClick) onClick();
        }
        pressed = false;
    }
    else if (!in.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
    {
        pressed = false;
    }
}

/*
 * @brief Draw the GUI button as a colored rect.
 *
 * Converts pixel-space coordinates to normalized device coordinates (NDC),
 * applies different colors for hover/press states, and issues a draw call.
 *
 * @param renderer Renderer used to draw the button quad.
 * @param quadMesh ref to a unit quad mesh.
*/
void GuiButton::Draw(Renderer& renderer, const Mesh2D& quadMesh) const {
    if (!visible) return;

    Vector3 color;

    // --- Compute highlight tint ---
    Vector3 tint(1.f, 1.f, 1.f);

    if (pressed)
        tint = Vector3(0.6f, 0.6f, 0.6f);
    else if (hovered)
    {
        float pulse = 0.55f + 0.10f * static_cast<float>(std::sin(glfwGetTime() * 10.0));
        tint = Vector3(pulse, pulse, pulse);
    }

    // --- Button color logic ---
    if (label == "Mute") {
        if (pressed)      color = { 0.1f, 0.3f, 0.7f };
        else if (hovered) color = { 0.3f, 0.5f, 1.0f };
        else              color = { 0.4f, 0.4f, 0.4f };
    }
    else if (label == "Exit") {
        if (pressed)      color = { 0.7f, 0.1f, 0.1f };
        else if (hovered) color = { 1.0f, 0.3f, 0.3f };
        else              color = { 0.4f, 0.4f, 0.4f };
    }
    else {
        if (pressed)      color = { 0.3f, 0.3f, 0.3f };
        else if (hovered) color = { 0.6f, 0.6f, 0.6f };
        else              color = { 0.4f, 0.4f, 0.4f };
    }

    // --- Convert from screen-space (pixels) to NDC ---
    GLFWwindow* win = glfwGetCurrentContext();

    int ww = 1, wh = 1;
    glfwGetWindowSize(win, &ww, &wh);

    int fbw = 1, fbh = 1;
    glfwGetFramebufferSize(win, &fbw, &fbh);

    // Convert pixel to normalized [-1, 1] range 
    float ndcX = (pos.x / (float)ww) * 2.0f - 1.0f;
    float ndcY = (pos.y / (float)wh) * 2.0f - 1.0f;
    float ndcW = (size.x / (float)ww) * 2.0f;
    float ndcH = (size.y / (float)wh) * 2.0f;

    
    Matrix3x3 model =
        Matrix3x3::BuildTranslation(ndcX, ndcY) *
        Matrix3x3::BuildScaling(ndcW, ndcH);

    // --- Draw texture if have  ---
    if (drawQuad) {
        if (texture != 0) {
            renderer.DrawMesh(quadMesh, model, tint, texture);
        }
        else {
            Vector3 baseColor = Vector3(0.4f, 0.4f, 0.4f);
            Vector3 finalColor = {
                baseColor.x * tint.x,
                baseColor.y * tint.y,
                baseColor.z * tint.z
            };
            renderer.DrawMesh(quadMesh, model, finalColor);
        }
    }
    // --- Draw label text ---
  /*  float textX = pos.x + (size.x * 0.35f);
    float textY = pos.y + (size.y * 0.35f);*/

    /* const char* fontForThis =
         (label == "Exit") ? "exit" : "default";*/

         // --- Draw centered label text ---
    float centerX = pos.x + size.x * 0.5f;
    float centerY = pos.y + size.y * 0.5f;

    // Calculate resolution scale based on 1080p height
    float scale = (float)fbh / 1080.f;
    float scaledFontSize = baseFontSize * scale;

    // Adjust for multiline text
    int numLines = (int)std::count(label.begin(), label.end(), '\n') + 1;
    float lineOffset = (numLines - 1) * scaledFontSize * 0.5f;
    float centerY_corrected = centerY - scaledFontSize * 0.35f - lineOffset;

    float wrapWidth = size.x * 0.9f;
    float drawX = centerX - (wrapWidth * 0.5f);

    if (drawLabel && !label.empty()) {
        EngineCore::FontRenderer::DrawText(
            renderer,
            font.c_str(),
            scaledFontSize,
            drawX,
            centerY_corrected,
            0xFFFFFFFF,
            label,
            wrapWidth,
            FontSys::Align::Center
        );
    }
}

/**
 * @brief Constructs a static GUI text element.
 *
 * @param p   Pixel position of the text.
 * @param t   String to render.
 * @param s   Font size.
 * @param col RGBA color for rendering.
 * @param f   Font name used by FontRenderer.
 */
GuiText::GuiText(Vector2 p,
    const std::string& t,
    float s,
    unsigned col,
    const std::string& f)
    : pos(p), text(t), size(s), baseSize(s), color(col), font(f) {
}

/**
 * @brief Constructs a new GUI panel.
 *
 * @param p Top-left pixel position
 * @param s Width/height in pixels
 * @param tex Texture ID (0 = solid color)
 */
GuiPanel::GuiPanel(Vector2 p, Vector2 s, unsigned tex)
    : pos(p), size(s), texture(tex), fontSize(24.f), baseFontSize(24.f) {}

/**
 * @brief Constructs a new GUI panel with text.
 */
GuiPanel::GuiPanel(Vector2 p, Vector2 s, const std::string& text, unsigned tex, float sizeVal, unsigned colorVal, const std::string& f)
    : pos(p), size(s), texture(tex), label(text), fontSize(sizeVal), baseFontSize(sizeVal), textColor(colorVal), font(f) {}

/**
 * @brief Draws the GUI panel.
 *
 * @param renderer Reference to the engine Renderer instance.
 * @param quadMesh Reference to the shared quad mesh.
 */
void GuiPanel::Draw(Renderer& renderer, const Mesh2D& quadMesh) const {
    if (!visible) return;

    // --- Convert from screen-space (pixels) to NDC ---
    GLFWwindow* win = glfwGetCurrentContext();

    // For NDC and cursor interaction, use window size
    int ww = 1, wh = 1;
    glfwGetWindowSize(win, &ww, &wh);

    // For font scaling, use framebuffer size 
    int fbw = 1, fbh = 1;
    glfwGetFramebufferSize(win, &fbw, &fbh);

    // Convert pixel to normalized [-1, 1] range 
    float ndcX = (pos.x / (float)ww) * 2.0f - 1.0f;
    float ndcY = (pos.y / (float)wh) * 2.0f - 1.0f;
    float ndcW = (size.x / (float)ww) * 2.0f;
    float ndcH = (size.y / (float)wh) * 2.0f;

    // Build transformation: Scale then Translate
    Matrix3x3 model =
        Matrix3x3::BuildTranslation(ndcX, ndcY) *
        Matrix3x3::BuildScaling(ndcW, ndcH);

    if (texture != 0) {
        renderer.DrawMesh(quadMesh, model, color, texture);
    } else {
        renderer.DrawMesh(quadMesh, model, color);
    }

    if (!label.empty()) {
        float centerX = pos.x + size.x * 0.5f;
        float centerY = pos.y + size.y * 0.5f;

        // Calculate resolution scale based on 1080p height
        float scale = (float)fbh / 1080.f;
        float scaledFontSize = baseFontSize * scale;

        float ratioX = (float)fbw / (float)ww;
        float ratioY = (float)fbh / (float)wh;

        float wrapWidth = size.x * 0.9f;
        float drawX = centerX - (wrapWidth * 0.5f);

        // Calculate Y-axis offset using actual text bounds
        float wrapWidth_fb = wrapWidth * ratioX;
        auto textGeo = EngineCore::FontRenderer::BuildTextGeometry(font, scaledFontSize, label, wrapWidth_fb, FontSys::Align::Center);
        
        float centerY_corrected = centerY - scaledFontSize * 0.35f; // fallback
        if (!textGeo.verts.empty()) {
            float minY = textGeo.verts[0].y;
            float maxY = textGeo.verts[0].y;
            for (const auto& v : textGeo.verts) {
                minY = std::min(minY, v.y);
                maxY = std::max(maxY, v.y);
            }
            float local_center_y_fb = (minY + maxY) * 0.5f;
           
            centerY_corrected = centerY + (local_center_y_fb / ratioY);
        }

        EngineCore::FontRenderer::DrawText(
            renderer,
            font.c_str(),
            scaledFontSize,
            drawX,
            centerY_corrected,
            textColor,
            label,
            wrapWidth, // Enable text wrapping at 90% of panel width
            FontSys::Align::Center
        );
    }
}

/*
 * @brief Construct the GUI system and create a reusable quad mesh.
 *
 * The quad mesh is defined in [0,1] space and shared by all buttons.
*/
GuiSystem::GuiSystem() {
    // --- Create a unit quad in [0, 1] space (two triangles) ---
    Vector3 white(1.f, 1.f, 1.f);
    std::vector<Vertex2D> verts = {
        Vertex2D({0.f, 0.f}, {0.f, 0.f}, white), // bottom-left
        Vertex2D({1.f, 0.f}, {1.f, 0.f}, white), // bottom-right
        Vertex2D({1.f, 1.f}, {1.f, 1.f}, white), // top-right
        Vertex2D({0.f, 1.f}, {0.f, 1.f}, white)  // top-left
    };

    std::vector<unsigned int> inds = { 0, 1, 2, 0, 2, 3 };

    quadMesh.Reserve(verts.size(), inds.size());
    quadMesh.SetVertices(verts);
    quadMesh.SetIndices(inds);
}

/*
 * @brief Add a new GUI button to the system.
 *
 * @param b Button instance to add (copied into internal storage).
*/
void GuiSystem::AddButton(const GuiButton& b) {
    buttons.push_back(b);
}

/**
 * @brief Adds a text element to the GUI system.
 *
 * @param t GuiText instance to register.
 */
void GuiSystem::AddText(const GuiText& t) {
    texts.push_back(t);
}

/**
 * @brief Adds a panel element to the GUI system.
 *
 * @param p GuiPanel instance to register.
 */
void GuiSystem::AddPanel(const GuiPanel& p) {
    panels.push_back(p);
}

/**
 * @brief Update all buttons managed by the GUI system.
 *
 * @param in Input handler used for button state updates.
*/
void GuiSystem::Update(const eng::Input& in, double dt, Vector2 viewportPos, Vector2 viewportSize) {
    static double lastHoverSfxTime = -1000.0;
    int prevHoverIndex = hoverIndex;

    int mouseHoverIndex = -1;
    for (int i = 0; i < (int)buttons.size(); i++)
    {
        auto& b = buttons[i];
        b.Update(in, viewportPos, viewportSize);
        if (b.visible && b.hovered)
            mouseHoverIndex = i;
    }

    if (mouseHoverIndex != -1)
    {
        hoverIndex = mouseHoverIndex;
        hoverSource = HoverSource::Mouse;
        selectedIndex = mouseHoverIndex;
    }
    else if (hoverSource == HoverSource::Mouse)
    {
        if (gamepadConnected && selectedIndex >= 0 && selectedIndex < (int)buttons.size())
        {
            hoverIndex = selectedIndex;
            hoverSource = HoverSource::Gamepad;
        }
        else
        {
            hoverIndex = -1;
            hoverSource = HoverSource::None;
        }
    }

    UpdateGamepad(in, static_cast<float>(dt));

    int finalHoverIndex = -1;
    if (hoverIndex >= 0 && hoverIndex < (int)buttons.size() && buttons[hoverIndex].visible)
    {
        finalHoverIndex = hoverIndex;
    }
    else if (gamepadConnected && selectedIndex >= 0 && selectedIndex < (int)buttons.size() && buttons[selectedIndex].visible && buttons[selectedIndex].gamepadSelectable)
    {
        finalHoverIndex = selectedIndex;
        hoverIndex = selectedIndex;
        hoverSource = HoverSource::Gamepad;
    }
    else
    {
        hoverIndex = -1;
        hoverSource = HoverSource::None;
    }

    for (int i = 0; i < (int)buttons.size(); i++)
    {
        bool h = (i == finalHoverIndex);
        buttons[i].hovered = h;
        buttons[i].wasHovered = h;
    }

    if (finalHoverIndex != -1 && finalHoverIndex != prevHoverIndex)
    {
        double now = glfwGetTime();
        if (now - lastHoverSfxTime > 0.08)
        {
            ResourceManager::PlaySfx("buttonHover", 0.45f);
            lastHoverSfxTime = now;
        }
    }
}

/*
 * @brief Draw all GUI buttons to the screen.
 *
 * Renders each button using the shared quad mesh.
 *
 * @param renderer Renderer used for drawing.
*/
void GuiSystem::Draw(Renderer& renderer) {
    // Draw panels first (backgrounds)
    for (auto& p : panels)
        p.Draw(renderer, quadMesh);

    for (auto& b : buttons)
        b.Draw(renderer, quadMesh);

    // Draw all GUI text
    int ww, wh;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &ww, &wh);
    float scale = (float)wh / 1080.f;

    for (auto& t : texts) {
        if (!t.visible) continue;

        float scaledSize = t.baseSize * scale;

        EngineCore::FontRenderer::DrawText(
            renderer,
            t.font.c_str(),
            scaledSize,
            t.pos.x,
            t.pos.y,
            t.color,
            t.text,
            0.0f,
            FontSys::Align::Center
        );
    }
}

/**
 * @brief Find a button by its identifier.
 *
 * Searches through the button collection for a button with the specified ID.
 *
 * @param id The unique identifier of the button to find.
 * @return Pointer to the found button, or nullptr if no button with the given ID exists.
 * @note Non-const version allows modification of the returned button.
 */
GuiButton* GuiSystem::FindButton(const std::string& id) {
    for (auto& b : buttons) {
        if (b.id == id)
            return &b;
    }
    return nullptr;
}

/**
 * @brief Find a button by its identifier (const version).
 *
 * Searches through the button collection for a button with the specified ID.
 *
 * @param id The unique identifier of the button to find.
 * @return Const pointer to the found button, or nullptr if no button with the given ID exists.
 * @note Const version provides read-only access to the button.
 */
const GuiButton* GuiSystem::FindButton(const std::string& id) const {
    for (const auto& b : buttons) {
        if (b.id == id)
            return &b;
    }
    return nullptr;
}

/**
 * @brief Find a text element by its identifier.
 *
 * Searches through the text collection for a text element with the specified ID.
 *
 * @param id The unique identifier of the text element to find.
 * @return Pointer to the found text element, or nullptr if no text with the given ID exists.
 * @note Non-const version allows modification of the returned text element.
 */
GuiText* GuiSystem::FindText(const std::string& id) {
    for (auto& t : texts) {
        if (t.id == id)
            return &t;
    }
    return nullptr;
}

/**
 * @brief Find a text element by its identifier (const version).
 *
 * Searches through the text collection for a text element with the specified ID.
 *
 * @param id The unique identifier of the text element to find.
 * @return Const pointer to the found text element, or nullptr if no text with the given ID exists.
 * @note Const version provides read-only access to the text element.
 */
const GuiText* GuiSystem::FindText(const std::string& id) const {
    for (const auto& t : texts) {
        if (t.id == id)
            return &t;
    }
    return nullptr;
}

GuiPanel* GuiSystem::FindPanel(const std::string& id) {
    for (auto& p : panels) {
        if (p.id == id)
            return &p;
    }
    return nullptr;
}

const GuiPanel* GuiSystem::FindPanel(const std::string& id) const {
    for (const auto& p : panels) {
        if (p.id == id)
            return &p;
    }
    return nullptr;
}

void GuiSystem::UpdateGamepad(const eng::Input& in, float dt)
{
    navCooldown -= dt;
    if (navCooldown < 0.0f) navCooldown = 0.0f;

    gamepadConnected = in.isGamepadConnected();
    if (!gamepadConnected)
        return;

    if (buttons.empty()) return;

    auto findVisibleFrom = [&](int start, int dir) -> int
        {
            const int n = (int)buttons.size();
            if (n <= 0) return start;
            int idx = start;
            for (int i = 0; i < n; i++)
            {
                if (idx < 0) idx = n - 1;
                else if (idx >= n) idx = 0;
                if (buttons[idx].visible && buttons[idx].gamepadSelectable) return idx;
                idx += dir;
            }
            return start;
        };

    if (selectedIndex < 0 || selectedIndex >= (int)buttons.size() || !buttons[selectedIndex].visible || !buttons[selectedIndex].gamepadSelectable)
        selectedIndex = findVisibleFrom(0, +1);

    int prevSelectedIndex = selectedIndex;

    bool up = in.isGamepadButtonDown(GLFW_GAMEPAD_BUTTON_DPAD_UP);
    bool down = in.isGamepadButtonDown(GLFW_GAMEPAD_BUTTON_DPAD_DOWN);
    bool left = in.isGamepadButtonDown(GLFW_GAMEPAD_BUTTON_DPAD_LEFT);
    bool right = in.isGamepadButtonDown(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT);

    // -------------------------
    // NAVIGATION
    // -------------------------
    if (navCooldown <= 0.0f)
    {
        bool adjusted = false;
        if (selectedIndex >= 0 && selectedIndex < (int)buttons.size())
        {
            GuiButton& b = buttons[selectedIndex];
            if (b.visible && b.onAdjust)
            {
                if (navMode == NavMode::Vertical)
                {
                    if (left) { b.onAdjust(-1); adjusted = true; }
                    else if (right) { b.onAdjust(+1); adjusted = true; }
                }
                else if (navMode == NavMode::Horizontal)
                {
                    if (up) { b.onAdjust(-1); adjusted = true; }
                    else if (down) { b.onAdjust(+1); adjusted = true; }
                }
            }
        }

        if (adjusted)
        {
            navCooldown = 0.08f;
        }
        else
        if (navMode == NavMode::Vertical)
        {
            if (up)
            {
                selectedIndex = findVisibleFrom(selectedIndex - 1, -1);
                navCooldown = 0.15f;
            }
            else if (down)
            {
                selectedIndex = findVisibleFrom(selectedIndex + 1, +1);
                navCooldown = 0.15f;
            }
        }
        else if (navMode == NavMode::Horizontal)
        {
            if (left)
            {
                selectedIndex = findVisibleFrom(selectedIndex - 1, -1);
                navCooldown = 0.15f;
            }
            else if (right)
            {
                selectedIndex = findVisibleFrom(selectedIndex + 1, +1);
                navCooldown = 0.15f;
            }
        }
    }

    if (selectedIndex < 0)
        selectedIndex = 0;
    if (selectedIndex >= (int)buttons.size())
        selectedIndex = (int)buttons.size() - 1;

    if (selectedIndex != prevSelectedIndex)
    {
        hoverIndex = selectedIndex;
        hoverSource = HoverSource::Gamepad;
    }
    else if (hoverSource != HoverSource::Mouse && hoverIndex == -1)
    {
        hoverIndex = selectedIndex;
        hoverSource = HoverSource::Gamepad;
    }

    // -------------------------
    // CONFIRM
    // -------------------------
    if (in.isGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_A))
    {
        if (selectedIndex >= 0 && selectedIndex < (int)buttons.size())
        {
            if (buttons[selectedIndex].onClick)
            {
                ResourceManager::PlaySfx("buttonClick", 0.65f);
                buttons[selectedIndex].onClick();
            }
        }
    }
}
