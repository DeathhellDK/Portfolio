/*
 * @file mapGenerator.h
 * @author   Jethro Sung
 * @email    sung.h
 * @brief Declares procedural labyrinth generation utilities.
 *
 * The MapGenerator namespace produces labyrinth-style maps, enclosed rooms,
 * and special tiles (walls, crates, doors, etc.) based on character grids.
*/
#pragma once
#include "Core/gameApp.h"
#include <string>
#include <vector>
#include <random>
#include <stack>

//Functions for procedural map layout generation.
namespace MapGenerator {

	//@brief Simple record for door placement metadata.
    struct DoorPlacement
    {
		int x = 0; ///< Tile X coordinate
		int y = 0; ///< Tile Y coordinate
        std::string name;         ///< Unique door name in the labyrinth
        std::string targetScene;  ///< Target scene path (empty = inert)
        std::string targetDoor;   ///< Target door name in the destination scene
        DoorArrivalDir dir;       ///< Direction player appears when entering
    };

    /*
     * @brief Generate a map from a 1D string (e.g. "112030002000003330").
     * @param app       GameApp instance to spawn entities into.
     * @param layout    Continuous string of digits representing tiles.
     * @param width     Number of columns in the grid.
     * @param tileSize  World size (in pixels) per tile.
     */
    void FromString(GameApp& app, const std::string& layout, int width, float tileSize);

    /*
     * @brief Generate a map from a multiline grid, easier for visual editing.
     * @param app       GameApp instance to spawn entities into.
     * @param grid      Each string row represents one line of the map.
     * @param tileSize  World size per tile (pixels or world units).
     */
    void FromGrid(GameApp& app, const std::vector<std::string>& grid, float tileSize, const std::string& bakePrefix);

    /*
     * @brief Convert a grid with door markers into a complete game room.
     * @param app GameApp instance to spawn entities into.
     * @param gridWithDoorMarker Grid containing tile characters and door markers.
     * @param tileSize World size per tile in pixels.
     * @param scenePath Path to save the generated room scene.
     */
    void BakeRoomFromGrid(GameApp& app, const std::vector<std::string>& gridWithDoorMarker, float tileSize, const std::string& scenePath);

    /*
     * @brief Remove all entities from a previously baked room.
     * @param app GameApp instance containing the room entities.
     * @param scenePath Path identifier of the room to clear.
     */
    void ClearBakedRoom(GameApp& app, const std::string& scenePath);

    /*
     * @brief Procedurally generate a random maze using DFS backtracking.
     *
     * Creates a fully connected labyrinth by carving passages through a wall grid.
     * Automatically adds doorways and optional crates at random positions.
     *
     * @param app          GameApp instance to spawn entities into.
     * @param worldSizePx  Approximate world width/height in pixels (e.g. 5000).
     * @param tileSize     Size of one tile in pixels (e.g. 64).
     */
    void GenerateMaze(GameApp& app, int worldSizePx, float tileSize);

    /*
     * @brief Generate a rectangular room with procedural content.
     * @param app GameApp instance to spawn entities into.
     * @param scenePath Path to save the generated scene JSON.
     * @param savePath Path to save the grid layout text file.
     * @param roomWidth Width of the room in tiles.
     * @param roomHeight Height of the room in tiles.
     * @param tileSize World size per tile in pixels.
     */
    void GenerateRoom(GameApp& app,
        const std::string& scenePath,   // e.g. "Assets/scene/entities_Level1.json"
        const std::string& savePath,    // e.g. "Assets/maps/room_Level1.txt"
        int roomWidth,
        int roomHeight,
        float tileSize);

    /**
     * @brief Marks a uniquely named enemy as defeated.
     * 
     * This is used to persist enemy death states across room transitions
     * or save/load cycles, ensuring unique bosses/minibosses don't respawn.
     * 
     * @param bakedName The unique identifier for the enemy entity.
     */
    void MarkEnemyDefeatedByName(const std::string& bakedName);

    /**
     * @brief Checks if a specific enemy has already been defeated.
     * 
     * @param bakedName The unique identifier for the enemy entity.
     * @return True if the enemy is marked as defeated.
     */
    bool IsEnemyDefeatedByName(const std::string& bakedName);
    bool IsBossDefeated(int level);

    /**
     * @brief Clears the registry of defeated enemies.
     * 
     * Typically called when starting a new game or resetting the world state.
     */
    void ClearDefeatedEnemies();

    /*void GenerateRoom(GameApp& app, int roomWidth, int roomHeight 
        float tileSize, const std::string& doorName, const std::string& targetScene,
        const std::string& targetDoor);*/
}