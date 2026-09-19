/*
 * @file     mapGenerator.cpp
 * @author   Jethro Sung
 * @email    sung.h,t.weiliangterril
 * @co-author Tan Wei Liang Terril
 * @date     2025-11-7
 * @brief Implements procedural map and room generation logic.
 * The generator builds maps from character grids ('1'=wall, '0'=open, '2'=door, '#'=room interior)
 * and converts them into batched render meshes, colliders, and door links to other scenes.
 *
 * Conventions:
 * - Tiles: '1'=wall, '0'=open, '2'=door, '#'=room interior, '=' etc for special.
 * - Positions are in tile space; world space = tile * tileSize.
 * - Transforms use TOP-LEFT origin; Colliders store FULL size (see collisionSystem).
 *
 * Major stages:
 * 1) Grid init & random walk carve.
 * 2) DFS branches & sparseness control.
 * 3) Room injection with padding and deterministic door.
 * 4) Connectivity fixups (ensure reachability).
 * 5) Bake to render: batched wall mesh + merged colliders.
 * 6) Door linking to external scenes (entities_LevelN.json).
 *
 * Key systems:
 * - Algorithmic layer: Handles random walk carving, room injection, and connectivity.
 * - Engine integration: Binds results to ECS entities, Mesh2D batching, and collision systems.
 * - Scene linkage: Registers doors to other levels via GameApp::RegisterDoor.
*/
#include "World/mapGenerator.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <array>
#include <utility>
#include <cstdint>
#include <algorithm>
#include <queue>
#include <cctype>
#include <unordered_set>
#include "Core/gameApp.h"
#include "Graphics/mesh2d.h"
#include "factories.h"
#include "Scripting/scriptcomponent.h"   
#include "Core/assetsPath.h"
#include "Editor/EditorShared.h" 
#include <json.hpp>


namespace
{
    using json = nlohmann::json;

    /**
     * @brief Generates a unique 64-bit key from two 32-bit integers (X, Y).
     * 
     * Used for map lookups where the coordinate pair needs to be a single key.
     * 
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return Combined 64-bit key.
     */
    static inline uint64_t KeyXY(int x, int y)
    {
        return (uint64_t(uint32_t(x)) << 32) | uint32_t(y);
    }

    /**
     * @struct PuzzleTileOverride
     * @brief Configuration data for special puzzle tiles loaded from JSON.
     * 
     * This struct holds properties for interactive elements like levers, doors,
     * collapsing floors, and memory triggers that are defined in external files.
     */
    struct PuzzleTileOverride
    {
        // For generator
        int groupId = 1;                ///< ID grouping related puzzle elements.
        int requiredHits = 1;           ///< Number of hits required to activate.
        bool requiresProjectile = true; ///< True if only projectiles trigger it.

        // For lever
        bool isLever = false;           ///< True if this tile is a lever.
        bool toggle = true;             ///< True if the lever toggles state.

        // For door
        bool isDoor = false;            ///< True if this tile is a door.
        bool manualDoor = false;        ///< True if it's a manual door (no puzzle logic).
        std::array<int, 4> requiredGroups{};  ///< List of group IDs required to open.
        int requiredGroupCount = 0;     ///< Number of required groups.

        // For Healing Memory
        int memoryId = 0;               ///< ID of the memory to reveal.
        int requiredMemoryId = -1;      ///< Prerequisite memory ID (-1 for none).
        std::string narrative = "A soldier's last memory..."; ///< Text to display.
        std::string mapPath = "";       ///< Path to the memory image.
        float healRadius = 100.0f;      ///< Radius for activation.
        float memoryDuration = 8.0f;    ///< Duration of the memory overlay.

        // For Collapsing Floor
        float collapseDelay = 0.8f;     ///< Time before floor collapses.
        float respawnDelay = 3.0f;      ///< Time before floor respawns.
        bool ignoreWhenBurrowed = true; ///< True if burrowed players don't trigger it.
        bool collapseOnce = false;      ///< True if it stays collapsed.
        bool createFallHazard = true;   ///< True if collapsing creates a pit.
    };

    /**
     * @brief Loads puzzle configuration overrides from a JSON file.
     * 
     * Expects a JSON file with the same base name as the grid text file,
     * suffixed with "_puzzles.json".
     * 
     * @param gridTxtPath Path to the grid layout text file.
     * @return Map of coordinate keys to puzzle configurations.
     */
    static std::unordered_map<uint64_t, PuzzleTileOverride>
        LoadPuzzleOverridesForGrid(const std::string& gridTxtPath)
    {
        std::unordered_map<uint64_t, PuzzleTileOverride> out;

        // Replace ".txt" -> "_puzzles.json"
        std::filesystem::path p(gridTxtPath);
        std::filesystem::path jsonPath = p;
        jsonPath.replace_extension(); // removes ".txt"
        jsonPath += "_puzzles.json";

        if (!std::filesystem::exists(jsonPath))
            return out;

        std::ifstream f(jsonPath.string());
        if (!f.is_open())
            return out;

        json j;
        try
        {
            f >> j;
        }
        catch (...)
        {
            return out;
        }

        if (!j.is_array())
            return out;

        for (const auto& item : j)
        {
            if (!item.is_object())
                continue;

            const int x = item.value("x", -1);
            const int y = item.value("y", -1);
            if (x < 0 || y < 0)
                continue;

            PuzzleTileOverride o;

            const std::string kind = item.value("kind", "");
            o.isDoor = (kind == "door");
            o.isLever = (kind == "lever");

            // Shared
            o.groupId = item.value("groupId", 1);

            if (o.isLever)
            {
                o.toggle = item.value("toggle", true);
            }
            else if (kind == "memory")
            {
                o.memoryId = item.value("memoryId", (x + y * 100));
                o.requiredMemoryId = item.value("requiredMemoryId", -1);
                o.narrative = item.value("narrative", "A soldier's last memory...");
                o.mapPath = item.value("mapPath", "");
                o.healRadius = item.value("healRadius", 100.0f);
                o.memoryDuration = item.value("duration", 8.0f);
            }
            else if (kind == "floor")
            {
                o.collapseDelay = item.value("collapseDelay", 0.8f);
                o.respawnDelay = item.value("respawnDelay", 3.0f);
                o.ignoreWhenBurrowed = item.value("ignoreWhenBurrowed", true);
                o.collapseOnce = item.value("collapseOnce", false);
                o.createFallHazard = item.value("fallHazard", true);
            }
            else if (!o.isDoor)
            {
                // Generator config
                o.requiredHits = item.value("requiredHits", 1);
                o.requiresProjectile = item.value("requiresProjectile", true);
            }
            else
            {
                // Door config
                o.manualDoor = item.value("manual", false);

                if (item.contains("requiredGroups") && item["requiredGroups"].is_array())
                {
                    auto arr = item["requiredGroups"];
                    int n = (int)arr.size();
                    if (n > 4) n = 4;

                    o.requiredGroupCount = n;
                    for (int i = 0; i < n; ++i)
                        o.requiredGroups[i] = arr[i].get<int>();
                }
                else
                {
                    // Legacy door: uses only groupId
                    o.requiredGroupCount = 0;
                }
            }

            out[KeyXY(x, y)] = o;
        }

        return out;
    }
}

namespace {
    static std::unordered_set<std::string> gDefeatedEnemyNames;
}

    static std::string SceneStemFromPath(const std::string& scenePath){
    std::filesystem::path p(scenePath);
    return p.stem().string(); // entities_Level2
}

    // Door Metadata Helpers
    struct DoorMeta {
        int x, y;
        int levelIndex;
    };

    static void SaveDoorMetadata(const std::string& path, const std::vector<DoorMeta>& doors) {
        std::ofstream out(path);
        if (out.is_open()) {
            for (const auto& d : doors) {
                out << d.x << " " << d.y << " " << d.levelIndex << "\n";
            }
        }
    }

    static std::vector<DoorMeta> LoadDoorMetadata(const std::string& path) {
        std::vector<DoorMeta> doors;
        std::ifstream in(path);
        if (in.is_open()) {
            int x, y, lvl;
            while (in >> x >> y >> lvl) {
                doors.push_back({ x, y, lvl });
            }
        }
        return doors;
    }

static std::string RoomBakePrefix(const std::string& scenePath){
    return "RoomBake_" + SceneStemFromPath(scenePath) + "_";
}

static int ExtractLevelIndexFromPath(const std::string& path) {
    std::string needle = "entities_Level";
    size_t p = path.rfind(needle);
    if (p == std::string::npos) return 1;
    p += needle.size();
    size_t q = p;
    while (q < path.size() && std::isdigit((unsigned char)path[q])) ++q;
    if (q > p) {
        try { return std::stoi(path.substr(p, q - p)); }
        catch (...) {}
    }
    return 1;
}

static bool StartsWith(const std::string& s, const std::string& prefix){
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

static inline char CellAt(const std::vector<std::string>& g, int x, int y, char oob = '.') {
    if (y < 0 || y >= (int)g.size()) return oob;
    const auto& row = g[y];
    if (x < 0 || x >= (int)row.size()) return oob;
    return row[x];
}

static void GenerateLabyrinthWallVariants(
    const std::vector<std::string>& grid,
    int gridHeight,
    const std::vector<VariantPlacement>& existingVariants,
    std::vector<VariantPlacement>& outVariants,
    bool onlyProtectedWalls = false)
{
    int H = (int)grid.size();
    int W = H ? (int)grid[0].size() : 0;

    // Track existing variants to avoid duplicates
    std::vector<std::vector<bool>> hasVariant(H, std::vector<bool>(W, false));
    auto markVariant = [&](const VariantPlacement& v) {
        if (v.gx >= 0 && v.gx < W) {
            int gy = (gridHeight - 1) - v.gy; // Convert back to grid Y
            if (gy >= 0 && gy < H) hasVariant[gy][v.gx] = true;
        }
    };

    for (const auto& v : existingVariants) markVariant(v);
    // fyi outVariants might already contain items if we are appending, but typically we treat them as 'new'
    // If outVariants contains items that should be respected, mark them too.
    for (const auto& v : outVariants) markVariant(v);

    // 1. Identify "Solid" tiles (Walls '1', Room Interior '#', Door '2')
    // Objects (3, 4, 6, etc.) are only solid if they are NOT adjacent to open space ('0').
    // This prevents random crates in corridors from forming false "Room Chunks",
    // while ensuring crates inside rooms maintain the room's solidity.
    auto isSolid = [&](int x, int y) {
        if (x < 0 || x >= W || y < 0 || y >= H) return false;
        char c = grid[y][x];

        // Always Solid (Structural)
        if (c == '1' || c == '#' || c == '2' || c == 'R') return true;

        // Always Empty (Open Space)
        if (c == '0' || c == '.' || c == ' ') return false;

        // Objects: Solid only if enclosed (no contact with '0')
        const int n[4][2] = { {0,1}, {0,-1}, {1,0}, {-1,0} };
        for (const auto& d : n) {
            int nx = x + d[0];
            int ny = y + d[1];
            if (nx >= 0 && nx < W && ny >= 0 && ny < H) {
                char nc = grid[ny][nx];
                if (nc == '0' || nc == '.' || nc == ' ') return false;
            }
        }
        return true;
    };

    // 2. (Removed) Chunk detection is no longer needed.
    // We now process ALL structural tiles to unify the variant look.

    // 3. Generate variants
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // Process only Structural tiles for variants
            char c = grid[y][x];
            if (c != '1' && c != '#' && c != 'R') continue;
            
            if (onlyProtectedWalls && c != 'R' && c != '#') continue;

            if (hasVariant[y][x]) continue;
            // Doors are skipped (they spawn their own prefab)
            if (c == '2') continue; 

            // Helper: Valid neighbor for connectivity
            // We connect to other Structural tiles or Doors.
            // This ensures all walls merge together, and walls frame doors correctly.
            auto isValidNeighbor = [&](int nx, int ny) {
                if (nx < 0 || nx >= W || ny < 0 || ny >= H) return false;
                char nc = grid[ny][nx];
                
                // If we are only processing protected walls (Rooms),
                // we should NOT connect to standard corridor walls ('1').
                // We ONLY connect to other Room parts ('R', '#') or Doors ('2').
                if (onlyProtectedWalls) {
                    if (nc == 'R' || nc == '#' || nc == '2') return true;
                    return false;
                }

                // Structural: Wall, Room, Protected Wall, Door
                if (nc == '1' || nc == '#' || nc == 'R' || nc == '2') return true;
                return false;
            };

            // Check if Interior (all 4 orthogonal neighbors are Valid)
            bool n_valid = isValidNeighbor(x, y - 1);
            bool s_valid = isValidNeighbor(x, y + 1);
            bool w_valid = isValidNeighbor(x - 1, y);
            bool e_valid = isValidNeighbor(x + 1, y);

            // Boundary -> Wall with 9-slice
            // Determine texture based on Valid neighbors (Chunks or Doors)
            // This ignores standard wall branches, keeping the 9-slice clean.
            
            bool n = n_valid;
            bool s = s_valid;
            bool w = w_valid;
            bool e = e_valid;

            std::string tex = "wall"; // Default (fully surrounded) -> single wall
            bool isFloor = false;

            if (n && s && w && e) {
                // Interior -> Floor
                tex = "Floor_Tile";
                isFloor = true;
            }
            else if (!n && !w) tex = "wall_TL";
            else if (!n && !e) tex = "wall_TR";
            else if (!s && !w) tex = "wall_BL_1";
            else if (!s && !e) tex = "wall_BR_1";
            else if (!n) tex = "wall_B&T";
            else if (!s) tex = "wall_B&T";
            else if (!w) tex = "wall_L_2";
            else if (!e) tex = "wall_R_2";
            
            VariantPlacement vp{};
            vp.gx = x;
            vp.gy = (gridHeight - 1) - y;
            vp.type = "WallTile";
            vp.textureKey = tex;
            vp.solid = !isFloor;
            if (isFloor) {
                vp.name = "AutoChunkFloor_" + std::to_string(x) + "_" + std::to_string(vp.gy);
            } else {
                vp.name = "AutoChunkWall_" + std::to_string(x) + "_" + std::to_string(vp.gy);
            }

            outVariants.push_back(vp);
            hasVariant[y][x] = true;
        }
    }
}

static void MaskVariantWallsOutOfBakeGrid(
    std::vector<std::string>& gridForBake,
    const std::string& scenePath,
    int gridHeight) {
    std::filesystem::path p(scenePath);
    const std::string stem = p.stem().string();
    const std::string variantPath = AssetPath("variants/" + stem + "_variants.json");

    std::vector<VariantPlacement> variants;
    if (!Variant::LoadFile(variantPath, gridHeight, variants))
        return;

    for (const auto& v : variants) {
        // Mask out any variant that should replace a baked wall tile
        const bool masksBaseWall =
            (v.type == "WallTile") ||
            (v.type == "DoorLock") ||
            (v.type == "BurrowWall") ||
            (v.type == "PuzzleGenerator") ||
            (v.type == "Lever") ||
            (v.type == "healingMemory") ||
            (v.type == "MutationHealer");

        bool shouldMask = masksBaseWall && !v.removed;
        if (v.type == "WallTile" && v.removed) {
            shouldMask = true; // explicitly remove the base wall if a WallTile was removed
        }

        if (!shouldMask)
            continue;

        const int row = (gridHeight - 1) - v.gy;
        if (row < 0 || row >= (int)gridForBake.size()) continue;
        if (v.gx < 0 || v.gx >= (int)gridForBake[row].size()) continue;

     
        char& cell = gridForBake[row][v.gx];

        // Remove the baked wall so the placed prefab is what appears there.
        if (cell == '1' || cell == '#')
        {
            cell = '0';
        }
    }
}

/**
 * @brief Syncs the grid with variant wall placements for the minimap.
 *
 * This function ensures that the minimap grid reflects all manual wall
 * additions and removals made via the variant system.
 */
static void SyncGridWithVariants(std::vector<std::string>& grid, const std::string& scenePath, int gridHeight) {
    std::filesystem::path p(scenePath);
    const std::string stem = p.stem().string();
    const std::string variantPath = AssetPath("variants/" + stem + "_variants.json");

    std::vector<VariantPlacement> variants;
    if (!Variant::LoadFile(variantPath, gridHeight, variants))
        return;

    for (const auto& v : variants) {
        const bool isWallLike =
            (v.type == "WallTile") ||
            (v.type == "BurrowWall") ||
            (v.type == "DoorLock") ||
            (v.type == "Crate") ||
            (v.type == "PuzzleGenerator") ||
            (v.type == "Lever") ||
            (v.type == "healingMemory") ||
            (v.type == "MutationHealer");

        if (!isWallLike) {
            // DoorLock is handled separately or ignored for now
            if (v.type == "DoorLock") {
                const int row = (gridHeight - 1) - v.gy;
                if (row >= 0 && row < (int)grid.size() && v.gx >= 0 && v.gx < (int)grid[row].size()) {
                    grid[row][v.gx] = '0'; // Treat as floor on minimap for now
                }
            }
            continue;
        }

        const int row = (gridHeight - 1) - v.gy;
        if (row < 0 || row >= (int)grid.size()) continue;
        if (v.gx < 0 || v.gx >= (int)grid[row].size()) continue;

        if (v.removed || !v.solid) {
            grid[row][v.gx] = '0';
        }
        else {
            grid[row][v.gx] = '1';
        }
    }
}

static DoorArrivalDir ComputeDoorDirFromGrid(const std::vector<std::string>& g, int x, int y){
    int H = (int)g.size();
    int W = H ? (int)g[0].size() : 0;

    auto inB = [&](int ix, int iy) { return ix >= 0 && iy >= 0 && ix < W && iy < H; };
    auto isCorridor = [&](int ix, int iy) {
        if (!inB(ix, iy)) return false;
        return g[iy][ix] == '0'; // keep strict; you can expand later
        };

    // Pick direction pointing toward corridor tile
    if (isCorridor(x, y - 1)) return DoorArrivalDir::Top;
    if (isCorridor(x, y + 1)) return DoorArrivalDir::Bottom;
    if (isCorridor(x - 1, y)) return DoorArrivalDir::Left;
    if (isCorridor(x + 1, y)) return DoorArrivalDir::Right;

    return DoorArrivalDir::Right; // fallback
}

/*
     * @brief Spawns a functional doorway entity and registers it in the door system.
     *
     * This function instantiates the "Doorway" prefab at the given world position,
     * retrieves its collider and transform components, and registers a corresponding
     * DoorLink with the global DoorSystem. The resulting link enables
     * inter-scene teleportation when the player collides with this door.
     *
     * @param app          Reference to the active GameApp instance.
     * @param pos          World position where the doorway will be spawned.
     * @param baseName     Unique name identifying this door in the current scene
     *                     (e.g., "DoorToRoom1").
     * @param targetScene  Path to the JSON scene file that this door leads to
     *                     (e.g., "Assets/scene/entities_Level3.json").
     * @param targetDoor   Name of the destination door within the target scene
     *                     that the player will appear at (commonly "ExitToLabyrinth").
     * @param dir          Direction from which the player will arrive when returning
     *                     through this door (Top, Bottom, Left, Right).
     * @param index        Numerical index of the door (used for naming or debug logging).
     *
     * - Prefab System: Uses GameApp::InstantiatePrefab("Doorway", pos) to spawn
     *   the physical entity and attach its components.
     * - ECS Components: Immediately fetches the collider and transform components
     *   to verify correct prefab configuration.
     * - Door Registration: Constructs a DoorLink struct and registers it with
     *   GameApp::GetDoorSystem().RegisterDoor(link) so the teleport system can
     *   recognize and manage transitions.
     *
     * WARNING:
     * If the `"Doorway"` prefab lacks a collider or transform, a warning is printed
     * and the door is still spawned visually but will not function as a transition trigger.
 */

static Entity SpawnDoor(GameApp& app, const Vector2& pos,
    const std::string& baseName,
    const std::string& targetScene,
    const std::string& targetDoor,
    DoorArrivalDir dir,
    [[maybe_unused]] int index,
    const std::string& entityNameForCleanup)   
{
    Entity door = app.InstantiatePrefab("Doorway", pos);

    // IMPORTANT: name the entity so ClearBakedRoom can find it
    if (!entityNameForCleanup.empty())
        app.SetEntityName(door, entityNameForCleanup);

    // --- Texture override based on level index ---
    int levelIndex = 0;
    // Extract level number from targetScene (e.g. "entities_Level3.json")
    // or from doorName (e.g. "DoorToRoom3")
    {
        std::string s = targetScene;
        size_t p = s.rfind("entities_Level");
        if (p != std::string::npos) {
            p += 14; // length of "entities_Level"
            size_t q = p;
            while (q < s.size() && std::isdigit((unsigned char)s[q])) ++q;
            if (q > p) try { levelIndex = std::stoi(s.substr(p, q - p)); } catch (...) {}
        }
        
        // If not found in targetScene, try baseName (e.g. "DoorToRoom3")
        if (levelIndex == 0) {
            s = baseName;
            p = s.rfind("DoorToRoom");
            if (p != std::string::npos) {
                p += 10;
                size_t q = p;
                while (q < s.size() && std::isdigit((unsigned char)s[q])) ++q;
                if (q > p) try { levelIndex = std::stoi(s.substr(p, q - p)); } catch (...) {}
            }
        }
    }

    if (levelIndex > 0) {
        std::string texKey = "door_lvl" + std::to_string(levelIndex);
        GLuint texID = ResourceManager::GetTexture(texKey);
        // Fallback to "door" if specific level texture is missing
        if (texID == 0) texID = ResourceManager::GetTexture("door");
        
        if (texID != 0) {
            if (auto* mr = app.GetRenderer(door)) {
                mr->SetTexture(texID);
            }
        }
    }

    auto* c = app.GetCollider(door);
    auto* t = app.GetTransform(door);

    if (!c || !t) {
        DebugConsole::Get().Warning("[MapGenerator] Doorway prefab missing collider/transform at spawn.\n");
    }
    else {
        DoorLink link;
        link.entity = door;
        link.name = baseName;          // keep as "ExitToLabyrinth"
        link.targetScene = targetScene;
        link.targetDoor = targetDoor;
        link.arrivalDir = dir;

        app.GetDoorSystem().RegisterDoor(link);
    }

    return door;
}


static void FixIsolatedChambers(std::vector<std::string>& g, int minChamberArea = 16)
{
    int H = (int)g.size();
    int W = (int)g[0].size();
    if (H <= 3 || W <= 3) return;

    const int D4[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };

    auto inB = [&](int x, int y) {
        return x > 0 && y > 0 && x < W - 1 && y < H - 1;
        };
    auto isOpen = [&](int x, int y) {
        return g[y][x] == '0';
        };

    // Mark tiles that already belong to some processed chamber
    std::vector<std::vector<uint8_t>> chamberMark(H, std::vector<uint8_t>(W, 0));

    for (int gy = 1; gy < H - 1; ++gy)
    {
        for (int gx = 1; gx < W - 1; ++gx)
        {
            if (!isOpen(gx, gy)) continue;             // gate must be open (corridor tile)

            // Collect open neighbors of this potential gate
            std::vector<std::pair<int, int>> neighbors;
            for (int k = 0; k < 4; ++k)
            {
                int nx = gx + D4[k][0];
                int ny = gy + D4[k][1];
                if (inB(nx, ny) && isOpen(nx, ny))
                    neighbors.push_back({ nx, ny });
            }
            if ((int)neighbors.size() < 2)
                continue; // cannot be a bottleneck if it only touches 0 or 1 open tiles

            // For each neighbor, see if there is a large region that becomes
            // isolated when (gx,gy) is treated as blocked.
            for (auto start : neighbors)
            {
                int sx = start.first;
                int sy = start.second;
                if (chamberMark[sy][sx]) continue; // already handled via some other gate

                // BFS with the gate treated as a wall: this finds the "side" behind the gate.
                std::queue<std::pair<int, int>> q;
                std::vector<std::vector<uint8_t>> vis(H, std::vector<uint8_t>(W, 0));
                std::vector<std::pair<int, int>> cluster;

                q.push({ sx, sy });
                vis[sy][sx] = 1;
                cluster.push_back({ sx, sy });

                bool touchesOtherGateNeighbor = false;

                while (!q.empty())
                {
                    auto [cx, cy] = q.front(); q.pop();

                    // If any cell (other than start) is also orth-adjacent to (gx,gy),
                    // then this region has another way to see the gate -> not single-entrance.
                    if (!(cx == sx && cy == sy))
                    {
                        for (int k = 0; k < 4; ++k)
                        {
                            int ax = cx + D4[k][0];
                            int ay = cy + D4[k][1];
                            if (ax == gx && ay == gy)
                            {
                                touchesOtherGateNeighbor = true;
                                break;
                            }
                        }
                    }

                    for (int k = 0; k < 4; ++k)
                    {
                        int nx = cx + D4[k][0];
                        int ny = cy + D4[k][1];
                        if (!inB(nx, ny)) continue;
                        if (nx == gx && ny == gy) continue;   // cannot go through the gate
                        if (!isOpen(nx, ny)) continue;
                        if (vis[ny][nx]) continue;
                        vis[ny][nx] = 1;
                        q.push({ nx, ny });
                        cluster.push_back({ nx, ny });
                    }
                }

                // Mark cluster so we don't repeat work from another neighbor.
                for (auto& c : cluster)
                    chamberMark[c.second][c.first] = 1;

                int area = (int)cluster.size();

                // Not a problematic chamber:
                // - too small, or
                // - it can "see" the gate from multiple tiles (not a single 1-tile neck).
                if (area < minChamberArea || touchesOtherGateNeighbor)
                    continue;

                // We have found a big region that is only accessible via (gx,gy).
                // Try to carve a second exit from this cluster to anywhere outside it.
                bool exitMade = false;

                auto isInCluster = [&](int x, int y) {
                    return x >= 0 && y >= 0 && x < W && y < H && vis[y][x];
                    };

                for (auto& cell : cluster)
                {
                    int cx = cell.first;
                    int cy = cell.second;

                    for (int k = 0; k < 4 && !exitMade; ++k)
                    {
                        int wx = cx + D4[k][0];
                        int wy = cy + D4[k][1];
                        if (!inB(wx, wy)) continue;

                        // Only carve through plain walls; never touch rooms/doors/protected.
                        if (g[wy][wx] != '1') continue;

                        int tx = wx;
                        int ty = wy;

                        // March through contiguous walls
                        while (inB(tx, ty) && g[ty][tx] == '1')
                        {
                            tx += D4[k][0];
                            ty += D4[k][1];
                        }
                        if (!inB(tx, ty)) continue;
                        if (!isOpen(tx, ty)) continue;
                        if (isInCluster(tx, ty)) continue; // must connect OUTSIDE the chamber

                        // Carve the corridor
                        tx = wx;
                        ty = wy;
                        while (inB(tx, ty) && g[ty][tx] == '1')
                        {
                            g[ty][tx] = '0';
                            tx += D4[k][0];
                            ty += D4[k][1];
                        }

                        exitMade = true;
                    }

                    if (exitMade) break;
                }

                // If we couldn't make a second exit, promote the gate to a door.
                if (!exitMade)
                {
                    /*if (isOpen(gx, gy))
                        g[gy][gx] = '2';*/
                }
            }
        }
    }
}

/*
    * @brief Create an enclosed rectangular room with walls, floor, and a door.
    *
    * This function modifies the grid directly:
    * - Surrounds an area with wall tiles ('1').
    * - Fills the interior with black floor ('#').
    * - Adds a single centered door ('2') along one wall.
    * - Opens one adjacent tile outside the door for accessibility.
    *
    * @param app Reference to the game instance.
    * @param grid Character grid representing the map.
    * @param startX Left boundary tile of the room.
    * @param startY Top boundary tile of the room.
    * @param roomW Room width (in tiles).
    * @param roomH Room height (in tiles).
    * @param tileSize Size of one tile (in world units).
    * @param doorName Name of the door for linking.
    * @param targetScene Target scene file (e.g., entities_LevelN.json).
    * @param targetDoor Name of door in target scene to return through.
*/
static bool AddEnclosedRoom([[maybe_unused]] GameApp& app,
    std::vector<std::string>& grid,
    int startX, int startY,
    int roomW, int roomH,
    [[maybe_unused]] float tileSize,
    [[maybe_unused]] const std::string& doorName,
    [[maybe_unused]] const std::string& targetScene,
    [[maybe_unused]] const std::string& targetDoor,
    int& outDoorX, int& outDoorY,
    [[maybe_unused]] std::vector<VariantPlacement>* outVariants)
{
    outDoorX = -1;
    outDoorY = -1;

    const int mazeH = static_cast<int>(grid.size());
    const int mazeW = mazeH ? static_cast<int>(grid[0].size()) : 0;
    if (mazeW <= 0 || mazeH <= 0) return false;
    if (roomW < 3 || roomH < 3) return false;

    // --- Clamp room inside map ---
    startX = std::clamp(startX, 2, mazeW - roomW - 3);
    startY = std::clamp(startY, 2, mazeH - roomH - 3);

    // --- Step 1: Check for overlap or adjacency (1-tile padding) ---
    bool occupied = false;
    for (int y = startY - 1; y <= startY + roomH; ++y) {
        for (int x = startX - 1; x <= startX + roomW; ++x) {
            if (y < 0 || y >= mazeH || x < 0 || x >= mazeW) continue;

            bool onRing = (y == startY - 1) || (y == startY + roomH) ||
                (x == startX - 1) || (x == startX + roomW);

            if (onRing) {
                char tile = grid[y][x];
                if (tile == '#' || tile == '2') { // forbid collisions w/ rooms & doors
                    occupied = true;
                    break;
                }
            }
        }
        if (occupied) break;
    }

    if (occupied) {
        DebugConsole::Get().Info(
            "[MapGenerator] Room prevented at (" + std::to_string(startX) + "," +
            std::to_string(startY) + ") due to adjacency.\n");
        return false;
    }

    // --- Step 2: Draw walls and interior ---
    for (int y = startY; y < startY + roomH; ++y) {
        for (int x = startX; x < startX + roomW; ++x) {
            bool border = (x == startX || x == startX + roomW - 1 ||
                y == startY || y == startY + roomH - 1);
            grid[y][x] = border ? 'R' : '#'; // protected walls + interior
        }
    }

    struct Side { int dx, dy; };
    const Side trySides[4] = {
        { 0,-1 }, // top
        { 0, 1 }, // bottom
        {-1, 0 }, // left
        { 1, 0 }  // right
    };

    auto inB = [&](int x, int y) {
        return x > 0 && y > 0 && x < mazeW - 1 && y < mazeH - 1;
        };

    auto tryDoorOnWall = [&](int wx0, int wy0, int wx1, int wy1, Side s, int& doorX, int& doorY,
        int& outOx, int& outOy) -> bool
        {
            // Center point along that wall segment
            int cx = (wx0 + wx1) / 2;
            int cy = (wy0 + wy1) / 2;

            // outside cell (one step out)
            int ox = cx + s.dx;
            int oy = cy + s.dy;

            // outside cell 2 (two steps out) - avoid carving into another room interior
            int ox2 = cx + s.dx * 2;
            int oy2 = cy + s.dy * 2;

            if (!inB(ox, oy)) return false;
            if (!inB(ox2, oy2)) return false;

            // Must punch into a wall tile and not immediately into room interior
            if (grid[oy][ox] != '1') return false;
            if (grid[oy2][ox2] == '#') return false;

            doorX = cx;
            doorY = cy;
            outOx = ox;
            outOy = oy;
            return true;
        };

    int doorX = -1, doorY = -1;
    int ox = -1, oy = -1;

    // Try placing the door at the center of one of the room walls
    bool placed =
        // top wall
        tryDoorOnWall(startX, startY, startX + roomW - 1, startY, { 0,-1 }, doorX, doorY, ox, oy) ||
        // bottom wall
        tryDoorOnWall(startX, startY + roomH - 1, startX + roomW - 1, startY + roomH - 1, { 0, 1 }, doorX, doorY, ox, oy) ||
        // left wall
        tryDoorOnWall(startX, startY, startX, startY + roomH - 1, { -1,0 }, doorX, doorY, ox, oy) ||
        // right wall
        tryDoorOnWall(startX + roomW - 1, startY, startX + roomW - 1, startY + roomH - 1, { 1, 0 }, doorX, doorY, ox, oy);

    if (placed)
    {
        grid[doorY][doorX] = '2';

        if (inB(ox, oy) && grid[oy][ox] == '1')
            grid[oy][ox] = '0';

        int ox2 = ox + (ox - doorX);
        int oy2 = oy + (oy - doorY);
        if (inB(ox2, oy2) && grid[oy2][ox2] == '1')
            grid[oy2][ox2] = '0';

        outDoorX = doorX;
        outDoorY = doorY;

        /*
         * Variants are now handled by GenerateLabyrinthWallVariants (Step 5.5) globally.
         * This ensures consistent 9-slice and interior handling for both rooms and random chunks.
         */

        return true;
    }

    {
        int cx = startX + roomW / 2;
        int cy = startY + roomH - 1;

        grid[cy][cx] = '2';

        int outX = cx;
        int outY = cy + 1;

        if (inB(outX, outY) && grid[outY][outX] == '1')
            grid[outY][outX] = '0';

        int outY2 = cy + 2;
        if (inB(outX, outY2) && grid[outY2][outX] == '1')
            grid[outY2][outX] = '0';

        outDoorX = cx;
        outDoorY = cy;

        /*
         * Variants are now handled by GenerateLabyrinthWallVariants (Step 5.5) globally.
         */

        return true;
    }
}


/*
    * @brief Locate a safe spawn point for the player within open tiles.
    *
    * Scans the grid for open cells surrounded by at least one layer of free space.
    * Converts chosen tile position to world coordinates.
    *
    * @param grid The maze grid.
    * @param tileSize Tile-to-world scale.
    * @return Vector2 Safe world-space spawn position.
*/
static Vector2 FindSafeSpawn(const std::vector<std::string>& grid, float tileSize)
{
    const int H = (int)grid.size();
    const int W = (int)grid[0].size();
    if (H <= 2 || W <= 2) return { tileSize, tileSize };

    const int D4[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };

    // 1) Label connected components of '0'
    std::vector<std::vector<int> > comp(H, std::vector<int>(W, -1));
    std::vector<int> compSize;
    int cid = 0;

    for (int y = 1; y < H - 1; ++y)
    {
        for (int x = 1; x < W - 1; ++x)
        {
            if (grid[y][x] != '0' || comp[y][x] != -1)
                continue;

            std::queue<std::pair<int, int> > q;
            q.push(std::make_pair(x, y));
            comp[y][x] = cid;
            int sz = 0;

            while (!q.empty())
            {
                std::pair<int, int> cur = q.front(); q.pop();
                int cx = cur.first;
                int cy = cur.second;
                ++sz;

                for (int k = 0; k < 4; ++k)
                {
                    int nx = cx + D4[k][0];
                    int ny = cy + D4[k][1];
                    if (nx <= 0 || ny <= 0 || nx >= W - 1 || ny >= H - 1)
                        continue;
                    if (grid[ny][nx] != '0' || comp[ny][nx] != -1)
                        continue;
                    comp[ny][nx] = cid;
                    q.push(std::make_pair(nx, ny));
                }
            }

            compSize.push_back(sz);
            ++cid;
        }
    }

    if (cid == 0) {
        // No open tiles (shouldn't happen, but be safe)
        return { tileSize, tileSize };
    }

    // 2) Choose main region as the largest
    int mainId = 0;
    for (int i = 1; i < cid; ++i)
        if (compSize[i] > compSize[mainId])
            mainId = i;

    auto isNiceSpawn = [&](int x, int y) -> bool
        {
            if (grid[y][x] != '0') return false;
            if (comp[y][x] != mainId) return false;

            int deg = 0;
            bool nearDoor = false;
            bool nearBad = false;

            for (int k = 0; k < 4; ++k)
            {
                int nx = x + D4[k][0];
                int ny = y + D4[k][1];
                char c = grid[ny][nx];

                if (c == '0') ++deg;
                if (c == '2') nearDoor = true;
                if (c == '3' || c == '4' || c == 'C' || c == 'B' || c == 'H') nearBad = true;
            }

            if (nearDoor) return false;      // don’t spawn on a door step
            if (nearBad) return false;       // keep spawn slightly clean
            if (deg < 2) return false;       // avoid cul-de-sac

            return true;
        };

    // 3) Prefer a "nice" spawn in main region
    for (int y = 1; y < H - 1; ++y)
        for (int x = 1; x < W - 1; ++x)
            if (isNiceSpawn(x, y)) {
                Vector2 pos((float)x * tileSize,
                    (float)(H - 1 - y) * tileSize);
                DebugConsole::Get().Info("[MapGenerator] Safe spawn(main,good) at (" + std::to_string(x) + "," + std::to_string(y) + ")\n");
                return pos;
            }

    // 4) Fallback: any tile in main region
    for (int y = 1; y < H - 1; ++y)
        for (int x = 1; x < W - 1; ++x)
            if (comp[y][x] == mainId && grid[y][x] == '0') {
                Vector2 pos((float)x * tileSize,
                    (float)(H - 1 - y) * tileSize);
                DebugConsole::Get().Info("[MapGenerator] Safe spawn(main,fallback) at (" + std::to_string(x) + "," + std::to_string(y) + ")\n");
                return pos;
            }

    // 5) Last resort: original fallback
    return { tileSize, tileSize };
}

/*
    * @brief Connect disconnected open areas to maintain global reachability.
    *
    * Finds isolated regions of walkable space ('0') and connects them using minimal
    * straight corridors. Ensures every room and corridor belongs to one connected component.
    *
    * @param grid Character grid to modify.
*/
static void ConnectAllRegions(std::vector<std::string>& grid)
{
    int height = (int)grid.size();
    int width = (int)grid[0].size();
    const int dirs[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

    // --- 1. Label connected components of '0' tiles ---
    std::vector<std::vector<int>> regionId(height, std::vector<int>(width, -1));
    std::vector<std::vector<std::pair<int, int>>> regions;
    int currentRegion = 0;

    for (int sy = 1; sy < height - 1; ++sy)
    {
        for (int sx = 1; sx < width - 1; ++sx)
        {
            if (grid[sy][sx] != '0' || regionId[sy][sx] != -1)
                continue;

            std::queue<std::pair<int, int>> q;
            q.push({ sx, sy });
            regionId[sy][sx] = currentRegion;
            regions.emplace_back();

            while (!q.empty())
            {
                auto [x, y] = q.front(); q.pop();
                regions.back().push_back({ x, y });

                for (auto& d : dirs)
                {
                    int nx = x + d[0];
                    int ny = y + d[1];
                    if (nx > 0 && ny > 0 && nx < width - 1 && ny < height - 1 &&
                        grid[ny][nx] == '0' && regionId[ny][nx] == -1)
                    {
                        regionId[ny][nx] = currentRegion;
                        q.push({ nx, ny });
                    }
                }
            }

            ++currentRegion;
        }
    }

    if (regions.empty())
        return;

    DebugConsole::Get().Info("[MapGenerator] Connecting " + std::to_string(regions.size()) + " regions.\n");

    // --- 2. Pick the largest region as the main region ---
    int mainRegion = 0;
    std::size_t mainSize = regions[0].size();
    for (int i = 1; i < (int)regions.size(); ++i)
    {
        if (regions[i].size() > mainSize)
        {
            mainSize = regions[i].size();
            mainRegion = i;
        }
    }

    auto inBounds = [&](int x, int y) {
        return x > 0 && y > 0 && x < width - 1 && y < height - 1;
        };

    auto isBlocked = [&](char c) {
        // Treat room interior / protected walls as solid.
        // We deliberately do NOT treat '1' as blocked, so BFS can tunnel through walls.
        return c == '#' || c == 'R';
        };

    // --- 3. For each smaller region, BFS to the main region and carve the path ---
    for (int i = 0; i < (int)regions.size(); ++i)
    {
        if (i == mainRegion) continue;

        std::queue<std::pair<int, int>> q;
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
        std::vector<std::vector<std::pair<int, int>>> parent(
            height,
            std::vector<std::pair<int, int>>(width, { -1, -1 })
        );

        // Start BFS from ALL tiles in region i
        for (const auto& [sx, sy] : regions[i])
        {
            q.push({ sx, sy });
            visited[sy][sx] = true;
        }

        bool found = false;
        std::pair<int, int> target = { -1, -1 };

        while (!q.empty() && !found)
        {
            auto [x, y] = q.front(); q.pop();

            for (auto& d : dirs)
            {
                int nx = x + d[0];
                int ny = y + d[1];
                if (!inBounds(nx, ny) || visited[ny][nx])
                    continue;

                char c = grid[ny][nx];
                if (isBlocked(c))
                    continue; // cannot walk or carve through rooms

                visited[ny][nx] = true;
                parent[ny][nx] = { x, y };

                // If we reached a '0' that belongs to the main region,
                // we found a connection point.
                if (grid[ny][nx] == '0' && regionId[ny][nx] == mainRegion)
                {
                    found = true;
                    target = { nx, ny };
                    break;
                }

                q.push({ nx, ny });
            }
        }

        if (!found)
        {
            DebugConsole::Get().Warning("[MapGenerator] BFS failed to find path from region "
                + std::to_string(i) + " to main region.\n");
            continue;
        }

        // --- Carve walls along the BFS path back from target ---
        int cx = target.first;
        int cy = target.second;

        while (parent[cy][cx].first != -1)
        {
            // Only convert solid walls to floor; keep doors and rooms intact.
            if (grid[cy][cx] == '1')
                grid[cy][cx] = '0';

            auto [px, py] = parent[cy][cx];
            cx = px;
            cy = py;
        }

        DebugConsole::Get().Info("[MapGenerator] Connected region " + std::to_string(i) + " to main region via BFS.\n");
    }
}

static void FixDiagonalCorners(std::vector<std::string>& g) {
    int H = (int)g.size(), W = (int)g[0].size();
    auto open = [&](int x, int y) { return x > 0 && y > 0 && x < W - 1 && y < H - 1 && g[y][x] == '0'; };
    auto wall = [&](int x, int y) { return x > 0 && y > 0 && x < W - 1 && y < H - 1 && g[y][x] == '1'; };
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x) {
        if (open(x, y) && open(x + 1, y + 1) && wall(x + 1, y) && wall(x, y + 1)) g[y][x + 1] = '0'; // bridge
        if (open(x + 1, y) && open(x, y + 1) && wall(x, y) && wall(x + 1, y + 1)) g[y + 1][x] = '0'; // bridge
    }
}

/*
    * @brief Ensure that all doors ('2') are reachable from open tiles ('0').
    *
    * This verification runs a flood-fill (BFS/DFS) from open corridors. Any door
    * not connected is re-linked via minimal corridor carving.
    *
    * @param grid Character grid representing current map state.
*/
static void EnsureDoorAccessibility(std::vector<std::string>& grid)
{
    const int H = (int)grid.size();
    const int W = (int)grid[0].size();
    const int D4[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };

    auto inB = [&](int x, int y) {
        return x > 0 && y > 0 && x < W - 1 && y < H - 1;
        };
    auto isOpen = [&](int x, int y) {
        return grid[y][x] == '0';
        };
    auto isWall = [&](int x, int y) {
        return grid[y][x] == '1';
        };
    auto isProtected = [&](int x, int y) {
        return grid[y][x] == 'R';
        };
    auto degreeOpen = [&](int x, int y) {
        int d = 0;
        for (int i = 0; i < 4; ++i) {
            int nx = x + D4[i][0];
            int ny = y + D4[i][1];
            if (inB(nx, ny) && isOpen(nx, ny)) ++d;
        }
        return d;
        };

    for (int y = 1; y < H - 1; ++y)
    {
        for (int x = 1; x < W - 1; ++x)
        {
            if (grid[y][x] != '2')
                continue;

            // 1) If we already have an adjacent corridor, this door is fine.
            bool hasAdjOpen = false;
            bool hasAdjRoom = false;
            int innerDX = 0, innerDY = 0;

            for (int i = 0; i < 4; ++i)
            {
                int nx = x + D4[i][0];
                int ny = y + D4[i][1];
                if (!inB(nx, ny)) continue;

                char c = grid[ny][nx];
                if (c == '0')
                    hasAdjOpen = true;
                else if (c == '#') {
                    hasAdjRoom = true;
                    innerDX = D4[i][0];
                    innerDY = D4[i][1];
                }
            }

            if (hasAdjOpen)
                continue;

            // 2) Door belongs to an enclosed room: carve ONLY straight outward.
            if (hasAdjRoom)
            {
                int odx = -innerDX;
                int ody = -innerDY;

                // carve 1–2 tiles in outward direction if they are walls
                for (int step = 1; step <= 2; ++step)
                {
                    int px = x + odx * step;
                    int py = y + ody * step;
                    if (!inB(px, py)) break;
                    if (isWall(px, py))
                        grid[py][px] = '0';
                    else
                        break; // stop if we hit non-wall
                }
                continue; // do NOT run the generic BFS for room doors
            }

            // 3) Generic case (non-room doors, e.g. promoted gates):
            //    Use BFS through walls to reach a corridor; carve that path.
            std::queue<std::pair<int, int>> q;
            std::vector<std::vector<int>> dist(H, std::vector<int>(W, -1));
            std::vector<std::vector<std::pair<int, int>>> prev(
                H, std::vector<std::pair<int, int>>(W, std::make_pair(-1, -1)));

            q.push(std::make_pair(x, y));
            dist[y][x] = 0;
            std::pair<int, int> target(-1, -1);

            while (!q.empty() && target.first == -1)
            {
                std::pair<int, int> cur = q.front(); q.pop();
                int cx = cur.first;
                int cy = cur.second;

                for (int i = 0; i < 4; ++i)
                {
                    int nx = cx + D4[i][0];
                    int ny = cy + D4[i][1];
                    if (!inB(nx, ny) || dist[ny][nx] != -1) continue;

                    if (isWall(nx, ny) && !isProtected(nx, ny))
                    {
                        dist[ny][nx] = dist[cy][cx] + 1;
                        prev[ny][nx] = cur;
                        q.push(std::make_pair(nx, ny));
                    }
                    else if (isOpen(nx, ny))
                    {
                        if (degreeOpen(nx, ny) >= 1)
                        {
                            target = std::make_pair(nx, ny);
                            prev[ny][nx] = cur;
                            break;
                        }
                    }
                }
            }

            // If nothing reachable, carve a very conservative fallback:
            if (target.first == -1)
            {
                for (int i = 0; i < 4; ++i)
                {
                    int nx = x + D4[i][0];
                    int ny = y + D4[i][1];
                    if (inB(nx, ny) && isWall(nx, ny))
                    {
                        grid[ny][nx] = '0';
                        break;
                    }
                }
                continue;
            }

            // Retrace and carve the path to the found corridor
            for (std::pair<int, int> cur = target;
                !(cur.first == x && cur.second == y);
                cur = prev[cur.second][cur.first])
            {
                int px = cur.first;
                int py = cur.second;
                if (!inB(px, py) || isProtected(px, py)) break;
                if (isWall(px, py))
                    grid[py][px] = '0';
            }
        }
    }
}

static void EnsureGlobalReachability(std::vector<std::string>& g, std::pair<int, int> spawn)
{
    int H = (int)g.size(), W = (int)g[0].size();
    std::vector<std::vector<std::uint8_t>> vis(H, std::vector<std::uint8_t>(W, 0));
    std::queue<std::pair<int, int>> q;
    auto inB = [&](int x, int y) {return x > 0 && y > 0 && x < W - 1 && y < H - 1; };
    q.push(spawn); vis[spawn.second][spawn.first] = 1;

    const int D[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
    while (!q.empty()) {
        auto [x, y] = q.front(); q.pop();
        for (auto& d : D) {
            int nx = x + d[0], ny = y + d[1];
            if (!inB(nx, ny) || vis[ny][nx]) continue;
            if (g[ny][nx] == '0') { vis[ny][nx] = 1; q.push({ nx,ny }); }
        }
    }

    // Any door not next to a visited '0' gets a small carve; any crate between visited pockets gets cleared
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x) {
        if (g[y][x] == '3' && (vis[y][x] || vis[y][x + 1] || vis[y][x - 1] || vis[y + 1][x] || vis[y - 1][x])) {
            // keep; otherwise you could also choose to clear unreachable crates:
            // if (!vis[y][x]) g[y][x]='0';
        }
        if (g[y][x] == '2')
        {
            bool ok = false;
            for (int k = 0; k < 4; ++k)
            {
                int nx = x + D[k][0];
                int ny = y + D[k][1];
                if (!inB(nx, ny)) continue;

                // If any adjacent corridor is reachable from spawn, door is fine.
                if (g[ny][nx] == '0' && vis[ny][nx])
                {
                    ok = true;
                    break;
                }
            }

            // If !ok, we leave it as-is.
            // Doors should already have been made accessible (or intentionally isolated)
            // by AddEnclosedRoom, EnsureDoorAccessibility, FixRoomEntrances,
            // and FixIsolatedChambers. However might patch back in future.
        }
    }
}

static void FixRoomEntrances(std::vector<std::string>& g)
{
    int H = (int)g.size(), W = (int)g[0].size();
    auto inB = [&](int x, int y) { return x > 0 && y > 0 && x < W - 1 && y < H - 1; };

    const int D4[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };

    for (int y = 1; y < H - 1; ++y)
    {
        for (int x = 1; x < W - 1; ++x)
        {
            if (g[y][x] != '2') continue; // door tile only

            bool orthOpen = false;
            for (auto& d : D4)
            {
                int nx = x + d[0], ny = y + d[1];
                if (inB(nx, ny) && g[ny][nx] == '0')
                {
                    orthOpen = true;
                    break;
                }
            }

            if (!orthOpen)
            {
                // Carve ONE orthogonal corridor tile (outside only)
                for (auto& d : D4)
                {
                    int nx = x + d[0], ny = y + d[1];
                    if (inB(nx, ny) && g[ny][nx] == '1')
                    {
                        g[ny][nx] = '0';
                        break;
                    }
                }
            }
        }
    }
}

static void ClearDoorChokes(std::vector<std::string>& grid)
{
    const int H = (int)grid.size(), W = (int)grid[0].size();
    const int D4[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };

    auto inB = [&](int x, int y) { return x > 0 && y > 0 && x < W - 1 && y < H - 1; };

    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x) {
        if (grid[y][x] != '2') continue;

        // Walk straight out up to 3 steps if corridor is 1-wide; clear movable blockers.
        int cx = x, cy = y;
        for (int step = 0; step < 3; ++step) {
            int ways = 0, nx = cx, ny = cy;
            for (auto& d : D4) {
                int tx = cx + d[0], ty = cy + d[1];
                if (!inB(tx, ty)) continue;
                char c = grid[ty][tx];
                if (c == '0' || c == '3' || c == '4' || c == 'C' || c == 'B' || c == 'H') { ++ways; nx = tx; ny = ty; }
            }
            if (ways != 1) break;            // stop at junc or dead end
            if (grid[ny][nx] == '3' || grid[ny][nx] == '4' || grid[ny][nx] == 'C' || grid[ny][nx] == 'B' || grid[ny][nx] == 'H') grid[ny][nx] = '0';
            cx = nx; cy = ny;
        }
    }
}

namespace MapGenerator {

    // Utility: in-bounds check
    static bool InBounds(int x, int y, int width, int height) {
        return x >= 0 && y >= 0 && x < width && y < height;
    }

    /*
        * @brief Convert an ASCII grid to in-game entities.
        *
        * This step reads each cell and spawns the corresponding ECS entity:
        * - Walls are merged into a single Mesh2D batch for render efficiency.
        * - Colliders are merged per contiguous wall block to reduce physics cost.
        * - Special objects (doors, crates, spawn points) are spawned via prefab factories.
        *
        * @param app Reference to GameApp for entity spawning.
        * @param grid 2D grid describing layout of map tiles.
        * @param tileSize Size of one tile (in world units).
        *
        * @note The batching system minimizes draw calls and collider count, improving performance.
    */
    //
    static bool HasRemovedWallOverride(
        const std::vector<VariantPlacement>* overlayVariants,
        int gx,
        int gyWorld)
    {
        if (!overlayVariants) return false;

        for (const auto& v : *overlayVariants)
        {
            if (v.type == "WallTile" && v.gx == gx && v.gy == gyWorld)
                return v.removed;
        }
        return false;
    }

    static void FromGridWithOverrides(GameApp& app, const std::vector<std::string>& grid, float tileSize, const std::string& bakePrefix, const std::unordered_map<uint64_t, PuzzleTileOverride>& puzzleOverrides) {
        if (grid.empty()) return;
        int height = (int)grid.size();
        int width = 0;

        for (auto& r : grid) width = std::max(width, (int)r.size());

        #if ENABLE_EDITOR
        if (app.GetEditorOverlay())
        {
            app.GetEditorOverlay()->ClearLabyrinthWallEntities();
        }
        #endif

        const std::vector<VariantPlacement>* overlayVariants = nullptr;

        #if ENABLE_EDITOR
        if (app.GetEditorOverlay())
        {
            overlayVariants = &app.GetEditorOverlay()->GetLabyrinthWallVariants();
        }
        #endif
        // Per-tile wall visuals
        // Keep visuals editable per tile, but keep merged colliders below for performance.
        for (int y = 0; y < height; ++y)
        {
            const std::string& row = grid[y];
            for (int x = 0; x < width; ++x)
            {
                if (x >= (int)row.size()) continue;
                if (CellAt(grid, x, y) != '1') continue;

                VariantPlacement variant;
                variant.type = "WallTile";
                variant.gx = x;
                variant.gy = (height - 1 - y); // convert txt row -> world/grid Y
                variant.textureKey = "wall_B&T";
                variant.solid = true;

                bool skipWall = false;

                if (overlayVariants)
                {
                    for (const auto& saved : *overlayVariants)
                    {
                        if (saved.gx == variant.gx && saved.gy == variant.gy)
                        {
                            if (saved.removed)
                            {
                                skipWall = true;
                            }
                            else
                            {
                                variant.textureKey = saved.textureKey;
                                variant.solid = saved.solid;
                            }
                            break;
                        }
                    }
                }

                if (skipWall)
                    continue;

                variant.name = bakePrefix + "WallTile_" +
                    std::to_string(x) + "_" + std::to_string(variant.gy);

                Entity wallTile = MakeWallTileFromVariant(app, variant, tileSize);

                // For Personal ref:
                // MakeWallTileFromVariant creates a collider by default.
                // It is removed here cause the merged collider pass below
                // is still responsible for labyrinth wall collisions.
                if (wallTile != INVALID_ENTITY)
                {
                    app.RemoveColliderComponent(wallTile);

                    #if ENABLE_EDITOR
                    if (app.GetEditorOverlay())
                    {
                        app.GetEditorOverlay()->RegisterLabyrinthWallEntity(
                            variant.gx,
                            variant.gy,
                            wallTile
                        );
                    }
                    #endif
                }
            }
        }

        // --- Merged colliders ---
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
        int colliderCount = 0;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int gyWorld = (height - 1) - y;
                if (visited[y][x] || CellAt(grid, x, y) != '1' ||
                    HasRemovedWallOverride(overlayVariants, x, gyWorld))
                    continue;

                int startX = x;
                while (x < width &&
                    CellAt(grid, x, y) == '1' &&
                    !visited[y][x] &&
                    !HasRemovedWallOverride(overlayVariants, x, (height - 1) - y))
                {
                    x++;
                }
                int runLength = x - startX;

                int h = 1;
                bool canExtend = true;
                while (y + h < height && canExtend)
                {
                    for (int i = 0; i < runLength; ++i)
                    {
                        int checkGyWorld = (height - 1) - (y + h);
                        if (CellAt(grid, startX + i, y + h) != '1' ||
                            visited[y + h][startX + i] ||
                            HasRemovedWallOverride(overlayVariants, startX + i, checkGyWorld))
                        {
                            canExtend = false;
                            break;
                        }
                    }
                    if (canExtend) h++;
                }

                for (int dy = 0; dy < h; ++dy)
                    for (int dx = 0; dx < runLength; ++dx)
                        visited[y + dy][startX + dx] = true;

                float px = startX * tileSize;
                float py = (height - (y + h)) * tileSize;
                float colliderW = runLength * tileSize;
                float colliderH = h * tileSize;

                MakeGameObject(app,
                    bakePrefix + "WallBlock_" + std::to_string(y) + "_" + std::to_string(startX),
                    nullptr,
                    { px, py },
                    { colliderW, colliderH },
                    0.f,
                    { 1,1,1 },
                    ColliderType::Box,
                    false);

                colliderCount++;
            }
        }


        for (int y = 0; y < height; ++y)
        {
            const std::string& row = grid[y];
            for (int x = 0; x < width; ++x)
            {
                if (x >= (int)row.size()) continue;
                char code = row[x];
                if (code == '.' || code == '0' || code == '1') continue;

                float px = x * tileSize;
                float py = (height - 1 - y) * tileSize;

                switch (code)
                {
                case '3': {
                    Entity crate = app.InstantiatePrefab("Crate", { px, py });
                    app.SetEntityName(crate, bakePrefix + "Crate_" + std::to_string(x) + "_" + std::to_string(y));
                    break;
                }
                case '4': {
                    const std::string bakedName = bakePrefix + "Slime_" + std::to_string(x) + "_" + std::to_string(y);
                    if (IsEnemyDefeatedByName(bakedName))
                        break;
                    Entity slime = app.InstantiatePrefab("ranged_mini_boss", { px, py });
                    app.SetEntityName(slime, bakedName);
                    break;
                }
                case '6': {
                    Entity torch = app.InstantiatePrefab("Torch", { px, py });
                    app.SetEntityName(torch, bakePrefix + "torch_" + std::to_string(x) + "_" + std::to_string(y));
                    break;
                }
                case 'C': {
                    const std::string bakedName = bakePrefix + "Slime_" + std::to_string(x) + "_" + std::to_string(y);
                    if (IsEnemyDefeatedByName(bakedName))
                        break;
                    Entity slime = app.InstantiatePrefab("EnemyContact", { px, py });
                    app.SetEntityName(slime, bakedName);
                    break;
                }
                case 'B': {
                    const std::string bakedName = bakePrefix + "Slime_" + std::to_string(x) + "_" + std::to_string(y);
                    if (IsEnemyDefeatedByName(bakedName))
                        break;
                    Entity slime = app.InstantiatePrefab("burrow_mini_boss", { px, py });
                    app.SetEntityName(slime, bakedName);
                    break;
                }
                case 'H': {
                    const std::string bakedName = bakePrefix + "Slime_" + std::to_string(x) + "_" + std::to_string(y);
                    if (IsEnemyDefeatedByName(bakedName))
                        break;
                    Entity slime = app.InstantiatePrefab("heal_mini_boss", { px, py });
                    app.SetEntityName(slime, bakedName);
                    break;
                }
                case '7': {
                    Mesh2D* quad = app.GetMesh(1);
                    const Vector3 white(1.f, 1.f, 1.f);

                    Entity puzzle = MakeGameObject(
                        app,
                        bakePrefix + "Puzzle_" + std::to_string(x) + "_" + std::to_string(y),
                        quad,
                        { px, py },
                        { tileSize, tileSize },
                        0.f,
                        white,
                        ColliderType::Box,
                        /*trigger*/ true);

                    GLuint texID = ResourceManager::GetTexture("PuzzleGenerator");
                    if (texID != 0)
                        if (auto* mr = app.GetRenderer(puzzle)) mr->SetTexture(texID);

                    //generator
                    int genGroup = 1;
                    int requiredHits = 1;
                    int requiresProj = 1;

                    auto itO = puzzleOverrides.find(KeyXY(x, y));
                    if (itO != puzzleOverrides.end())
                    {
                        const auto& o = itO->second;
                        if (!o.isDoor)
                        {
                            genGroup = o.groupId;
                            requiredHits = o.requiredHits;
                            requiresProj = o.requiresProjectile ? 1 : 0;
                        }
                    }

                    PuzzleObject& po = app.AddPuzzleObject(puzzle, PuzzleKind::ShootTarget, genGroup);
                    po.i0 = requiredHits;  // required hits
                    po.i1 = 0;             // current hits
                    po.i2 = requiresProj;  // requires player projectile (1 = yes)
                    po.active = false;

                    break;
                }
                case '8': {
                    Mesh2D* quad = app.GetMesh(1);
                    const Vector3 white(1.f, 1.f, 1.f);

                    Entity lock = MakeGameObject(
                        app,
                        bakePrefix + "DoorLock_" + std::to_string(x) + "_" + std::to_string(y),
                        quad,
                        { px, py },
                        { tileSize, tileSize },
                        0.f,
                        white,
                        ColliderType::Box,
                        /*trigger*/ false // CLOSED initially (solid)
                    );

                    // Give it a texture if you want
                    GLuint texID = ResourceManager::GetTexture("DoorLock");
                    if (texID != 0)
                        if (auto* mr = app.GetRenderer(lock)) mr->SetTexture(texID);

                    // DoorLock: supports legacy single-group and multi-group requirements via sidecar json
                    PuzzleObject& po = app.AddPuzzleObject(lock, PuzzleKind::DoorLock, /*groupId*/ 1);
                    po.active = false;

                    auto itO = puzzleOverrides.find(KeyXY(x, y));
                    if (itO != puzzleOverrides.end())
                    {
                        const auto& o = itO->second;
                        if (o.isDoor)
                        {
                            if (o.manualDoor)
                            {
                                // manual-only door (lever/plate/etc); PuzzleSystem won't auto-open it
                                po.groupId = 0;
                                po.i0 = 0;
                                po.i1 = po.i2 = po.i3 = 0;
                            }
                            else if (o.requiredGroupCount > 0)
                            {
                                // multi-group door
                                po.groupId = o.requiredGroups[0];
                                po.i0 = o.requiredGroupCount;           // number of required groups (1..4)
                                po.i1 = (o.requiredGroupCount >= 2) ? o.requiredGroups[1] : 0;
                                po.i2 = (o.requiredGroupCount >= 3) ? o.requiredGroups[2] : 0;
                                po.i3 = (o.requiredGroupCount >= 4) ? o.requiredGroups[3] : 0;
                            }
                            else
                            {
                                // legacy: single group door
                                po.groupId = o.groupId;
                                po.i0 = 0;
                                po.i1 = po.i2 = po.i3 = 0;
                            }
                        }
                    }

                    break;
                }
                case '9': {
                    int level = 1;
                    std::string needle = "entities_Level";
                    size_t p = bakePrefix.find(needle);
                    if (p != std::string::npos) {
                        p += needle.size();
                        size_t q = p;
                        while (q < bakePrefix.size() && std::isdigit((unsigned char)bakePrefix[q])) ++q;
                        if (q > p) {
                            try { level = std::stoi(bakePrefix.substr(p, q - p)); }
                            catch (...) { level = 1; }
                        }
                    }

                    std::string prefabName = "ranged_mini_boss";
                    if (level == 2) prefabName = "burrow_mini_boss";
                    else if (level == 3) prefabName = "heal_mini_boss";
                    else if (level == 4) prefabName = "Boss";

                    const std::string bakedName = bakePrefix + "Boss_" + std::to_string(level) + "_" + std::to_string(x) + "_" + std::to_string(y);

                    if (IsEnemyDefeatedByName(bakedName))
                        break;
                    Entity bossEntity = app.InstantiatePrefab(prefabName, { px, py });
                    app.SetEntityName(bossEntity, bakedName);
                    if (auto* ec = app.GetEnemyController(bossEntity)) {
                        ec->SetBoss(true);
                        
                        MobType mt = MobType::RANGED;
                        if (level == 2) mt = MobType::BURROW;
                        else if (level == 3) mt = MobType::HEAL;
                        else if (level == 4) mt = MobType::FINALBOSS;
                        
                        ec->SetAiMobType(mt);
                    }

                    if (Transform* t = app.GetTransform(bossEntity))
                    {
                        Vector2 s = t->GetScale();
                        t->SetScale({ s.x * 2.5f, s.y * 2.5f });
                    }

                    if (Collider* c = app.GetCollider(bossEntity))
                    {
                        c->size.x *= 2.5f;
                        c->size.y *= 2.5f;
                    }

                    break;
                }
                case 'P': {
                    Mesh2D* quad = app.GetMesh(1);
                    const Vector3 white(1.f, 1.f, 1.f);

                    Entity burrowWall = MakeGameObject(
                        app,
                        bakePrefix + "BurrowWall_" + std::to_string(x) + "_" + std::to_string(y),
                        quad,
                        { px, py },
                        { tileSize, tileSize },
                        0.f,
                        white,
                        ColliderType::Box,
                        /*trigger*/ false // SOLID by default
                    );

                    GLuint texID = ResourceManager::GetTexture("BurrowWall");
                    if (texID != 0)
                        if (auto* mr = app.GetRenderer(burrowWall)) mr->SetTexture(texID);

                    if (auto* c = app.GetCollider(burrowWall)) {
                        c->isPassableWhenBurrowed = true;
                    }

                    // Register into puzzle system burrowWall
                    PuzzleObject& po = app.AddPuzzleObject(burrowWall, PuzzleKind::BurrowWall, /*groupId*/ 2);
                    po.active = false;

                    break;
                }
                case 'L': {
                    Mesh2D* quad = app.GetMesh(1);
                    const Vector3 white(1.f, 1.f, 1.f);

                    Entity lever = MakeGameObject(
                        app,
                        bakePrefix + "Lever_" + std::to_string(x) + "_" + std::to_string(y),
                        quad,
                        { px, py },
                        { tileSize, tileSize },
                        0.f,
                        white,
                        ColliderType::Box,
                        /*trigger*/ true
                    );

                    GLuint texID = ResourceManager::GetTexture("Lever");
                    if (texID != 0) if (auto* mr = app.GetRenderer(lever)) mr->SetTexture(texID);

                    int leverGroup = 1;
                    int toggle = 1; // default: toggle lever on/off

                    auto itO = puzzleOverrides.find(KeyXY(x, y));
                    if (itO != puzzleOverrides.end())
                    {
                        const auto& o = itO->second;
                        if (o.isLever)
                        {
                            leverGroup = o.groupId;
                            toggle = o.toggle ? 1 : 0;
                        }
                    }

                    PuzzleObject& po = app.AddPuzzleObject(lever, PuzzleKind::Lever, leverGroup);
                    po.i0 = toggle;
                    po.active = false;

                    break;
                }
                case 'D': {
                    Mesh2D* quad = app.GetMesh(1);
                    const Vector3 white(1.f, 1.f, 1.f);

                    Entity lock = MakeGameObject(
                        app,
                        bakePrefix + "DoorLock_" + std::to_string(x) + "_" + std::to_string(y),
                        quad,
                        { px, py },
                        { tileSize, tileSize },
                        0.f,
                        white,
                        ColliderType::Box,
                        /*trigger*/ false // CLOSED initially (solid)
                    );

                    GLuint texID = ResourceManager::GetTexture("DoorLock");
                    if (texID != 0)
                        if (auto* mr = app.GetRenderer(lock)) mr->SetTexture(texID);

                    int doorGroup = 1; // group the lever will activate

                    auto itO = puzzleOverrides.find(KeyXY(x, y));
                    if (itO != puzzleOverrides.end())
                    {
                        const auto& o = itO->second;
                        if (o.isDoor)
                            doorGroup = o.groupId;
                    }

                    PuzzleObject& po = app.AddPuzzleObject(lock, PuzzleKind::DoorLock, doorGroup);

                    po.active = false;
                    po.i0 = -1;  // IMPORTANT: manual-group door (lever-controlled)
                    po.i1 = po.i2 = po.i3 = 0;

                    break;
                }
                case 'M': { // Healing Memory Corpse
                    Mesh2D* quad = app.GetMesh(1);
                    const Vector3 white(1.f, 1.f, 1.f);

                    Entity corpse = MakeGameObject(
                        app,
                        bakePrefix + "MemoryCorpse_" + std::to_string(x) + "_" + std::to_string(y),
                        quad,
                        { px, py },
                        { tileSize, tileSize },
                        0.f,
                        white,
                        ColliderType::Box,
                        /*trigger*/ true // Walkthrough
                    );

                    GLuint texID = ResourceManager::GetTexture("healingMemory"); // Ensure this texture exists or fallback
                    if (texID != 0) if (auto* mr = app.GetRenderer(corpse)) mr->SetTexture(texID);

                    PuzzleObject& po = app.AddPuzzleObject(corpse, PuzzleKind::HealingMemory, 0);
                    
                    // Defaults
                    po.i0 = x + y * 100; // memoryId default
                    po.i1 = -1;          // requiredMemoryId
                    po.f0 = 100.0f;      // healRadius
                    po.f1 = 8.0f;        // duration
                    po.s0 = "A soldier's memory...";
                    po.s1 = "";
                    po.active = false;   // hasBeenHealed = false

                    auto itO = puzzleOverrides.find(KeyXY(x, y));
                    if (itO != puzzleOverrides.end())
                    {
                        const auto& o = itO->second;
                        po.i0 = o.memoryId;
                        po.i1 = o.requiredMemoryId;
                        po.f0 = o.healRadius;
                        po.f1 = o.memoryDuration;
                        po.s0 = o.narrative;
                        po.s1 = o.mapPath;
                    }
                    break;
                }
                case 'F': { // Collapsing Floor
                    Mesh2D* quad = app.GetMesh(1);
                    const Vector3 white(1.f, 1.f, 1.f);

                    Entity floor = MakeGameObject(
                        app,
                        bakePrefix + "FragileFloor_" + std::to_string(x) + "_" + std::to_string(y),
                        quad,
                        { px, py },
                        { tileSize, tileSize },
                        0.f,
                        white,
                        ColliderType::Box,
                        /*trigger*/ true // Walk-through initially so player can step on it
                    );
                        
                    GLuint texID = ResourceManager::GetTexture("Floor_Tile_Fragile");
                    if (texID == 0) texID = ResourceManager::GetTexture("Floor_Tile");
                    if (texID != 0) if (auto* mr = app.GetRenderer(floor)) mr->SetTexture(texID);

                    PuzzleObject& po = app.AddPuzzleObject(floor, PuzzleKind::BurrowFloor, 0);
                    
                    // Defaults
                    po.active = true; // Stable
                    po.f0 = 0.8f;     // collapseDelay
                    po.f1 = 3.0f;     // respawnDelay
                    po.f2 = 0.8f;     // timer
                    po.i0 = 1;        // ignoreWhenBurrowed = true
                    po.i1 = 0;        // collapseOnce = false
                    po.i2 = 1;        // createFallHazard = true

                    auto itO = puzzleOverrides.find(KeyXY(x, y));
                    if (itO != puzzleOverrides.end())
                    {
                        const auto& o = itO->second;
                        po.f0 = o.collapseDelay;
                        po.f1 = o.respawnDelay;
                        po.f2 = o.collapseDelay;
                        po.i0 = o.ignoreWhenBurrowed ? 1 : 0;
                        po.i1 = o.collapseOnce ? 1 : 0;
                        po.i2 = o.createFallHazard ? 1 : 0;
                    }
                    break;
                }

                case '#': {
                    Entity blk = app.InstantiatePrefab("BlackTile", { px, py });
                    app.SetEntityName(blk, bakePrefix + "blk_" + std::to_string(x) + "_" + std::to_string(y));
                    break;
                }
                default: break;
                }
            }
        }
    }

    /*
        * @brief Procedurally generate a labyrinth map.
        *
        * The algorithm proceeds as follows:
        * 1. Initialize a grid of solid walls ('1').
        * 2. Use randomized DFS/random walk to carve open corridors ('0').
        * 3. Place enclosed rooms using AddEnclosedRoom with spacing checks.
        * 4. Guarantee reachability between rooms and corridors.
        * 5. Spawn and link door entities for existing scene files.
        * 6. Bake the results into Mesh2D and colliders for runtime use.
        *
        * @param app Reference to GameApp.
        * @param worldSizePx World dimensions (in pixels).
        * @param tileSize Tile size (in world units).
        *
        * Also, the generator integrates with the ECS and rendering pipeline:
        * - Mesh2D batching reduces draw overhead.
        * - Collider merging simplifies collision passes.
        * - Door linking ensures inter-scene teleport consistency.
    */
    void GenerateMaze(GameApp& app, int worldSizePx, float tileSize) {
        const std::string savePath = AssetPath("maps/labyrinth.txt");
        const std::string doorMetaPath = AssetPath("maps/labyrinth_doors.txt");
        const std::string labyrinthScenePath = AssetPath("scene/labyrinth.json");
        const std::string bakePrefix = RoomBakePrefix(labyrinthScenePath);

        std::vector<std::string> grid;
        if (SceneRuntime::LoadGridTxt(savePath, grid, nullptr, nullptr)) {
            int heightLoaded = (int)grid.size();
            int widthLoaded = (int)grid[0].size();

            bool hasEnemy = false;
            for (int yy = 0; yy < heightLoaded && !hasEnemy; ++yy) {
                for (int xx = 0; xx < widthLoaded; ++xx) {
                    char c = grid[yy][xx];
                    if (c == '4' || c == 'C' || c == 'B' || c == 'H' || c == '9') {
                        hasEnemy = true;
                        break;
                    }
                }
            }

            if (!hasEnemy) {
                std::mt19937 rng{ std::random_device{}() };
                std::uniform_int_distribution<int> enemyChance(0, 99);
                std::uniform_int_distribution<int> enemyType(0, 3);

                auto deg4 = [&](int x, int y) {
                    int d = 0;
                    d += (grid[y][x - 1] == '0');
                    d += (grid[y][x + 1] == '0');
                    d += (grid[y - 1][x] == '0');
                    d += (grid[y + 1][x] == '0');
                    return d;
                };

                for (int yy = 1; yy < heightLoaded - 1; ++yy) {
                for (int xx = 1; xx < widthLoaded - 1; ++xx) {
                    if (grid[yy][xx] != '0')
                        continue;

                    int d = deg4(xx, yy);
                    if (enemyChance(rng) < 4 && (d >= 3 || d == 1)) {
                        int t = enemyType(rng);
                        char code = '4';
                        if (t == 1) code = 'C';
                        else if (t == 2) code = 'B';
                        else if (t == 3) code = 'H';
                        grid[yy][xx] = code;
                    }
                }
            }

                std::ofstream out(savePath);
                for (auto& row : grid) out << row << "\n";
            }

            float worldW = widthLoaded * tileSize;
            float worldH = heightLoaded * tileSize;

            app.SetWorldBounds({ 0.0f, 0.0f }, { worldW, worldH });
            DebugConsole::Get().Info("[MapGenerator] Loaded saved labyrinth (" +
                std::to_string(widthLoaded) + "x" + std::to_string(heightLoaded) + ")\n");

            Vector2 safe = FindSafeSpawn(grid, tileSize);
            app.SetPlayerSpawn(safe);

            std::filesystem::path lp(labyrinthScenePath);
            const std::string stem = lp.stem().string();
            const std::string variantPath = AssetPath("variants/" + stem + "_variants.json");

            std::vector<VariantPlacement> manualVariants;
            Variant::LoadFile(variantPath, heightLoaded, manualVariants);

            std::vector<VariantPlacement> allVariants = manualVariants;
            // GenerateLabyrinthWallVariants(grid, heightLoaded, manualVariants, allVariants);

            // Replace all '#' with '0' (floor), so runtime grid has proper floors
            /*
            for (auto& row : grid) {
                for (char& c : row) {
                    if (c == '#') c = '0';
                }
            }
            */
#if ENABLE_EDITOR
            if (app.GetEditorOverlay())
            {
                app.GetEditorOverlay()->ClearLabyrinthWallVariants();
                app.GetEditorOverlay()->SetLabyrinthWallVariants(allVariants);
            }
#endif
            std::vector<std::string> gridForBake = grid;
            for (const auto& v : allVariants) {
                const bool masksBaseWall =
                    (v.type == "WallTile") ||
                    (v.type == "DoorLock") ||
                    (v.type == "BurrowWall") ||
                    (v.type == "PuzzleGenerator") ||
                    (v.type == "Lever") ||
                    (v.type == "healingMemory") ||
                    (v.type == "MutationHealer");

                bool shouldMask = masksBaseWall && !v.removed;
                if (v.type == "WallTile" && v.removed) {
                    shouldMask = true;
                }

                if (!shouldMask) continue;
                int row = (heightLoaded - 1) - v.gy;
                if (row < 0 || row >= (int)gridForBake.size()) continue;
                if (v.gx < 0 || v.gx >= (int)gridForBake[row].size()) continue;
                
                // Mask with space to prevent underlying tile generation (e.g. double floors)
                gridForBake[row][v.gx] = ' ';
            }

            SyncGridWithVariants(grid, labyrinthScenePath, heightLoaded);
            app.GetMinimapHUD().SetLabyrinth(grid, tileSize);

            std::vector<Entity> variantEntities = CreateVariantEntities(app, allVariants, tileSize);
            for (size_t i = 0; i < variantEntities.size(); ++i) {
                Entity e = variantEntities[i];
                if (e != INVALID_ENTITY) {
                    std::string entityName = bakePrefix + "Variant_" + std::to_string(i);
                    app.SetEntityName(e, entityName);
                }
            }

            auto puzzleOverrides = LoadPuzzleOverridesForGrid(savePath);
            FromGridWithOverrides(app, gridForBake, tileSize, RoomBakePrefix(labyrinthScenePath), puzzleOverrides);
            app.SetEnemyGrid(grid, tileSize);

            std::vector<std::string> sceneFiles;
            for (int i = 1; i <= 5; ++i) {
                std::string path = AssetPath("scene/entities_Level" + std::to_string(i) + ".json");
                if (std::filesystem::exists(path))
                    sceneFiles.push_back(path);
            }

            // Load Metadata
            std::vector<DoorMeta> loadedMeta = LoadDoorMetadata(doorMetaPath);
            auto getMetaLevel = [&](int x, int y) -> int {
                for (const auto& dm : loadedMeta) {
                    if (dm.x == x && dm.y == y) return dm.levelIndex;
                }
                return 0;
            };

            // Collect ALL door tiles from the saved grid and spawn them
            int doorIdx = 0;
            for (int yy = 0; yy < (int)grid.size(); ++yy) {
                for (int xx = 0; xx < (int)grid[yy].size(); ++xx) {
                    if (grid[yy][xx] != '2') continue;

                    // If have more doors than scenes, just stop (or loop)
                    if (doorIdx >= (int)sceneFiles.size()) {
                        DebugConsole::Get().Warning("[MapGenerator] Found extra door tile in saved labyrinth.txt with no matching scene.\n");
                        continue;
                    }

                    int levelIndex = getMetaLevel(xx, yy);
                    std::string targetScene;
                    
                    if (levelIndex > 0) {
                        // Found in metadata, use this exact level
                        targetScene = AssetPath("scene/entities_Level" + std::to_string(levelIndex) + ".json");
                    }
                    else {
                        // Fallback: use scan order
                        targetScene = sceneFiles[doorIdx];
                        levelIndex = ExtractLevelIndexFromPath(targetScene);
                    }

                    // const std::string doorName = "DoorToRoom" + std::to_string(doorIdx + 1);
                    const std::string doorName = "DoorToRoom" + std::to_string(levelIndex);
                    const std::string targetDoor = "ExitToLabyrinth";

                    Vector2 pos{ xx * tileSize, ((int)grid.size() - 1 - yy) * tileSize };
                    DoorArrivalDir dir = ComputeDoorDirFromGrid(gridForBake, xx, yy);

                    SpawnDoor(app, pos, doorName, targetScene, targetDoor, dir, doorIdx, bakePrefix + "Door_" + doorName);
                    ++doorIdx;
                }
            }

            return;
        }

        // Basic dimensions
        int width = std::clamp((int)(worldSizePx / tileSize), 30, 200);
        int height = std::clamp((int)(worldSizePx / tileSize), 30, 200);
        /*std::vector<std::string> grid(height, std::string(width, '1'));*/
        grid.assign(height, std::string(width, '1'));
        std::mt19937 rng{ std::random_device{}() };

        // --- Step 1: Random walk carve                                                                                                    
        const int dirs[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
        std::uniform_int_distribution<int> dirPick(0, 3);
        std::uniform_int_distribution<int> stepPick(1, 5);

        int x = width / 2, y = height / 2;
        grid[y][x] = '0';
        int openCount = 1;
        int targetOpen = (int)(width * height * 0.35f);

        while (openCount < targetOpen) {
            int dir = dirPick(rng), steps = stepPick(rng);
            for (int s = 0; s < steps; ++s) {
                x = std::clamp(x + dirs[dir][0], 1, width - 2);
                y = std::clamp(y + dirs[dir][1], 1, height - 2);
                if (grid[y][x] == '1') { grid[y][x] = '0'; ++openCount; }
            }
        }

        // --- Step 2: DFS side branches
        auto carveDFS = [&](int sx, int sy, int depthMax) {
            int order[4] = { 0,1,2,3 };
            std::stack<std::pair<int, int>> st;
            st.push({ sx, sy });

            while (!st.empty()) {
                auto [cx, cy] = st.top(); st.pop();
                std::shuffle(std::begin(order), std::end(order), rng);

                for (int k = 0; k < 4; ++k) {
                    int i = order[k];
                    int nx = cx + dirs[i][0] * 2;
                    int ny = cy + dirs[i][1] * 2;
                    if (InBounds(nx, ny, width, height) && grid[ny][nx] == '1') {
                        grid[cy + dirs[i][1]][cx + dirs[i][0]] = '0';
                        grid[ny][nx] = '0';
                        st.push({ nx, ny });
                    }
                }
                if ((int)st.size() > depthMax) break;
            }
            };

        for (int i = 0; i < 3; ++i) {
            int rx = std::uniform_int_distribution<int>(2, width - 3)(rng);
            int ry = std::uniform_int_distribution<int>(2, height - 3)(rng);
            if (grid[ry][rx] == '0') carveDFS(rx, ry, 80);
        }

        // --- Step 4: Add entrance and deterministic rooms based on existing scene files ---
        std::vector<std::string> sceneFiles;
        for (int i = 1; i <= 5; ++i) {
            std::string path = AssetPath("scene/entities_Level" + std::to_string(i) + ".json");
            if (std::filesystem::exists(path))
                sceneFiles.push_back(path);
        }

        auto countDoorTiles = [&](const std::vector<std::string>& g) {
            size_t c = 0;
            for (const auto& row : g)
                for (char ch : row)
                    if (ch == '2') ++c;
            return c;
            };

        /* const int mazeH = static_cast<int>(grid.size());
         const int mazeW = mazeH ? static_cast<int>(grid[0].size()) : 0;*/

        DebugConsole::Get().Success("[MapGenerator] Maze size: " + std::to_string(width) + "x" + std::to_string(height) + "\n");

        struct PendingDoor {
            int x, y;
            std::string name;
            std::string targetScene;
            std::string targetDoor;
        };

        std::vector<PendingDoor> pendingDoors;
        std::vector<VariantPlacement> generatedRoomVariants;

        // Generate one room per valid scene file
        int roomIndex = 0;
        for (const auto& scenePath : sceneFiles) {
            const int maxAttempts = 10;
            bool placed = false;

            int levelIndex = ExtractLevelIndexFromPath(scenePath);
            std::string doorName = "DoorToRoom" + std::to_string(levelIndex);

            for (int attempt = 0; attempt < maxAttempts && !placed; ++attempt) {
                int rw = 5 + (rand() % 4);
                int rh = 4 + (rand() % 3);
                int sx = 3 + rand() % (width - rw - 6);
                int sy = 3 + rand() % (height - rh - 6);

                int doorX = -1, doorY = -1;

                bool ok = AddEnclosedRoom(app, grid, sx, sy, rw, rh, tileSize,
                    doorName,
                    scenePath,
                    "ExitToLabyrinth",
                    doorX, doorY,
                    &generatedRoomVariants);

                if (ok) {
                    pendingDoors.push_back({
                        doorX, doorY,
                        doorName,
                        scenePath,
                        "ExitToLabyrinth"
                        });
                    placed = true;
                }
            }

            if (!placed) {
                DebugConsole::Get().Warning("[MapGenerator] Could not place room for " + scenePath + "\n");
            }

            roomIndex++;
        }

        ConnectAllRegions(grid);
        FixDiagonalCorners(grid);
        FixRoomEntrances(grid);
        EnsureDoorAccessibility(grid);
        FixIsolatedChambers(grid);

        auto nearDoorR2 = [&](int x, int y) {
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    if (std::abs(dx) + std::abs(dy) > 2) continue;     // manhattan ring <= 2
                    int nx = x + dx, ny = y + dy;
                    if (ny < 1 || ny >= height - 1 || nx < 1 || nx >= width - 1) continue;
                    if (grid[ny][nx] == '2') return true;
                }
            }
            return false;
            };

        // --- Step 3: Sprinkle crates n enemies
        std::uniform_int_distribution<int> crateChance(0, 99);
        /*auto isDoorAdj = [&](int x, int y) {
            for (auto [dx, dy] : std::array<std::pair<int, int>, 4>{ {{1,0},{-1,0},{0,1},{0,-1}} })
                if (grid[y + dy][x + dx] == '2') return true;
            return false;
            };*/
        auto isStraightCorridor = [&](int x, int y) {
            bool left = grid[y][x - 1] == '0';
            bool right = grid[y][x + 1] == '0';
            bool up = grid[y - 1][x] == '0';
            bool down = grid[y + 1][x] == '0';

            // Horizontal hallway
            if (left && right && !up && !down) return true;

            // Vertical hallway
            if (up && down && !left && !right) return true;

            return false;
            };

        auto deg4 = [&](int x, int y) {
            int d = 0;
            d += (grid[y][x - 1] == '0');
            d += (grid[y][x + 1] == '0');
            d += (grid[y - 1][x] == '0');
            d += (grid[y + 1][x] == '0');
            return d;
            };
        auto isElbow = [&](int x, int y) {
            bool L = grid[y][x - 1] == '0';
            bool R = grid[y][x + 1] == '0';
            bool U = grid[y - 1][x] == '0';
            bool D = grid[y + 1][x] == '0';
            // elbow/corner = exactly two neighbors that are orthogonal (not opposite)
            return ((L || R) && (U || D)) && !(L && R) && !(U && D);
            };

        //crate sprinkling loop:
        for (int yy = 1; yy < height - 1; ++yy)
        {
            for (int xx = 1; xx < width - 1; ++xx)
            {
                int d = deg4(xx, yy);              // neighbors count
                if (grid[yy][xx] == '0' &&
                    crateChance(rng) < 3 &&
                    !nearDoorR2(xx, yy) &&
                    !isStraightCorridor(xx, yy) &&
                    !isElbow(xx, yy) &&            // skip elbow/choke tiles
                    (d >= 3 || d == 1))            // prefer junctions or dead-ends
                {
                    grid[yy][xx] = '3';
                }
            }
        }

        std::uniform_int_distribution<int> enemyChance(0, 99);
        std::uniform_int_distribution<int> enemyType(0, 3);
        for (int yy = 1; yy < height - 1; ++yy)
        {
            for (int xx = 1; xx < width - 1; ++xx)
            {
                int d = deg4(xx, yy);
                if (grid[yy][xx] == '0' &&
                    enemyChance(rng) < 4 &&
                    !nearDoorR2(xx, yy) &&
                    !isStraightCorridor(xx, yy) &&
                    !isElbow(xx, yy) &&
                    (d >= 3 || d == 1))
                {
                    int t = enemyType(rng);
                    char code = '4';
                    if (t == 1) code = 'C';
                    else if (t == 2) code = 'B';
                    else if (t == 3) code = 'H';
                    grid[yy][xx] = code;
                }
            }
        }

        std::uniform_int_distribution<int> torchChance(0, 99);

        for (int yy = 1; yy < height - 1; ++yy)
        {
            for (int xx = 1; xx < width - 1; ++xx)
            {
                if (grid[yy][xx] != '0') continue;               // avoids walls/doors/crates/enemies/black

                // % chance
                if (torchChance(rng) < 4) {                    // 4%
                    grid[yy][xx] = '6';                          // Torch marker
                }
            }
        }

        ClearDoorChokes(grid);

        // --- Step 5: Enforce border walls to fully enclose the maze
        for (int xx = 0; xx < width; ++xx) {
            grid[0][xx] = '1';
            grid[height - 1][xx] = '1';
        }
        for (int yy = 0; yy < height; ++yy) {
            grid[yy][0] = '1';
            grid[yy][width - 1] = '1';
        }

        // --- Step 5.5: Generate variants for 3x3 wall chunks (Labyrinth walls) ---
        {
            // We pass generatedRoomVariants as 'outVariants' so new variants are appended.
            // GenerateLabyrinthWallVariants will internally mark existing ones to avoid duplicates.
            // ONLY generate for protected walls (Rooms)
            GenerateLabyrinthWallVariants(grid, height, {}, generatedRoomVariants, true);
        }

        // Convert protected room walls back to normal walls
        for (auto& row : grid)
            for (char& c : row)
                if (c == 'R') c = '1';

        // --- Step 6: Save + render
        // Choose safe spawn on final decorated grid
        Vector2 safe = FindSafeSpawn(grid, tileSize);
        app.SetPlayerSpawn(safe);

        // Ensure global reachability based on that spawn
        EnsureGlobalReachability(
            grid,
            { (int)std::round(safe.x / tileSize),
              (int)std::round((grid.size() - 1) - safe.y / tileSize) }
        );

        // Now save the final, fixed version (with '#' preserved for future loading)
        std::ofstream out(savePath);
        for (auto& row : grid) out << row << "\n";
        out.close();

        // Replace all '#' with '0' (floor) for runtime use
        
        for (auto& row : grid) {
            for (char& c : row) {
                if (c == '#') c = '0';
            }
        }
        

        //DebugConsole::Get().Success("[MapGenerator] Hybrid maze generated " + std::to_string(width) + "x" + std::to_string(height) + " rooms= " + numRooms + "\n");

        std::filesystem::path lp(labyrinthScenePath);
        const std::string stem = lp.stem().string();
        const std::string variantPath = AssetPath("variants/" + stem + "_variants.json");

        // generated a new map, so we discard any old variants file and use the new generated ones.
        // also SAVE these variants so the loading path works next time.
        //std::vector<VariantPlacement> allVariants = generatedRoomVariants;

        std::vector<VariantPlacement> allVariants;


#if ENABLE_EDITOR
        if (app.GetEditorOverlay())
        {
            allVariants = app.GetEditorOverlay()->GetLabyrinthWallVariants();
        }
        else
        {
            allVariants = generatedRoomVariants;
        }
        
        if (app.GetEditorOverlay())
        {
            app.GetEditorOverlay()->ClearLabyrinthWallVariants();
            app.GetEditorOverlay()->SetLabyrinthWallVariants(allVariants);
        }
#else
        allVariants = generatedRoomVariants;
#endif

        // Save generated variants to JSON (empty txtPath means only JSON is saved).
        // We pass a copy of the grid because SaveRoomData modifies it (burns variants as '1'), 
        // and we want to preserve the original grid state (especially '0's and '#') for runtime logic.
        {
            std::vector<std::string> tempGrid = grid;
            Variant::SaveRoomData("", variantPath, stem, tileSize, tempGrid, allVariants);
        }

        std::vector<std::string> gridForBake = grid;
        for (const auto& v : allVariants) {
            const bool masksBaseWall =
                (v.type == "WallTile") ||
                (v.type == "DoorLock") ||
                (v.type == "BurrowWall") ||
                (v.type == "PuzzleGenerator") ||
                (v.type == "Lever") ||
                (v.type == "healingMemory") ||
                (v.type == "MutationHealer");

            bool shouldMask = masksBaseWall && !v.removed;
            if (v.type == "WallTile" && v.removed) {
                shouldMask = true;
            }

            if (!shouldMask) continue;
            int row = (height - 1) - v.gy;
            if (row < 0 || row >= (int)gridForBake.size()) continue;
            if (v.gx < 0 || v.gx >= (int)gridForBake[row].size()) continue;
            
            // Mask with space to prevent underlying tile generation
            gridForBake[row][v.gx] = ' ';
        }

        SyncGridWithVariants(grid, labyrinthScenePath, height);
        app.GetMinimapHUD().SetLabyrinth(grid, tileSize);

        auto puzzleOverrides = LoadPuzzleOverridesForGrid(savePath);
        FromGridWithOverrides(app, gridForBake, tileSize, RoomBakePrefix(labyrinthScenePath), puzzleOverrides);
        app.SetEnemyGrid(grid, tileSize);

        std::vector<Entity> variantEntities = CreateVariantEntities(app, allVariants, tileSize);
        for (size_t i = 0; i < variantEntities.size(); ++i) {
            Entity e = variantEntities[i];
            if (e != INVALID_ENTITY) {
                std::string entityName = bakePrefix + "Variant_" + std::to_string(i);
                app.SetEntityName(e, entityName);
            }
        }

        std::vector<DoorMeta> generatedDoorsMeta;
        for (const auto& pd : pendingDoors){
            // grid -> world
            Vector2 pos{ pd.x * tileSize, (height - 1 - pd.y) * tileSize };

            int lvl = ExtractLevelIndexFromPath(pd.targetScene);
            if (lvl > 0) {
                generatedDoorsMeta.push_back({ pd.x, pd.y, lvl });
            }

            // Decide arrival direction based on which adjacent tile is corridor '0'
            DoorArrivalDir dir = ComputeDoorDirFromGrid(grid, pd.x, pd.y);

            // Spawn + register link
            SpawnDoor(app,
                pos,
                pd.name,
                pd.targetScene,
                pd.targetDoor,
                dir,
                0,
                bakePrefix + "Door_" + pd.name);
        }

        SaveDoorMetadata(doorMetaPath, generatedDoorsMeta);

        float worldW = width * tileSize;
        float worldH = height * tileSize;
        Vector2 minB = { 0.0f, 0.0f };
        Vector2 maxB = { worldW, worldH };

        app.SetWorldBounds(minB, maxB);
    }

    /*
        * @brief Build a grid from a flat ASCII string and delegate to FromGrid.
        *
        * Converts a one-dimensional ASCII string to a 2D grid by wrapping at given width.
        * Useful for deterministic testing or small handcrafted maps.
        *
        * @param app Game instance.
        * @param layout ASCII map data (row-major order).
        * @param width Number of columns per row.
        * @param tileSize Tile world size.
    */
    void FromString(GameApp& app, const std::string& layout, int width, float tileSize) {
        if (width <= 0 || layout.empty()) {
            DebugConsole::Get().Error("[MapGenerator] Invalid layout or width.\n");
            return;
        }

        int height = static_cast<int>(layout.size()) / width;
        Vector2 start{ 0.f, 0.f };

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                char code = layout[y * width + x];
                if (code == '0') continue;

                Vector2 pos{ start.x + x * tileSize, start.y + (height - 1 - y) * tileSize };

                switch (code) {
                case '1': app.InstantiatePrefab("Wall", pos);     break;
                case '3': app.InstantiatePrefab("Crate", pos);    break;
                case '4': app.InstantiatePrefab("ranged_mini_boss", pos);    break;
                case '5': app.InstantiatePrefab("PlayerSpawn", pos); break;
                case '6': app.InstantiatePrefab("Torch", pos);    break;
                case 'C': app.InstantiatePrefab("EnemyContact", pos); break;
                case 'B': app.InstantiatePrefab("burrow_mini_boss", pos); break;
                case 'H': app.InstantiatePrefab("heal_mini_boss", pos); break;
                case 'M': app.InstantiatePrefab("MutationHealer", pos); break;
                default:
                    DebugConsole::Get().Warning("[MapGenerator] Unknown tile '" + std::string(1, code) + "' at (" + std::to_string(x) + "," + std::to_string(y) + ")\n");
                    break;
                }
            }
        }
    }

   

    void MapGenerator::FromGrid(GameApp& app,
        const std::vector<std::string>& grid,
        float tileSize,
        const std::string& bakePrefix)
    {
        // Default: no overrides (keeps all old behaviour)
        static const std::unordered_map<uint64_t, PuzzleTileOverride> kEmpty;
        FromGridWithOverrides(app, grid, tileSize, bakePrefix, kEmpty);
    }

    void GenerateRoom(GameApp& app,
        const std::string& scenePath,
        const std::string& savePath,
        int /*roomWidth*/,
        int /*roomHeight*/,
        float tileSize)
    {
        std::vector<std::string> grid;
        int width = 0;
        int height = 0;

        int doorX = -1;
        int doorY = -1;

        // -------------------------------------------------------------
        // 1) Load existing room from txt (if it exists)
        //    We preserve door position using a '2' marker in the file.
        // -------------------------------------------------------------

        if (std::filesystem::exists(savePath)) {
            if (!SceneRuntime::LoadGridTxt(savePath, grid, &doorX, &doorY)) {
                DebugConsole::Get().Warning("[MapGenerator] Failed to load room from " + savePath + "\n");
            }
            else {
                height = (int)grid.size();
                width = (int)grid[0].size();

                DebugConsole::Get().Info("[MapGenerator] Loaded room from " + savePath +
                    " (" + std::to_string(width) + "x" + std::to_string(height) + ")\n");
            }
        }


        // -------------------------------------------------------------
        // 2) If no valid file, procedurally generate a new room
        // -------------------------------------------------------------
        if (grid.empty())
        {
            DebugConsole::Get().Error(
                "[MapGenerator] Room grid missing/empty: " + savePath +
                ". Room auto-generation disabled. Create/edit this room in the editor.\n");
            return;
        }

        if (width <= 0 || height <= 0)
        {
            DebugConsole::Get().Error("[MapGenerator] GenerateRoom: Empty grid, abort!\n");
            return;
        }

        // -------------------------------------------------------------
        // 3) World bounds
        // -------------------------------------------------------------
        float worldW = width * tileSize;
        float worldH = height * tileSize;
        app.SetWorldBounds({ 0.0f, 0.0f }, { worldW, worldH });

        // -------------------------------------------------------------
        // 4) Render / colliders: feed a copy WITHOUT '2' into FromGrid
        // -------------------------------------------------------------
        std::vector<std::string> gridForBake = grid;
        if (doorX >= 0 && doorY >= 0 && doorY < height && doorX < width)
            gridForBake[doorY][doorX] = '0';  // treat door tile as open floor for baking

        std::vector<std::string> gridForAI = gridForBake; //let ai use unmasked grid

        const std::string prefix = RoomBakePrefix(scenePath);

        MaskVariantWallsOutOfBakeGrid(gridForBake, scenePath, height);

        // -------------------------------------------------------------
        // 4.5) Apply wall variants 
        // -------------------------------------------------------------
        std::vector<Entity> variantEntities = ApplyRoomVariants(app, scenePath, height, tileSize);

        for (size_t i = 0; i < variantEntities.size(); ++i) {
            Entity e = variantEntities[i];
            if (e != INVALID_ENTITY) {
                std::string entityName = prefix + "Variant_" + std::to_string(i);
                app.SetEntityName(e, entityName);
            }
        }

        auto puzzleOverrides = LoadPuzzleOverridesForGrid(savePath);
        FromGridWithOverrides(app, gridForBake, tileSize, RoomBakePrefix(scenePath), puzzleOverrides);
        //app.GetMinimapHUD().SetLabyrinth(gridForBake, tileSize);
        SyncGridWithVariants(gridForAI, scenePath, height);
        app.GetMinimapHUD().SetLabyrinth(gridForAI, tileSize);
        app.SetEnemyGrid(gridForAI, tileSize);

        // -------------------------------------------------------------
        // 5) Spawn the functional door entity back to the labyrinth
        // -------------------------------------------------------------
        if (doorX >= 0 && doorY >= 0)
        {
            float px = doorX * tileSize;
            float py = (height - 1 - doorY) * tileSize;
            Vector2 pos{ px, py };

            // Choose arrivalDir based on adjacent open tile (same logic style as labyrinth)
            DoorArrivalDir dir = DoorArrivalDir::Right;
            auto safeGet = [&](int x, int y) -> char {
                if (x < 0 || y < 0 || x >= width || y >= height) return '1';
                return gridForBake[y][x];
                };
            if (safeGet(doorX, doorY - 1) == '0')      dir = DoorArrivalDir::Top;
            else if (safeGet(doorX, doorY + 1) == '0') dir = DoorArrivalDir::Bottom;
            else if (safeGet(doorX - 1, doorY) == '0') dir = DoorArrivalDir::Left;
            else if (safeGet(doorX + 1, doorY) == '0') dir = DoorArrivalDir::Right;

            // Parse level index from scenePath: entities_LevelN.json -> N
            int levelIndex = 1;
            {
                std::string needle = "entities_Level";
                size_t p = scenePath.rfind(needle);
                if (p != std::string::npos)
                {
                    p += needle.size();
                    size_t q = p;
                    while (q < scenePath.size() && std::isdigit(static_cast<unsigned char>(scenePath[q])))
                        ++q;
                    if (q > p)
                    {
                        try {
                            levelIndex = std::stoi(scenePath.substr(p, q - p));
                        }
                        catch (...) {
                            levelIndex = 1;
                        }
                    }
                }
            }

            std::string baseName = "ExitToLabyrinth";
            std::string targetScene = AssetPath("scene/labyrinth.json");
            std::string targetDoor = "DoorToRoom" + std::to_string(levelIndex);

            SpawnDoor(app, pos, baseName, targetScene, targetDoor, dir, 0, prefix + "Door_ExitToLabyrinth");

            DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[MapGenerator] Room door '" + baseName, "' at grid (" + doorX,
                "," + doorY, ") world=(" + std::to_string(pos.x), "," + std::to_string(pos.y), ") -> " + targetScene, " door " + targetDoor, "\n");
        }
    }


    void BakeRoomFromGrid(GameApp& app, const std::vector<std::string>& grid, float tileSize, const std::string& scenePath) {
        if (grid.empty()) return;

        ClearBakedRoom(app, scenePath);

        const std::string prefix = RoomBakePrefix(scenePath);

        int height = (int)grid.size();
        int width = (int)grid[0].size();

        // find door marker '2' in the edited grid
        int doorX = -1, doorY = -1;
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < (int)grid[y].size(); ++x)
                if (grid[y][x] == '2') { doorX = x; doorY = y; break; }
       

        // bounds
        float worldW = width * tileSize;
        float worldH = height * tileSize;
        app.SetWorldBounds({ 0.f,0.f }, { worldW, worldH });

        // bake copy without '2'
        std::vector<std::string> gridForBake = grid;
        if (doorX >= 0 && doorY >= 0 && doorY < height && doorX < width)
            gridForBake[doorY][doorX] = '0';

        std::vector<std::string> gridForAI = gridForBake;

        MaskVariantWallsOutOfBakeGrid(gridForBake, scenePath, height);

        std::vector<Entity> variantEntities = ApplyRoomVariants(app, scenePath, height, tileSize);

        for (size_t i = 0; i < variantEntities.size(); ++i) {
            Entity e = variantEntities[i];
            if (e != INVALID_ENTITY) {
                std::string entityName = prefix + "Variant_" + std::to_string(i);
                app.SetEntityName(e, entityName);
            }
        }

        FromGrid(app, gridForBake, tileSize, prefix);
        SyncGridWithVariants(gridForAI, scenePath, height);
        app.GetMinimapHUD().SetLabyrinth(gridForAI, tileSize);
        app.SetEnemyGrid(gridForAI, tileSize);

        // spawn room->labyrinth door 
        if (doorX >= 0 && doorY >= 0){
            float px = doorX * tileSize;
            float py = (height - 1 - doorY) * tileSize;
            Vector2 pos{ px, py };

            DoorArrivalDir dir = DoorArrivalDir::Right;
            auto safeGet = [&](int x, int y)->char {
                if (x < 0 || y < 0 || x >= width || y >= height) return '1';
                return gridForBake[y][x];
                };
            if (safeGet(doorX, doorY - 1) == '0')      dir = DoorArrivalDir::Top;
            else if (safeGet(doorX, doorY + 1) == '0') dir = DoorArrivalDir::Bottom;
            else if (safeGet(doorX - 1, doorY) == '0') dir = DoorArrivalDir::Left;
            else if (safeGet(doorX + 1, doorY) == '0') dir = DoorArrivalDir::Right;

            // parse LevelN from scenePath
            int levelIndex = ExtractLevelIndexFromPath(scenePath);

            std::string baseName = "ExitToLabyrinth";
            std::string targetScene = AssetPath("scene/labyrinth.json");
            std::string targetDoor = "DoorToRoom" + std::to_string(levelIndex);

            //SpawnDoor(app, pos, baseName, targetScene, targetDoor, dir, 0);
            SpawnDoor(app, pos, baseName, targetScene, targetDoor, dir, 0, prefix + "Door_ExitToLabyrinth");
        }
    }


    void ClearBakedRoom(GameApp& app, const std::string& scenePath){
        const std::string prefix = RoomBakePrefix(scenePath);

        std::unordered_set<Entity> doomed;
        doomed.reserve(2048);

        for (const auto& obj : app.GetAllEntities()) {
            if (StartsWith(obj.name, prefix)){
                app.ClearPrefabTagIfAny(obj.id);
                doomed.insert(obj.id);
            }
        }

        if (!doomed.empty()){
            app.GetDoorSystem().UnregisterDoorsForEntities(doomed);
            for (Entity e : doomed)
                app.DestroyOwnedMeshFor(e);

            app.RemoveEntities(doomed);
        }

        
    }

    void MarkEnemyDefeatedByName(const std::string& bakedName)
    {
        if (!bakedName.empty())
            gDefeatedEnemyNames.insert(bakedName);
    }

    bool IsEnemyDefeatedByName(const std::string& bakedName)
    {
        return !bakedName.empty() && gDefeatedEnemyNames.count(bakedName) > 0;
    }

    bool IsBossDefeated(int level)
    {
        std::string searchStr = "Boss_" + std::to_string(level) + "_";
        for (const auto& name : gDefeatedEnemyNames) {
            if (name.find(searchStr) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    void ClearDefeatedEnemies()
    {
        gDefeatedEnemyNames.clear();
    }

    // For Future Use: Simple enclosed room generator DO NOT DELETE, I have to test it in the future
    //void MapGenerator::GenerateRoom(GameApp& app,
    //    int roomWidth,
    //    int roomHeight,
    //    float tileSize,
    //    const std::string& doorName,
    //    const std::string& targetScene,
    //    const std::string& targetDoor)
    //{
    //    std::vector<std::string> grid(roomHeight, std::string(roomWidth, '0'));

    //    for (int y = 0; y < roomHeight; ++y)
    //    {
    //        for (int x = 0; x < roomWidth; ++x)
    //        {
    //            // Make outer border solid walls
    //            if (x == 0 || x == roomWidth - 1 || y == 0 || y == roomHeight - 1)
    //                grid[y][x] = '1';
    //            else
    //                grid[y][x] = '0';
    //        }
    //    }

    //    // Add a single door in the bottom wall
    //    int doorX = roomWidth / 2;
    //    grid[roomHeight - 1][doorX] = '2';

    // DebugConsole::Get().Info("[MapGenerator] Generated enclosed room " + std::to_string(roomWidth) + "x" + std::to_string(roomHeight) + "\n");

    //    // Render the room
    //    FromGrid(app, grid, tileSize);

    //    // Spawn and register the door manually (so it links to labyrinth)
    //    Vector2 doorPos{ doorX * tileSize, (roomHeight - 1 - (roomHeight - 1)) * tileSize };
    //    SpawnDoor(app, doorPos, doorName, targetScene, targetDoor, DoorArrivalDir::Bottom, 0);
    //}
};
