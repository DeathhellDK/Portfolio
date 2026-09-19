/**
* @file     miniMap.cpp
* @author   Woh Kye Le
* @email    w.kyele
* @co-author
* @date     2025-11-11
*
* @brief  Implementation of the MinimapHUD class.
* 
* This file handles the visual rendering of the in-game minimap HUD.  
* It draws the circular minimap background, walls (based on the labyrinth grid),
* and the player's position indicator. The minimap acts as a simple 2D
* overview of the level, using normalized coordinates to map world positions
* into minimap space.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "UI/miniMap.h"

#include "Graphics/renderer.h"
#include "Graphics/mesh2d.h"
#include "Math/matrix3x3.h"
#include "Math/vect2.h"
#include "Math/vect3.h"
#include "Core/gameApp.h"
#include "puzzleObject.h"
#include "Core/transform.h"

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <string>  
#include <cmath>

/**
 * @brief Draws the entire minimap, including background, walls, and player marker.
 *
 * The minimap is rendered as a circular overlay at the bottom-right corner
 * of the screen. It first draws a dark circular background, then calls
 * helper functions to draw the level layout (walls) and the player�s
 * current position.
 *
 * @param renderer Reference to the active Renderer instance.
 */
void MinimapHUD::Draw(Renderer& renderer, GameApp& app){
    if (!circleMesh) return; // nothing to draw if no circle mesh

    // Get current framebuffer size for HUD positioning
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0) return;

    const float guiScale = (float)fbh / 1080.0f;
    const float size = 270.0f * guiScale;   
    const float margin = 54.0f * guiScale;  

    // Position minimap at bottom-right corner
    const float xPixels = fbw - size - margin;
    const float yPixels = margin;

    // Create orthographic projection in pixel space
    float l = 0.0f, r = static_cast<float>(fbw);
    float b = 0.0f, t = static_cast<float>(fbh);

    Matrix3x3 ortho = Matrix3x3::Identity();
    ortho(0, 0) = 2.0f / (r - l);
    ortho(1, 1) = 2.0f / (t - b);
    ortho(2, 0) = -(r + l) / (r - l);
    ortho(2, 1) = -(t + b) / (t - b);

    // Build transformation for the minimap background
    Matrix3x3 modelBG =
        Matrix3x3::BuildTranslation(xPixels, yPixels) *
        Matrix3x3::BuildScaling(size, size);

    Matrix3x3 mvp = ortho * modelBG;

    Camera2D* prevCam = renderer.getCamera();
    renderer.setCamera(nullptr); // disable world camera for HUD-space rendering

    // --- Draw minimap background ---
    Vector3 color(0.04f, 0.04f, 0.04f); // dark grey
    renderer.DrawMesh(*circleMesh, mvp, color, 0);

    // --- Draw level and player overlays ---
    DrawWalls(renderer, ortho, xPixels, yPixels, size);
    DrawDoors(renderer, app, ortho, xPixels, yPixels, size);
    DrawPlayer(renderer, ortho, xPixels, yPixels, size);

    renderer.setCamera(prevCam);
}

void MinimapHUD::DrawDoors(Renderer& renderer, GameApp& app, const Matrix3x3& ortho, float xPixels, float yPixels, float size) {
    if (!circleMesh) return;
    if (labyrinthGrid.empty()) return;

    std::string scenePath = app.sceneManager.GetCurrentGameplayScene();
    std::transform(scenePath.begin(), scenePath.end(), scenePath.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    if (scenePath.empty() || scenePath.find("labyrinth.json") == std::string::npos) return;

    auto* pc = app.GetController(GameApp::playerEntity);
    if (!pc) return;

    int H = static_cast<int>(labyrinthGrid.size());
    int W = static_cast<int>(labyrinthGrid[0].size());
    if (W <= 0 || H <= 0) return;

    float tilePixel = size * (labyrinthTileSize / visibleWorldRange);
    float halfTile = tilePixel * 0.5f;
    const float radius2 = 0.25f;

    for (const DoorLink& door : app.GetDoorSystem().GetDoorLinks()) {
        Transform* t = app.GetTransform(door.entity);
        if (!t) continue;

        Vector2 pos = t->GetPosition();
        int gx = static_cast<int>(std::floor(pos.x / labyrinthTileSize));
        int gy = static_cast<int>(std::floor((float)(H - 1) - (pos.y / labyrinthTileSize)));
        if (gx < 0 || gx >= W || gy < 0 || gy >= H) continue;
        if (!discovered.empty() && !discovered[gy][gx]) continue;

        float tx = (gx + 0.5f) * labyrinthTileSize;
        float ty = (H - 1 - gy + 0.5f) * labyrinthTileSize;

        float dx_world = tx - playerWorldPos.x;
        float dy_world = ty - playerWorldPos.y;

        float mx = 0.5f + (dx_world / visibleWorldRange);
        float my = 0.5f + (dy_world / visibleWorldRange);

        float dnx = mx - 0.5f;
        float dny = my - 0.5f;
        if (dnx * dnx + dny * dny > radius2) continue;

        float sx = xPixels + mx * size;
        float sy = yPixels + my * size;

        Matrix3x3 modelD =
            Matrix3x3::BuildTranslation(sx - halfTile, sy - halfTile) *
            Matrix3x3::BuildScaling(tilePixel, tilePixel);

        Matrix3x3 mvpD = ortho * modelD;
        int levelIndex = 1;
        if (!door.targetScene.empty()) {
            std::string targetScene = door.targetScene;
            size_t p = targetScene.rfind("Level");
            if (p != std::string::npos) {
                p += 5;
                size_t q = p;
                while (q < targetScene.size() && std::isdigit((unsigned char)targetScene[q])) ++q;
                if (q > p) {
                    try { levelIndex = std::stoi(targetScene.substr(p, q - p)); }
                    catch (...) {}
                }
            }
        }

        bool unlocked = true;
        if (levelIndex == 2) {
            unlocked = pc->HasKeyType(PlayerAbility::PROJECTILE);
        }
        else if (levelIndex == 3) {
            unlocked = pc->HasKeyType(PlayerAbility::BURROW);
        }
        else if (levelIndex == 4) {
            unlocked = pc->HasKeyType(PlayerAbility::BURROW) &&
                pc->HasKeyType(PlayerAbility::HEAL) &&
                pc->HasKeyType(PlayerAbility::PROJECTILE);
        }

        Vector3 doorColor = unlocked ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f);
        renderer.DrawMesh(*circleMesh, mvpD, doorColor, 0);
    }
}

/**
 * @brief Sets the labyrinth grid data used by the minimap and resets discovery.
 *
 * This is usually called when a new level is loaded. It gives the minimap
 * a copy of the labyrinth's tile layout and the tile size in world units.
 * It also clears any previous "discovered" state so fog-of-war starts fresh.
 *
 * @param grid      Text-based labyrinth layout (each string is one row of tiles).
 * @param tileSize  Size of one tile in world units (must match the map generator).
 */
void MinimapHUD::SetLabyrinth(const std::vector<std::string>& grid, float tileSize) {
    labyrinthGrid = grid;
    labyrinthTileSize = tileSize;

    // Start with everything undiscovered. Tiles will be revealed as the player moves.
    int H = static_cast<int>(labyrinthGrid.size());
    int W = H > 0 ? static_cast<int>(labyrinthGrid[0].size()) : 0;
    discovered.assign(H, std::vector<bool>(W, false));
}


/**
 * @brief Updates the player's world position for the minimap and refreshes discovery.
 *
 * This should be called every frame with the player's current world-space position.
 * The minimap uses this to:
 *  - Place the red player marker
 *  - Reveal tiles in a radius around the player (fog-of-war update)
 *
 * @param pos Player position in world coordinates (same space as the main game).
 */
void MinimapHUD::SetPlayerWorldPos(const Vector2& pos) {
    playerWorldPos = pos;
    hasPlayerPos = true;
    UpdateDiscovery(); // reveals tiles around player 
}

/**
 * @brief Draws all the visible walls from the labyrinth grid onto the minimap.
 *
 * Each wall cell ('1' or '#') in the grid is rendered as a small white dot
 * within the circular minimap area. Coordinates are normalized from grid
 * indices into [0,1] minimap space, then converted into pixel space.
 *
 * @param renderer Reference to the Renderer.
 * @param ortho    The precomputed orthographic projection matrix.
 * @param xPixels  X offset of the minimap (in screen pixels).
 * @param yPixels  Y offset of the minimap (in screen pixels).
 * @param size     Diameter of the minimap in pixels.
 */
void MinimapHUD::DrawWalls(Renderer& renderer, const Matrix3x3& ortho, float xPixels, float yPixels, float size){
    if (!circleMesh) return;
    if (labyrinthGrid.empty()) return;

    int H = static_cast<int>(labyrinthGrid.size());
    int W = static_cast<int>(labyrinthGrid[0].size());
    if (W <= 0 || H <= 0) return;

    bool useDiscovery = !discovered.empty() &&
        static_cast<int>(discovered.size()) == H &&
        static_cast<int>(discovered[0].size()) == W;

    float tilePixel = size * (labyrinthTileSize / visibleWorldRange);
    float halfTile = tilePixel * 0.5f;
    const float radius2 = 0.25f; // circular clipping radius squared (relative to 1x1)

    for (int gy = 0; gy < H; ++gy) {
        const std::string& row = labyrinthGrid[gy];
        for (int gx = 0; gx < static_cast<int>(row.size()); ++gx) {
            char c = row[gx];

            // Only draw walls or solid tiles
            if (c != '1' && c != '3' && c != 'P')
                continue;

            // Skip undiscovered tiles (fog-of-war)
            if (useDiscovery && !discovered[gy][gx])
                continue;

            // World position of tile center
            float tx = (gx + 0.5f) * labyrinthTileSize;
            float ty = (H - 1 - gy + 0.5f) * labyrinthTileSize;

            // Offset from player
            float dx_world = tx - playerWorldPos.x;
            float dy_world = ty - playerWorldPos.y;

            // Normalized position in minimap [0..1]
            float mx = 0.5f + (dx_world / visibleWorldRange);
            float my = 0.5f + (dy_world / visibleWorldRange);

            // Skip tiles outside the circular area
            float dnx = mx - 0.5f;
            float dny = my - 0.5f;
            if (dnx * dnx + dny * dny > radius2)
                continue;

            // Convert normalized to pixel coordinates within minimap
            float sx = xPixels + mx * size;
            float sy = yPixels + my * size;

            Matrix3x3 modelDot =
                Matrix3x3::BuildTranslation(sx - halfTile, sy - halfTile) *
                Matrix3x3::BuildScaling(tilePixel, tilePixel);

            Matrix3x3 mvpDot = ortho * modelDot;
            Vector3 wallColor(1.0f, 1.0f, 1.0f); // white
            renderer.DrawMesh(*circleMesh, mvpDot, wallColor, 0);
        }
    }
}

/**
 * @brief Draws the player�s position marker on the minimap.
 *
 * The player is displayed as a small red dot whose position is derived
 * from the player�s world coordinates. The world coordinates are first
 * normalized against the labyrinth�s dimensions, then mapped into the
 * minimap�s coordinate space.
 *
 * @param renderer Reference to the Renderer.
 * @param ortho    The orthographic projection matrix for screen-space.
 * @param xPixels  X offset of the minimap in screen pixels.
 * @param yPixels  Y offset of the minimap in screen pixels.
 * @param size     Diameter of the minimap in pixels.
 */
void MinimapHUD::DrawPlayer(Renderer& renderer, const Matrix3x3& ortho, float xPixels, float yPixels, float size){
    if (!circleMesh) return;
    if (!hasPlayerPos) return;

    // Player is always at the center in this scrolling mode
    float mx = 0.5f;
    float my = 0.5f;

    float sx = xPixels + mx * size;
    float sy = yPixels + my * size;

    // Size relative to the visible world range
    float tilePixel = size * (labyrinthTileSize / visibleWorldRange);
    float playerSize = tilePixel * 1.6f;

    Matrix3x3 modelP =
        Matrix3x3::BuildTranslation(sx - playerSize * 0.5f, sy - playerSize * 0.5f) *
        Matrix3x3::BuildScaling(playerSize, playerSize);

    Matrix3x3 mvpP = ortho * modelP;
    Vector3 playerColor(1.0f, 1.0f, 0.0f); // yellow
    renderer.DrawMesh(*circleMesh, mvpP, playerColor, 0);
}

/**
 * @brief Updates the fog-of-war discovery mask around the player.
 *
 * This function marks tiles as "discovered" if they fall within a certain
 * radius of the player's current position. Once a tile is discovered, it
 * stays revealed for the rest of the level.
 *
 */
void MinimapHUD::UpdateDiscovery() {
    if (!hasPlayerPos)          return;
    if (labyrinthGrid.empty())  return;
    if (discoverRadiusWorld <= 0.0f) return;

    int H = static_cast<int>(labyrinthGrid.size());
    int W = static_cast<int>(labyrinthGrid[0].size());
    if (W <= 0 || H <= 0) return;

    // Make sure the discovery mask has the same dimensions as the grid.
    if (discovered.size() != static_cast<size_t>(H) ||  
        (!discovered.empty() &&
            discovered[0].size() != static_cast<size_t>(W))) {
        discovered.assign(H, std::vector<bool>(W, false));
    }

    // Player position in "tile space" (world units divided by tile size).
    float pxTile = playerWorldPos.x / labyrinthTileSize; // Roughly 0..W
    float pyTile = playerWorldPos.y / labyrinthTileSize; // Roughly 0..H

    // Convert player world position into grid index space.
    // In the grid, row 0 is the top, which corresponds to the highest world Y.
    float playerGridXF = pxTile;
    float playerGridYF = static_cast<float>(H - 1) - pyTile; // Flip Y to line up with the grid.

    // Convert the discovery radius from world units into tiles.
    float rTiles = discoverRadiusWorld / labyrinthTileSize;
    float rTilesSq = rTiles * rTiles;
    int   rInt = static_cast<int>(std::ceil(rTiles));

    int centerGX = static_cast<int>(std::floor(playerGridXF));
    int centerGY = static_cast<int>(std::floor(playerGridYF));

    int minGX = std::max(0, centerGX - rInt);
    int maxGX = std::min(W - 1, centerGX + rInt);
    int minGY = std::max(0, centerGY - rInt);
    int maxGY = std::min(H - 1, centerGY + rInt);

    for (int gy = minGY; gy <= maxGY; ++gy) {
        for (int gx = minGX; gx <= maxGX; ++gx) {
            // Compute the tile's center in "world-tile" space.
            float cx = gx + 0.5f;              // X is direct.
            float cy = (H - 1 - gy) + 0.5f;     // Y is flipped to match the way we spawned and draw tiles.

            float dx = cx - pxTile;
            float dy = cy - pyTile;
            float dist2 = dx * dx + dy * dy;

            // If the tile center falls inside the radius, mark it as discovered.
            if (dist2 <= rTilesSq) {
                discovered[gy][gx] = true; // Once discovered, it stays revealed.
            }
        }
    }
}
