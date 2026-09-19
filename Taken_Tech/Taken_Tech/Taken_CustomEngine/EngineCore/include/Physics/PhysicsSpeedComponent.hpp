/**
* @file PhysicsSpeedComponent.hpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-10-30
*
* @brief Header file for physics speed component that will be used to
* store speed information for the entity,
*
* @version 1.0
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#ifndef PHYSICS_SPEED_COMPONENT_HPP
#define PHYSICS_SPEED_COMPONENT_HPP

#include "Core/component.h"	
#include "Math/vect2.h"

/**
* @struct SpeedComponent
* @brief A component that holds speed information for an entity.
* This component contains a 2D vector representing the speed and a boolean flag indicating whether the entity is dynamic.
*/
struct SpeedComponent : public Component {
	Vector2 speed{ 0.0f, 0.0f };
	/*Vector2 prevPosition{ 0.f, 0.f };*/
	float maxSpeed{ 200.0f }; // default max speed, can be overridden
	float friction{ 400.0f }; // default friction, can be overridden

	bool isDynamic;
	bool roomSpeed{ false };
	bool outsideSpeed{ true };

	/**
	* @brief This is a custom default contructor that initialises the component
	* with an invalid entity (empty component).
	*
	* This is needed as it does not work with the base component default constructor.
	*/
	SpeedComponent() : Component(INVALID_ENTITY), speed(0.0f, 0.0f), isDynamic(true) {}

	/**
	* @brief Constructs a SpeedComponent with the specified owner entity, initial speed, and dynamic flag.
	*/
	explicit SpeedComponent(Entity ownerId, const Vector2& initialSpeed = {},
		bool dynamic = true)
		: Component(ownerId), speed(initialSpeed), isDynamic(dynamic) {
	}


	/*struct PlayerPresetTag {};

	SpeedComponent(Entity ownerId, PlayerPresetTag)
		: Component(ownerId)
	{
		speed = { 0, 0 };
		maxSpeed = 90.0f;
		friction = 400.0f;
		isDynamic = true;
	}*/
};

#endif // !PHYSICS_SPEED_COMPONENT_HPP