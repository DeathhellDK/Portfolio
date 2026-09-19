/**
* @file EntityForceProxy.hpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-10-27
*
* @brief Delcaration of EntityForceProxy class which perform as a adapter
* for physics and ECS components and its respective component that will
* be applying the forces to the entity
*
*
* @update version history
*
* @version 1.0:
* - Created a class EntityForceProxy and define its function that will use
* to get both speed and mass which will then be using it to apply the force
* to the entity
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#ifndef ENTITYFORCEPROXY_HPP
#define ENTITYFORCEPROXY_HPP

#include "Physics/ForceProxy.hpp"
#include "Core/componentcontext.h"
#include "Core/transform.h"

/**
* @class EntityForceProxy
* @brief Provide an interface to modify phyiscs-based entity components
*/
class EntityForceProxy : public ForceProxy {
public:

	/**
	* @brief Constructs the EntityForceProxy with ECS reference
	* 
	* @param context Reference to the ECS component context for transformation
	* @param speed Refernece to entity speed component map
	* @param mass Reference to entity mass componentn map
	*/
	EntityForceProxy(IComponentContext& context, std::unordered_map<Entity, SpeedComponent>& speed, 
		std::unordered_map<Entity, MassComponent>& mass);

	/**
	* @brief Retrieves the transform of a given entity
	* 
	* @param e Entity ID
	*/
	Transform* GetTransform(Entity e) override;

	/**
	* @brief Retrieves the SpeedComponent of a given entity
	* 
	* @param e Entity ID
	*/
	SpeedComponent* GetSpeedComponent(Entity e) override;

	/**
	* @brief Checks if the given entity has a SpeedComponent
	* 
	* @param e Entity ID
	*/
	bool HasSpeedComponent(Entity e) override;

	/**
	* @brief Retrieves a list of all entities with a SpeedComponent
	*/
	std::vector<Entity> GetAllEntitiesWithSpeedComponent() const override;

	/**
	* @brief Retrieves the MassComponent of a given entity
	*
	* @param e Entity ID
	*/
	MassComponent* GetMassComponent(Entity e) override;

	/**
	* @brief Checks if the given entity has a MassComponent
	*
	* @param e Entity ID
	*/
	bool HasMassComponent(Entity e) override;

	/**
	* @brief Apply physics force to the specify entity
	* 
	* @param e Entity ID
	* @param force Force vector to apply
	*/
	void triggerForce(Entity e, const Vector2& force) override;

	/**
	* @brief Updates physics for all entities based on 
	* current speed and friction
	* 
	* @param dt Delta time in seconds for frame update
	*/
	bool useForcePhysics(float dt) override;

private:
	/// Reference to ECS context
	IComponentContext& componentContext;

	/// Map of entity soeed data 
	std::unordered_map<Entity, SpeedComponent>& speedComponentMap;
	
	/// Map of entity mass data
	std::unordered_map<Entity, MassComponent>& massComponentMap;
};

#endif // !ENTITYFORCEPROXY_HPP