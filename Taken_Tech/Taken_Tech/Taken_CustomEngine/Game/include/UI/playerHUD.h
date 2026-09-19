#pragma once
/**
 * @file      playerHUD.h
 * @author    Woh kye Le
 * @co-author Tan Wei Liang Terril, Jethro Sung
 * @email     w.kyele,t.weiliangterril,sung.h
 * @date      2026-02-20
 *
 * @brief     This file defines the PlayerHUD class, which is responsible for
 *            displaying the player's health bar on the screen.
 *
 * @version   1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "Math/vect2.h"
#include "Graphics/mesh2d.h"
#include "Core/componentcontext.h" 


#include "Math/matrix3x3.h"

class Renderer;
class GameApp;
class Camera2D;

/**
 * @struct HUDContext
 * @brief Common rendering context for HUD elements to reduce code duplication.
 */
struct HUDContext {
    Matrix3x3 ortho;
    float guiScale = 1.0f;
    int fbw = 0;
    int fbh = 0;
    Camera2D* prevCam = nullptr;
    bool valid = false;
};

/**
 * @class PlayerHUD
 * @brief A class that handles the player's Heads-Up Display (HUD).
 *
 * Currently, this class manages the health bar, including its background
 * and the dynamic foreground that changes based on the player's health.
 */
class PlayerHUD {
public:
    PlayerHUD() = default;

    /**
     * @brief Prepares the HUD for use.
     *
     * This function creates the necessary 3D meshes (quads) for the health bar
     * and loads the required textures (empty bar and fill bar).
     */
    void Init();

    /**
     * @brief Renders the HUD to the screen.
     *
     * This function calculates the player's current health percentage,
     * updates the health bar's appearance, and draws it on top of the game world.
     *
     * @param renderer The renderer used to draw the meshes.
     * @param app      The main game application, used to access player data.
     */
    void Draw(Renderer& renderer, GameApp& app);

    /**
     * @brief Draws the thinking bubble and the bottom prompt text when the player is near a supported entity.
     * @param renderer The renderer used to draw.
     * @param app The game application used to query player and entity data.
     */
    void DrawInteractionHints(Renderer& renderer, GameApp& app);

    /**
     * @brief Draws the health bar at a specific position.
     * @param renderer The renderer used to draw.
     * @param app      The game application.
     * @param pos      The screen position (pixels from top-left).
     */
    void DrawHealthBar(Renderer& renderer, GameApp& app, Vector2 pos);

    /**
     * @brief Draws the mutation bar at a specific position.
     * @param renderer The renderer used to draw.
     * @param app      The game application.
     * @param pos      The screen position (pixels from top-left).
     */
    void DrawMutationBar(Renderer& renderer, GameApp& app, Vector2 pos);

    /**
     * @brief Draws the ability inventory at a specific position.
     * @param renderer The renderer used to draw.
     * @param app      The game application.
     * @param pos      The screen position (pixels from top-left).
     */
    void DrawAbilityInventory(Renderer& renderer, GameApp& app, Vector2 pos);

    /**
     * @brief Resets the HUD state (e.g., smooth mutation bar) for a new game.
     */
    void Reset();

    /**
     * @brief Sets the texture ID for the health bar's fill (foreground).
     * @param id The OpenGL texture ID to use.
     */
    void SetForegroundTexture(unsigned int id) { fgTextureID = id; }

    /**
     * @brief Sets the texture ID for the health bar's background (frame).
     * @param id The OpenGL texture ID to use.
     */
    void SetBackgroundTexture(unsigned int id) { bgTextureID = id; }

    /**
     * @brief Sets the position of the HUD.
     * @param p The new top-left position.
     */
    void SetPosition(Vector2 p) { position = p; }
    /**
     * @brief Gets the size of the mutation bar.
     * @return Vector2 containing width and height.
     */
    Vector2 GetMutationSize() const { return mutationSize; }

    /**
     * @brief Gets the size of the health bar.
     * @return Vector2 containing width and height.
     */
    Vector2 GetSize() const { return size; }

private:
    /**
     * @brief Prepares common rendering context (ortho matrix, scaling, etc.)
     * @param renderer Reference to the renderer.
     * @return HUDContext containing prepared rendering data.
     */
    HUDContext PrepareHUD(Renderer& renderer);

    Mesh2D quadMesh;          // The mesh used for the background bar
    Mesh2D foregroundMesh;    // The mesh used for the health fill 
    bool initialized = false; //  Tracks if Init() has been called

    unsigned int bgTextureID = 0; // Texture ID for the empty health bar frame
    unsigned int fgTextureID = 0; // Texture ID for the filled health bar

    unsigned int bgMutationTextureID = 0; // Texture ID for the empty mutation bar frame
    unsigned int fgMutationTextureID = 0; // Texture ID for the filled mutation bar

    unsigned int keyBoxTextureID = 0;  // Texture ID for the empty key box
    unsigned int keyItemTextureID = 0; // Texture ID for the key item
    unsigned int abilityInventoryTextureID = 0; // Texture ID for the ability inventory background
    unsigned int burrowIconTextureID = 0;     // Texture ID for burrow ability
    unsigned int healingIconTextureID = 0;    // Texture ID for healing ability
    unsigned int projectileIconTextureID = 0; // Texture ID for projectile ability
    unsigned int thinkingBubbleTextureIDL = 0; // Left variant
    unsigned int thinkingBubbleTextureIDR = 0; // Right variant

    // HUD Layout Configuration
    Vector2 position{ 20.0f, 20.0f };       // Top-left screen position of the HUD
    Vector2 size{ 400.0f, 40.0f };          // Dimensions of the health bar
    Vector2 mutationSize{ 600.0f, 120.0f }; // Dimensions of the Mutation bar
    Vector2 abilityInventorySize{ 80.0f, 80.0f }; // Dimensions of the ability inventory

    float smoothMutationPercent = 0.0f;  // For smooth interpolation of the mutation bar
};
