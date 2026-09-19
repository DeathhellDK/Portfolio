/**
* @file ForceSystem.hpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril,zhijie.lim
* @co-author	Lim Zhi Jie
* @date 2025-10-29
*
* @brief Declaration of global functions and data structures used to intialize
* and execute the physics force system in the engine. It is used to integrates
* varuous physics components such as mass and speed through ForceManager and
* EntityForceProxy
*
* @version 1.0
* - Declearation of global functions that will be used to integrates
* various physics components.
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#ifndef FORCE_SYSTEM_HPP
#define FORCE_SYSTEM_HPP

#include <unordered_map>
#include "Physics/PhysicsSpeedComponent.hpp"
#include "Physics/PhysicsMassComponent.hpp"
#include "Core/componentcontext.h"
#include "Physics/ForceManager.hpp"

class EntityForceProxy;  /// Forward declaration to EntityForceProxy class

/**
* @brief Global pointer to the current active EntityForceProxy instance
* 
* @note This proxy acts as the interface between the Forcemanager and the
* entity-component system, allowing forces to be accessed and modified without
* directly link to a specific compoennt logic
*/
extern EntityForceProxy* g_entityForceProxy;

/**
* @breif Temporary storage for entity SpeedComponent
* 
* @note This unordered map stores SpeedComponent data that is key by
* entity ID. 
*/
extern std::unordered_map<Entity, SpeedComponent> temp_spdComponent;

/**
* @brief Temporary storage for entity MassComponents
* 
* @param This unorderd map stores MassComponent data that is 
* key by entity ID
*/
extern std::unordered_map<Entity, MassComponent> temp_massComponent;

/**
* @brief Initialise the ForceSystem within the component that is
* passing through the parameter
* 
* @param componentContext Reference to the component context managing 
* all entities and components
*/
void InitForceSystem(IComponentContext& componentContext);

/**
* @brief Execute the force update for all for all the entities in
* the system
* 
* @param componentContext Reference to the active component context
* @param dt Delta time in seconds
*/
void ExecuteForceSystem(IComponentContext& componentContext, float dt);

/**
 * @brief Shuts down the force system and releases associated resources.
 */
void ShutdownForceSystem();

#endif // !FORCE_SYSTEM_HPP