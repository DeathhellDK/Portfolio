#pragma once
/**
 * @file      confirmDialog.h
 * @author    Sng Swee Yong Dillon
 * @email     sweeyongdillon.sng
 * @date      2026-01-10
 *
 * @brief Declares ConfirmDialog, a small modal UI component for yes/no confirmation.
 *
 * ConfirmDialog is a reusable "modal" overlay that:
 *  - blocks interaction with underlying UI while visible
 *  - displays a message and two buttons (Yes/No)
 *  - executes callbacks depending on user choice
 *
 * SceneManager or any other controller can trigger it without owning UI creation logic.
 * 
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include <functional>
#include <string>
#include "Graphics/mesh2d.h"

class Renderer;
class GuiSystem;
namespace eng { class Input; }


struct ConfirmDialogLayout {
    // Panel (relative to window)
    Vector2 panelRelSize{ 0.42f, 0.22f };

    // Colors (packed RGBA: 0xAARRGGBB)
    unsigned panelColor = 0xFF262626; // dark gray
    unsigned dimColor = 0x99000000;   // optional fullscreen dim

    // Text (position relative inside panel: 0..1)
    Vector2 messageRelPos{ 0.50f, 0.62f };
    std::string messageFont = "default";
    float messageFontSize = 30.f;
    unsigned messageColor = 0xFFFFFFFF;

    // Buttons (relative inside panel)
    Vector2 buttonRelSize{ 0.25f, 0.26f };
    Vector2 yesRelPos{ 0.18f, 0.18f };
    Vector2 noRelPos{ 0.57f, 0.18f };

    // Button text/style
    std::string buttonFont = "default";
    float buttonFontSize = 22.f;
    unsigned buttonTextColor = 0xFFFFFFFF;

    // Optional button texture
    std::string buttonTexture = "menu_button"; 

    // widget ID
    std::string textId      = "Confirm.Text";
    std::string yesButtonId = "Confirm.Yes";
    std::string noButtonId  = "Confirm.No";
};



class ConfirmDialog{
public:
    /**
     * @brief Builds and registers the dialog's widgets into a GuiSystem.
     *
     * This should be called once during initialization. Widgets are created as
     * hidden by default, and are only shown when Show() is called.
     *
     * @param gui GuiSystem that stores the dialog text/buttons.
     */
    void Build(GuiSystem& gui);

    /**
     * @brief Opens the dialog with a message and callbacks.
     *
     * Makes the dialog visible and sets the current confirm/cancel actions.
     * While visible, the dialog should be updated/drawn by the controller.
     *
     * @param message Text shown in the dialog.
     * @param onYes   Callback executed when the user confirms.
     * @param onNo    Callback executed when the user cancels.
     */
    void Show(const std::string& message,
        std::function<void()> onYes,
        std::function<void()> onNo);

    /**
     * @brief Hides the dialog and clears callbacks.
     */
    void Hide();

    /**
     * @brief Returns whether the dialog is currently visible.
     */
    bool IsVisible() const { return visible; }

    bool LoadLayout(const std::string& path, GuiSystem& gui);
    bool SaveLayout(const std::string& path, const GuiSystem& gui) const;
    void ReloadTextures(GuiSystem& gui);

    /**
     * @brief Updates the dialog input and handles modal interactions.
     *
     * - Routes input only to the dialog widgets (modal behavior).
     * - Supports ESC to cancel (same as pressing No).
     *
     * @param in Input state for the current frame.
     * @param gui GuiSystem containing the dialog widgets.
     */
    void Update(const eng::Input& in, double dt, GuiSystem& gui, Vector2 viewportPos = { -1, -1 }, Vector2 viewportSize = { -1, -1 });

    /**
     * @brief Draws the dialog overlay and positions widgets based on window size.
     *
     * The dialog computes a centered panel rectangle and updates widget
     * positions/sizes accordingly. It then draws the GuiSystem.
     *
     * @param renderer Renderer used for screen-space rendering.
     * @param gui      GuiSystem holding the dialog widgets.
     * @param ww       Window width in pixels.
     * @param wh       Window height in pixels.
     */
    void Draw(Renderer& renderer, GuiSystem& gui, int ww, int wh) const;

private:
    void SetWidgetsVisible(GuiSystem& gui, bool v) const;
    void Layout(GuiSystem& gui, int ww, int wh) const;

    bool visible = false;
    std::string msg = "Confirm?";

    ConfirmDialogLayout layout;

    std::function<void()> onYes;
    std::function<void()> onNo;

    Mesh2D panelQuad;     // unit quad (0..1)
    bool panelBuilt = false;
    
};
