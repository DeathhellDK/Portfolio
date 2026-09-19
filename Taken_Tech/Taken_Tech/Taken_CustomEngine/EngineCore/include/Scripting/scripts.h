#pragma once
/**
 * @file     scripts.h
 * @author   Jethro Sung
 * @email    sung.h
 * @date     2025-11-7
 *
 * @brief    Declares the IScript base interface for runtime script behaviors.
 *
 * IScript provides virtual lifecycle hooks allowing gameplay code to be
 * attached to entities in a modular and data-driven way. The ScriptSystem
 * invokes these functions automatically each frame for scripted entities.
 *
 * Lifecycle:
 *  - OnStart  -> Called once on first Update after script attachment
 *  - OnUpdate -> Called every frame while script is active
 *
 * @note Scripts should not directly access global state; always operate using
 *       IComponentContext for safe ECS queries and modifications.
 */

#include <cstdint>
class IComponentContext;
using Entity = std::uint32_t;

/**
 * @class IScript
 * @brief Base interface for attachable runtime scripts.
*/
class IScript {
public:
    virtual ~IScript() {}
    virtual void OnStart(IComponentContext& /*ctx*/, Entity /*e*/) {}
    virtual void OnUpdate(IComponentContext& /*ctx*/, Entity /*e*/, float /*dt*/) {}
};