#pragma once
/**
 * @file    systemManager.h
 * @author  Jethro Sung
 * @email   sung.h
 * @date    2025-09-29
 *
 * @brief   Declares the ISystem interface and SystemManager class for ECS.
 *
 * The ECS architecture in this project uses systems to process specific
 * groups of components (e.g., rendering, collision, movement). This file
 * defines:
 * - `ISystem`: a base interface for all systems, with virtual Update and Draw.
 * - `SystemManager`: a container/owner of systems, responsible for updating
 *   and drawing all registered systems in the correct order.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without prior
 * written consent of DigiPen Institute of Technology is prohibited.
 */
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include "Graphics/renderer.h"
#include "Core/component.h"
#include <unordered_map>

// ============================================================================
// Base mixin providing ECS signature functionality to systems
// ============================================================================
class SystemBase {
    private:
        bool enabled = true;           ///< Whether this system is active
        Signature systemSignature;     ///< Bitmask describing the required components

    public:
        virtual ~SystemBase() = default;

        // -- Enable / disable control --
        void SetEnabled(bool state) noexcept { enabled = state; }
        bool IsEnabled() const noexcept { return enabled; }

        // -- Signature access --
        void SetSignature(const Signature& sig) noexcept { systemSignature = sig; }
        const Signature& GetSignature() const noexcept { return systemSignature; }

        // -- Utility: check if an entity matches this system's requirements
        bool Matches(const Signature& entitySig) const noexcept {
            return (entitySig & systemSignature) == systemSignature;
        }
};


 /**
  * @class ISystem
  * @brief Interface base class for all ECS systems.
  *
  * Provides a standard interface with `Update()` and `Draw()` so that
  * systems can be stored polymorphically in SystemManager.
  */
class ISystem {
public:
    // Virtual destructor for polymorphic base.
    virtual ~ISystem() = default;

    /**
     * @brief Updates system logic.
     *
     * Default implementation does nothing. Derived systems should override
     * this to process their associated components.
     *
     * @param dt Delta time (in seconds) since the last update call.
     */
    virtual void Update(float /*dt*/) {}

    /**
     * @brief Draws system content to the screen.
     *
     * Default implementation does nothing. Derived systems should override
     * this to submit their render calls.
     *
     * @param renderer Renderer object used for drawing.
     */
    virtual void Draw(Renderer& /*renderer*/) {}


    /**
    * @brief Enables or disables this system.
    *
    * When disabled, the system will be skipped during Update and Draw calls.
    *
    * @param e True to enable, false to disable.
    */
    //void SetEnabled(bool e) { enabled = e; }

    /**
     * @brief Checks whether this system is enabled.
     *
     * @return True if the system is enabled, false if it is disabled.
     */
    //bool IsEnabled() const { return enabled; }

private:
    //bool enabled = true;
};

struct SystemTiming {
    std::string name;
    double lastUpdateMs = 0.0;
    double lastDrawMs = 0.0;
};


/**
 * @class SystemManager
 * @brief Manages a collection of systems in the ECS architecture.
 *
 * Owns all systems via unique_ptr, ensuring proper cleanup. Provides
 * centralized methods to update and draw all registered systems each frame.
 * When we create a new game object, we also create its components and attach. So essentially, 
 * each entity is just an ID that ties its components together. When we set up a system, we pass in references to the components it needs to operate on.
 */
class SystemManager {
    std::vector<std::unique_ptr<ISystem>> systems;
    std::vector<SystemTiming> timings;

public:
    /**
    * @brief Adds a new system of type T to the manager.
    *
    * Constructs the system with forwarded arguments, stores it in the
    * internal collection, and returns a reference to it.
    *
    * @tparam T System type (must derive from ISystem).
    * @tparam Args Variadic argument types for the system constructor.
    * @param args Arguments forwarded to the system constructor.
    * @return Reference to the newly created system.
    */
    template <typename T, typename... Args>
    T& AddSystem(Args&&... args) {
        auto sys = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *sys;
        timings.push_back({ typeid(T).name(), 0.0, 0.0 });
        systems.push_back(std::move(sys));
        return ref;
    }


    /**
    * @brief Updates all registered systems.
    *
    * Calls `Update(dt)` on each system in the order they were added.
    *
    * @param dt Delta time (in seconds) since the last update.
    */
    void UpdateAll(float dt) {
        for (size_t i = 0; i < systems.size(); ++i) {
            auto* sys = systems[i].get();
            if (auto* base = dynamic_cast<SystemBase*>(sys); !base || base->IsEnabled()) {
                auto start = std::chrono::high_resolution_clock::now();
                sys->Update(dt);
                auto end = std::chrono::high_resolution_clock::now();
                timings[i].lastUpdateMs =
                    std::chrono::duration<double, std::milli>(end - start).count();
            }
        }
    }

    /**
     * @brief Draws all registered systems.
     *
     * Calls `Draw(renderer)` on each system in the order they were added.
     *
     * @param renderer Renderer used to submit draw calls.
     */
    void DrawAll(Renderer& renderer) {
        for (size_t i = 0; i < systems.size(); ++i) {
            auto* sys = systems[i].get();
            if (auto* base = dynamic_cast<SystemBase*>(sys); !base || base->IsEnabled()) {
                auto start = std::chrono::high_resolution_clock::now();
                sys->Draw(renderer);
                auto end = std::chrono::high_resolution_clock::now();
                timings[i].lastDrawMs =
                    std::chrono::duration<double, std::milli>(end - start).count();
            }
        }
    }

    // ------------------------------------------------------------------------
    // Retrieve all entities whose signatures match the system's requirements
    // ------------------------------------------------------------------------
    std::vector<Entity> GetMatchingEntities(
        const std::unordered_map<Entity, Signature>& allSignatures,
        const Signature& systemSig) const
    {
        std::vector<Entity> matches;
        matches.reserve(allSignatures.size());

        for (auto it = allSignatures.begin(); it != allSignatures.end(); ++it) {
            Entity entity = it->first;
            const Signature& sig = it->second;

            // Entity matches if it has all components required by the system
            if ((sig & systemSig) == systemSig)
                matches.push_back(entity);
        }
        return matches;
    }

    const std::vector<SystemTiming>& GetTimings() const {
        return timings;
    }
};