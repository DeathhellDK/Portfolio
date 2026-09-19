#pragma once 
/**
* @file     miniMap.h
* @author   Woh Kye Le
* @email    w.kyele
* @co-author
* @date     2025-11-11
*
* @brief
* This header file contains all helper function declaration to create a minimap 
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Math/vect2.h"   
#include "Math/matrix3x3.h"
#include "Math/vect3.h"   
#include <vector>

class Renderer;
class Camera2D;
class Mesh2D;

class MinimapHUD {
public:
    MinimapHUD() = default;

    /**
     * @brief Sets the main camera reference used for rendering calculations.
     * @param cam Pointer to the Camera2D instance.
     */
    void SetCamera(Camera2D* cam) { mainCamera = cam; }

    /**
     * @brief Sets the mesh used for rendering the minimap.
     * @param mesh Pointer to the Mesh2D instance (usually a circle).
     */
    void SetMesh(const Mesh2D* mesh) { circleMesh = mesh; }

    /**
     * @brief Sets the discovery radius for the fog of war.
     * @param r Radius in world units that the player can reveal on the minimap.
     */
    void SetDiscoveryRadius(float r) { discoverRadiusWorld = r; }

    /**
     * @brief Sets how much of the world is visible within the minimap circle.
     * @param r Range in world units.
     */
    void SetVisibleRange(float r) { visibleWorldRange = r; }

    /**
     * @brief Renders the minimap to the screen.
     * @param renderer Reference to the Renderer.
     * @param app Reference to the GameApp context.
     */
    void Draw(Renderer& renderer, class GameApp& app);

    /**
     * @brief Sets the labyrinth grid data to be displayed on the minimap.
     * @param grid 2D vector of strings representing the labyrinth layout.
     * @param tileSize Size of each tile in world units.
     */
    void SetLabyrinth(const std::vector<std::string>& grid, float tileSize);

    /**
     * @brief Updates the player's world position for minimap tracking.
     * @param pos The player's current world coordinates.
     */
    void SetPlayerWorldPos(const Vector2& pos);

private:
    Camera2D* mainCamera = nullptr;      
    const Mesh2D* circleMesh = nullptr;  

    float sizeRatio = 0.25f;    // diameter relative to screen height
    float marginRatio = 0.05f;  // offset relative to screen height

    // --- map data ---
    std::vector<std::string> labyrinthGrid;
    float labyrinthTileSize = 100.0f; // default, but overwritten by SetLabyrinth

    // --- fog of war ---
    std::vector<std::vector<bool>> discovered; // True if this tile has ever been within the player's reveal radius
    float discoverRadiusWorld = 400.0f;        // in world units
    float visibleWorldRange = 2000.0f;         // how much of the world is shown in the minimap circle

    // --- player data---
    Vector2 playerWorldPos{ 0.f, 0.f };
    bool hasPlayerPos = false;

    // --- object drawing within the minimap ---
    void DrawWalls(Renderer& renderer, const Matrix3x3& ortho, float xPixels, float yPixels, float size);
    void DrawPlayer(Renderer& renderer, const Matrix3x3& ortho, float xPixels, float yPixels, float size);
    void DrawDoors(Renderer& renderer, class GameApp& app, const Matrix3x3& ortho, float xPixels, float yPixels, float size);

    // --- mark tiles within radius as discovered ---
    void UpdateDiscovery();
};