#pragma once
/**
 * @file      prefabManager.h
 * @author    Jethro Sung
 * @email     sung.h
 * @date      2025-10-25
 *
 * @brief     Declaration of the PrefabManager class.
 *
 * The PrefabManager is responsible for loading, parsing, and storing
 * prefab definitions from JSON files. Prefabs describe reusable entity
 * templates (mesh, collider, texture, emitter, etc.) that can be
 * instantiated during level loading or editor runtime.
 *
 * @version   1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "prefab.h"
#include <unordered_map>
#include <string>

 /**
  * @class PrefabManager
  * @brief Handles loading and retrieval of prefab data from JSON files.
  *
  * This manager parses prefab definitions stored in `Assets/prefabs.json`
  * (or similar), validates their structure, and stores them in a lookup map.
  * Each prefab can then be retrieved by name and used to spawn entities.
  *
  */
class PrefabManager {
public:
    /**
     * @brief Load all prefabs from a JSON file into memory.
     *
     * Parses the given JSON file and stores each prefab definition under
     * its logical name. Handles nested "prefabs" objects or top-level arrays.
     *
     * @param filename Path to the prefab definition file (e.g., "Assets/prefabs.json").
     * @return true if at least one prefab was successfully loaded, false otherwise.
     */
    bool LoadPrefabs(const std::string& filename);

    /**
     * @brief Saves the current set of prefabs back to a JSON file.
     *
     * Serializes all loaded prefabs, including their scale, color, collider,
     * texture, and particle emitter settings into the specified file.
     *
     * @param filename Output JSON file path.
     * @return true if the file was written successfully.
     */
    bool SavePrefabs(const std::string& filename) const;

    /**
     * @brief Retrieve a prefab definition by name.
     *
     * @param name Logical prefab name (e.g., "Crate", "Wall", "Player").
     * @return Pointer to the Prefab object, or nullptr if not found.
     */
    const Prefab* Get(const std::string& name) const;

    /**
     * @brief Get a list of all loaded prefab names.
     *
     * @return std::vector<std::string> List of names.
     */
    std::vector<std::string> GetNames() const;

private:
    // Internal map of prefab definitions indexed by their names
    std::unordered_map<std::string, Prefab> prefabs;
};