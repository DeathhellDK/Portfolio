/**
 * @file      layerManager.cpp
 * @author    Jethro Sung
 * @email     sung.h
 * @date      2026-01-30
 *
 * @brief     Implements LayerManager, an O(1) entity-to-layer membership
 *            tracker used by the engine layering system.
 *
 * LayerManager maintains:
 *  - One Bucket per eng::LayerId (dense vector of entities + index map)
 *  - A global entityToLayer_ map for fast reverse lookup (Entity -> LayerId)
 *
 * Each Bucket supports O(1) insert and O(1) removal using a swap-pop strategy:
 *  - Removing an entity swaps the last element into the removed slot, updates
 *    its index, then pops the vector back.
 *
 * Public operations:
 *  - Add(layer, e): inserts entity into a layer; if already tracked, normalizes
 *    by moving instead.
 *  - Remove(e): removes entity from its current layer (if tracked).
 *  - Move(newLayer, e): moves entity between layers in O(1), or treats the call
 *    as Add() if the entity is not tracked yet.
 *  - Entities(layer): returns the dense vector for fast iteration.
 *  - GetLayer(e): returns current layer, defaulting to Background if unknown.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include "Core/layerManager.h"

void LayerManager::Bucket::Add(Entity e)
{
    if (index.find(e) != index.end()) return;
    index[e] = entities.size();
    entities.push_back(e);
}

void LayerManager::Bucket::Remove(Entity e)
{
    auto it = index.find(e);
    if (it == index.end()) return;

    std::size_t i = it->second;
    std::size_t last = entities.size() - 1;

    if (i != last)
    {
        Entity swapped = entities[last];
        entities[i] = swapped;
        index[swapped] = i;
    }

    entities.pop_back();
    index.erase(it);
}

bool LayerManager::Bucket::Contains(Entity e) const
{
    return index.find(e) != index.end();
}

void LayerManager::Add(eng::LayerId layer, Entity e)
{
    // If it already exists somewhere, normalize by moving.
    auto it = entityToLayer_.find(e);
    if (it != entityToLayer_.end())
    {
        Move(layer, e);
        return;
    }

    buckets_[Idx(layer)].Add(e);
    entityToLayer_[e] = layer;
}

void LayerManager::Remove(Entity e)
{
    auto it = entityToLayer_.find(e);
    if (it == entityToLayer_.end()) return;

    eng::LayerId layer = it->second;
    buckets_[Idx(layer)].Remove(e);
    entityToLayer_.erase(it);
}

void LayerManager::Move(eng::LayerId newLayer, Entity e)
{
    auto it = entityToLayer_.find(e);
    if (it != entityToLayer_.end())
    {
        eng::LayerId oldLayer = it->second;
        if (oldLayer == newLayer) return;

        buckets_[Idx(oldLayer)].Remove(e);
        buckets_[Idx(newLayer)].Add(e);
        it->second = newLayer;
        return;
    }

    // If not tracked yet, treat it like Add.
    Add(newLayer, e);
}

const std::vector<Entity>& LayerManager::Entities(eng::LayerId layer) const
{
    return buckets_[Idx(layer)].entities;
}

eng::LayerId LayerManager::GetLayer(Entity e) const
{
    auto it = entityToLayer_.find(e);
    return (it != entityToLayer_.end()) ? it->second : eng::LayerId::Background;
}