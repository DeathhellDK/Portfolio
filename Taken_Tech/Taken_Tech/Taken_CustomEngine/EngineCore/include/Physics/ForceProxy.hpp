/**
* @file ForceProxy.hpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-10-27
*
* @brief Declaration of ForceProxy class which is used as an interface
* for accessing and manipulating physics-related component and entities
*
* @version 1.0
* - Created a class ForceManager class and its fuctions that will acts
* as an bridge between the physics system and the game entity-component
* structure
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#ifndef FORCE_PROXY_HPP
#define FORCE_PROXY_HPP

#include "Core/component.h"
#include "Math/vect2.h"
#include "Physics/PhysicsSpeedComponent.hpp"
#include "Physics/PhysicsMassComponent.hpp"
#include <vector>

class Transform; /// Forward declaration to Transform class

/**
* @class ForceProxy
* 
* @brief Abstract interface that defines the access to
* the physics components and force operations
*/
class ForceProxy {
public:

	/**
	* @brief Virtual destructor for safe cleanup in derived classes
	*/
	virtual ~ForceProxy() = default;

	/**
	* @breif Get the Transform component of the given entity
	* 
	* @param e Entity identifier
	*/
	virtual Transform* GetTransform(Entity e) = 0;

	/// --- Speed Component ----

	/**
	* @brief Get the SpeedComponent of the specified entity
	* 
	* @param e Entity identifier
	*/
	virtual SpeedComponent* GetSpeedComponent(Entity e) = 0;

	/**
	* @brief Check if the given entity possesses a SpeedComponent
	*
	* @param e Entity identifier
	*/
	virtual bool HasSpeedComponent(Entity e) = 0;

	/**
	* @brief Get all entities that hase a SpeedComponent
	*/
	virtual std::vector<Entity> GetAllEntitiesWithSpeedComponent() const = 0;

	/// ---- Mass Component ----

	/**
	* @brief Get all the MassComponent of the specified entity
	* 
	* @param e Entity identifier
	*/
	virtual MassComponent* GetMassComponent(Entity e) = 0;

	/**
	* @brief Check if the given entity possesses a MassComponent
	*
	* @param e Entity identifier
	*/
	virtual bool HasMassComponent(Entity e) = 0;

	/// ---- Force ----

	/**
	* @brief Applies force to the specified entity
	* 
	* @param e Entity identifier
	* @param force 2D vector representing the force to be applied
	*/
	virtual void triggerForce(Entity e, const Vector2& force) = 0;

	/**
	* @brief Updates all forces in the physics system for the current frame.
	* 
	* @param dt Delta Time in seconds
	*/
	virtual bool useForcePhysics(float dt) = 0;
};



#endif // !FORCE_PROXY_HPP
