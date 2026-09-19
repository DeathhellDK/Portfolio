#pragma once
/**
 * @file    component.h
 * @author  Jethro Sung
 * @email   sung.h
 * @date    2025-09-29
 *
 * @brief   Declares the base Component class and the Entity type.
 *
 * Entities are defined as unique 32-bit unsigned integers (`Entity`),
 * serving as IDs in the ECS (Entity-Component-System) architecture.
 * All component types derive from the base Component class, which
 * ties each component to the entity that owns it.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include <cstdint>
#include <bitset>




// Maximum number of distinct component types ECS supports.
constexpr std::size_t MAX_COMPONENTS = 32;

// Signature represents which components an entity has using a bitmask.
// Example: 00001101 means this entity has components 0, 2, and 3.
using Signature = std::bitset<MAX_COMPONENTS>;

// Enum listing all possible component types in engine.
// Each component corresponds to a unique bit position in the Signature bitset.
enum ComponentType : std::size_t
{
    TRANSFORM = 0,
    MESHRENDERER,
    SPRITEANIMATOR,
    COLLIDER,
    PLAYERCONTROLLER,
    PERSISTENTTAG,
    SPEED,
    MASS,
    ENEMYCONTROLLER,
    PARTICLEEMITTER,
    PUZZLEOBJECT,
    PROJECTILE,
    LIGHT,
    // Add new components above this line
    COMPONENT_COUNT // keep this as the last item DO NOT REMOVE
};

/**
* @typedef Entity
* @brief Alias for entity IDs.
*
* Entities are unique 32-bit unsigned integers that identify game objects.
* Components are associated with these IDs, and systems operate on them.
*/
using Entity = std::uint32_t;


/**
* @brief Sentinel value representing an invalid or non-existent entity.
*
* This constant is used to mark uninitialized, destroyed, or missing entities.
*/
constexpr Entity INVALID_ENTITY = static_cast<Entity>(-1);


/**
 * @class Component
 * @brief Base type for all ECS components.
 *
 * Each component holds the ID of the entity that owns it.
 * Specific components (e.g., Transform, Collider, MeshRenderer)
 * inherit from this base class to ensure they are linked
 * to a valid entity.
 */
class Component {
public:
    /**
     * @brief Constructs a Component owned by a given entity.
     *
     * @param ownerId The entity ID that owns this component.
     */
    explicit Component(Entity ownerId) : owner(ownerId) {}//all component we req the id, and must be tied to an id when created.
    // Virtual dtor to allow safe polymorphic deletion.
    virtual ~Component() = default;

    // Which entity owns this component
    Entity owner;//storing of which entity owns this component
};