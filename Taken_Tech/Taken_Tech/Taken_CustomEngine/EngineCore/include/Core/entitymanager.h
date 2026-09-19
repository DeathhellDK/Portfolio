#pragma once
/**
 * @file    entitymanager.h
 * @author  Jethro
 * @email    sung.h, sweeyongdillon.sng
 * @co-author Sng Swee Yong Dillon
 * @date    2025-09-29
 *
 * @brief   Declares the EntityManager class.
 *
 * The EntityManager is responsible for creating, storing, and destroying
 * entities (lightweight IDs with debug metadata). It does not manage
 * components directly; instead, it provides entity lifecycle management
 * for use with systems and component vectors.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "gameobj.h"
#include "Core/component.h"
#include <vector>
#include <algorithm>
#include <unordered_map>
 /**
  * @class EntityManager
  * @brief Manages entity records and signature mappings in the ECS architecture.
  *
  * Each entity corresponds to:
  * - A unique numeric ID
  * - A debug name
  * - An active state (stored within GameObject)
  *
  * EntityManager stores GameObjects and maintains a signature table used by
  * systems to determine whether an entity should be processed.
  *
  * @note Destroying an entity here removes only internal tracking. To prevent
  *       memory leaks, associated components must be removed from their
  *       respective component stores in the component context.
  */
class EntityManager {
public:
    /**
     * @brief Creates a new entity with a given debug name.
     *
     * Generates a new unique ID, constructs a `GameObject` record,
     * and stores it internally.
     *
     * @param name Debug name/tag for the entity.
     * @return Entity ID of the created entity.
     */
    Entity CreateEntity(const std::string& name) {
        Entity id = nextId++;
        entities.emplace_back(id, name);
        signatures[id] = Signature(); // initialize blank signature
        return id;
    }

    /**
     * @brief Creates an entity with a specific fixed ID.
     *
     * Used for operations like undo/redo or level loading where entities
     * must be restored with their original IDs.
     *
     * @param id The predetermined ID to assign to the new entity.
     * @return The entity ID (same as the input parameter).
     */
    Entity CreateEntityWithFixedID(Entity id)
    {
        // prevent ID reuse
        if (id >= nextId)
            nextId = id + 1;

        // create GameObject entry
        entities.emplace_back(id, "RestoredEntity");

        // initialize signature
        signatures[id] = Signature();

        return id;
    }


    /**
     * @brief Retrieves a pointer to an entity record by ID.
     *
     * @param id Entity ID to search for.
     * @return Pointer to the entity's GameObject record,
     *         or nullptr if not found.
     */
    GameObject* GetEntity(Entity id) {
        for (auto& e : entities) {
            if (e.id == id) return &e;
        }
        return nullptr;
    }

    /**
     * @brief Destroys an entity by ID.
     *
     * Removes the entity record from the internal list.
     * Does not automatically clean up associated components.
     *
     * @param id Entity ID to remove.
     */
    void DestroyEntity(Entity id) {
        entities.erase(
            std::remove_if(entities.begin(), entities.end(),
                [id](const GameObject& e) { return e.id == id; }),
            entities.end()
        );
        signatures.erase(id);
    }

    /**
     * @brief Returns a read-only reference to all entity records.
     *
     * @return Constant reference to internal entity vector.
     */
    const std::vector<GameObject>& GetEntities() const { return entities; }

    //Signature management
    /**
     * @brief Sets a signature for a specific entity.
     *
     * @param e Entity ID
     * @param sig Bitmask representing which components this entity has.
     */
    void SetSignature(Entity e, const Signature& sig) {
        signatures[e] = sig;
    }

    /**
     * @brief Retrieves the signature of a specific entity.
     *
     * @param e Entity ID
     * @return Signature mask representing active components for this entity.
     */
    const Signature& GetSignature(Entity e) const {
        static Signature empty;
        auto it = signatures.find(e);
        return (it != signatures.end()) ? it->second : empty;
    }

    /**
     * @brief Updates the debug name of an existing entity.
     * 
     * @param e The entity ID whose name to update.
     * @param name The new name to assign to the entity.
     */
    void SetEntityName(Entity e, const std::string& name){
        for (auto& obj : entities){
            if (obj.id == e){
                obj.name = name;
                return;
            }
        }
    }

    /**
    * @brief Retrieves the mapping of all entity-to-signature data.
    *
    * @return Constant reference to internal signature table.
    */
    const std::unordered_map<Entity, Signature>& GetAllSignatures() const {
        return signatures;
    }

private:
    Entity nextId = 1;  // start at 1 for clarity, everytime we create a new entity, this +1
    std::vector<GameObject> entities;//we hold all the game records in here
    std::unordered_map<Entity, Signature> signatures;
};