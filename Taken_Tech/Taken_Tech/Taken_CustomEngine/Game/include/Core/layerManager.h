/**
 * @file      layerManager.h
 * @author    Jethro Sung
 * @email     sung.h
 * @date      2026-01-30
 *
 * @brief     Declares LayerManager, a utility container that tracks which
 *            entities belong to which engine layer.
 *
 * LayerManager provides an efficient O(1) mechanism for:
 *  - Adding entities to a layer
 *  - Removing entities from a layer
 *  - Moving entities between layers
 *
 * Each layer stores a "bucket" containing:
 *  - a dense vector of Entity handles (fast iteration)
 *  - an index map (Entity -> vector index) to support O(1) erase via swap-pop
 *
 * This is designed to work together with eng::Layering, where Layering controls
 * the enable/disable state (update/render) of each layer, while LayerManager
 * controls membership (which entities belong to each layer).
 *
 * Systems such as rendering, physics, editor overlay, and gameplay logic can
 * iterate only the entities of enabled layers, avoiding expensive full-world
 * scans.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#pragma once
#include <vector>
#include <array>
#include <unordered_map>
#include <cstddef>
#include "Core/layering.h"

#include "Core/entitymanager.h" 

class LayerManager
{
public:
    /**
     * @brief Add an entity to a specific layer.
     * If the entity is already in another layer, it will be moved.
     * @param layer The target layer ID.
     * @param e The entity to add.
     */
    void Add(eng::LayerId layer, Entity e);

    /**
     * @brief Remove an entity from its current layer.
     * @param e The entity to remove.
     */
    void Remove(Entity e);

    /**
     * @brief Move an entity to a new layer.
     * @param newLayer The destination layer ID.
     * @param e The entity to move.
     */
    void Move(eng::LayerId newLayer, Entity e);

    /**
     * @brief Get a read-only list of all entities in a given layer.
     * @param layer The layer ID to query.
     * @return A constant reference to the vector of entities.
     */
    const std::vector<Entity>& Entities(eng::LayerId layer) const;

    /**
     * @brief Query the current layer of an entity.
     * @param e The entity to query.
     * @return The layer ID of the entity, or Background if unknown.
     */
    eng::LayerId GetLayer(Entity e) const;

private:
    struct Bucket
    {
        std::vector<Entity> entities;
        std::unordered_map<Entity, std::size_t> index; // Entity -> position in vector

        void Add(Entity e);
        void Remove(Entity e);
        bool Contains(Entity e) const;
    };

    static constexpr std::size_t Idx(eng::LayerId id) { return static_cast<std::size_t>(id); }

    std::array<Bucket, static_cast<std::size_t>(eng::LayerId::Count)> buckets_{};
    std::unordered_map<Entity, eng::LayerId> entityToLayer_;
};