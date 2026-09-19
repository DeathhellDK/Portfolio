/**
* @file ForceSystem.cpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril,zhijie.lim
* @co-author	Lim Zhi Jie
* @date 2025-10-27
*
* @brief Implementation of Force System function which initialise the
* EntityForceProxy and executes the force update for all physics entities
*
* @version 1.0
*
* @update version history
* @version 1.0 - Implementation of Force system function
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include <Physics/ForceSystem.hpp>
#include "Physics/EntityForceProxy.hpp"

/// Temporary storage for entity speed
std::unordered_map<Entity, SpeedComponent> temp_spdComponent;

/// Temporary storage for entity mass
std::unordered_map<Entity, MassComponent> temp_massComponent;

/// Global pointer to EntityForceProxy for access across the system
EntityForceProxy* g_entityForceProxy = nullptr;

/**
 * @brief Initializes the force system by creating the EntityForceProxy, 
 * ensuring that only one instance is created.
 *
 * @param componentContext Reference to the ECS component context
 */
void InitForceSystem(IComponentContext& componentContext){
	/// Create the EntityForceProxy if it hasn't been created
	if (!g_entityForceProxy)
		g_entityForceProxy = new EntityForceProxy(componentContext, temp_spdComponent, temp_massComponent);
}

/**
 * @brief Executes the force system update for the current frame.
 *
 * @param componentContext Reference to the ECS component context
 * @param dt Delta time in seconds for the frame
 */
void ExecuteForceSystem(IComponentContext& componentContext, float dt){
	/// Create a temporary proxy using current ECS context and component maps
	EntityForceProxy proxy(componentContext, temp_spdComponent, temp_massComponent);

	/// Create a ForceManager with the proxy
	ForceManager forceManager(proxy);

	/// Update all entities
	forceManager.Update(dt);
}

void ShutdownForceSystem()
{
	delete g_entityForceProxy;
	g_entityForceProxy = nullptr;

	temp_spdComponent.clear();
	temp_massComponent.clear();
}
