#pragma once
/**
 * @file    gameobj.h
 * @author  Jethro
 * @email   sung.h
 * @date    2025-09-29
 *
 * @brief   Declares the GameObject struct as a lightweight entity record.
 *
 * In this ECS design, a GameObject is not a container of components.
 * Instead, it is a simple record containing only the entity ID,
 * a debug name, and an active flag. Components are stored separately
 * in arrays/vectors and associated with these IDs.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include <string>
#include <cstdint>

/**
 * @typedef Entity
 * @brief Alias for entity IDs (32-bit unsigned integer).
 */
using Entity = std::uint32_t;


/**
 * @struct GameObject
 * @brief A simple record for identifying an entity in the game world.
 *
 * Stores the entity ID, a human-readable debug name, and an active flag.
 * Actual behavior and data are provided by components linked via the ID.
 * essentially, we have remodified game obj into a struct that only holds the entity id, name and active status
 * Now instead of having a GameObject class that holds components, we have an EntityManager that manages entities
 * and components are stored in separate arrays/vectors in the GameApp class
 * So now game objs are more like a label for an entity ID, and components are associated with these IDs in the GameApp
 */
struct GameObject {
    Entity id;              // unique ID
    std::string name;       // debug name or tag
    bool active = true;     // can disable without destroying

    /**
    * @brief Constructs a new GameObject record.
    *
    * @param eid Entity ID.
    * @param n   Debug name or tag.
    */
    GameObject(Entity eid, const std::string& n)
        : id(eid), name(n) {
    }
};