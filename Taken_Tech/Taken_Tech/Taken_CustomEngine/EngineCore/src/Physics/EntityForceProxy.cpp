/**
* @file EntityForceProxy.cpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-10-30
*
* @brief Implementation of EntityForceProxy class functions
*
* @version 1.0
*
* @update version history
* @version 1.0 - Implementation of function EntityForceProxy class
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Physics/EntityForceProxy.hpp"
#include "Math/vect2.h"

/**
* @breif Constructs the EntityForceProxy with reference to ECS context and component maps
*
* @param context Reference to the ECS component context
* @param speed Reference to the map of entity speed components
* @param mass Reference to the map of entity mass components
*/
EntityForceProxy::EntityForceProxy(IComponentContext& context, std::unordered_map<Entity, SpeedComponent>& speed, std::unordered_map<Entity, MassComponent>& mass)
	: componentContext(context), speedComponentMap(speed), massComponentMap(mass) {
}

/**
* @breif Retrieve the Transform component of a given entity
*
* @param e Entity ID
*/
Transform* EntityForceProxy::GetTransform(Entity e) {
	return componentContext.GetTransform(e);
}

/**
*  @brief Get the SpeedComponent of a given entity
*
 * @param e Entity ID
*/
SpeedComponent* EntityForceProxy::GetSpeedComponent(Entity e) {
	auto it = speedComponentMap.find(e);
	return (it != speedComponentMap.end()) ? &(it->second) : nullptr;
}

/**
 * @brief Check if the entity has a SpeedComponent
 *
 * @param e Entity ID
 */
bool EntityForceProxy::HasSpeedComponent(Entity e) {
	return speedComponentMap.find(e) != speedComponentMap.end();
}

/**
 * @brief Get all entities that exist in the ECS context
 */
std::vector<Entity> EntityForceProxy::GetAllEntitiesWithSpeedComponent() const {
	return componentContext.GetEntities();
}

/**
 * @brief Get the MassComponent of a given entity
 *
 * @param e Entity ID
 */
MassComponent* EntityForceProxy::GetMassComponent(Entity e) {
	auto it = massComponentMap.find(e);
	return (it != massComponentMap.end()) ? &(it->second) : nullptr;
}

/**
 * @brief Check if the entity has a MassComponent
 *
 * @param e Entity ID
 */
bool EntityForceProxy::HasMassComponent(Entity e) {
	return massComponentMap.find(e) != massComponentMap.end();
}


/**
 * @brief Apply a force to a given entity
 *
 * @param e Entity ID
 * @param force Vector force to apply
 */
void EntityForceProxy::triggerForce(Entity e, const Vector2& force) {
	auto speed = GetSpeedComponent(e);
	auto mass = GetMassComponent(e);

	if (speed && mass && mass->mass > 0.0f) {
		speed->speed += force / mass->mass;
		Vector2 acceleration = force / mass->mass;
		speed->speed += acceleration;
	}
}

/**
 * @brief Update all entities' physics based on current speed, friction, and max speed
 *
 * @param dt Delta time
 */
bool EntityForceProxy::useForcePhysics(float dt) {
	// Loop through all entities that have a SpeedComponent
	for (auto e : GetAllEntitiesWithSpeedComponent())
	{
		auto* spd = GetSpeedComponent(e);
		auto* mass = GetMassComponent(e);
		auto* tr = GetTransform(e);

		if (!spd || !mass || !tr)
			continue;

		// Retrieve physics properties (stored in each entity's SpeedComponent)
		float maxSpeed = spd->maxSpeed;

		// Clamp the current speed to max speed
		float speedMag = spd->speed.Length();
		if (speedMag > maxSpeed) {
			spd->speed = spd->speed.Normalized() * maxSpeed;
		}

		// Apply friction only when not moving (i.e. no external force this frame)
		if (spd->speed.LengthSqd() > 0.0f)
		{
			/// Movement direction
			Vector2 dir = spd->speed.Normalized();

			/// Friction acts in opposite direction
			Vector2 frictionDir = { -dir.x, -dir.y };

			/// Friction Force
			Vector2 frictionForce = frictionDir * spd->friction * dt;

			// Stop completely if friction overcomes current velocity
			if (frictionForce.Length() > spd->speed.Length())
				spd->speed = Vector2{ 0.0f, 0.0f };
			else
				spd->speed += frictionForce;
		}

		// Update transform position using final velocity
		tr->SetPosition(tr->GetPosition() + (spd->speed * dt));
	}
	//(void)dt;
	return true;
}


/// Loop through all the entities that have speed component
/*for (auto& entitypair : speedComponentMap) {

	/// Get the ID
	Entity e = entitypair.first;

	/// Get the transform component
	auto* trans = GetTransform(e);

	/// Get the speed component
	auto& speed = entitypair.second;

	/// Check if is moving
	if (speed.speed.LengthSqd() > 0.0f) {
		/// Movement direction
		Vector2 dir = speed.speed.Normalized();

		/// Friction acts in opposite direction
		Vector2 frictionDir = { -dir.x, -dir.y };

		/// Friction Force
		Vector2 frictionForce = frictionDir * speed.friction * dt;

		/// Check if the friction is stronger than the current speed
		if (frictionForce.Length() > speed.speed.Length()) {
			/// Stop the movement
			speed.speed = Vector2(0.0f, 0.0f);
		}
		else {
			/// Reduce the speed until it come to a complete stop
			speed.speed += frictionForce;
		}
	}

	/// Clamp the entity speed to its maximum speed
	if (speed.speed.Length() > speed.maxSpeed) {
		Vector2 dir = speed.speed.Normalized();
		speed.speed = dir * speed.maxSpeed;
	}

	/// Update the entity position based on speed and delta time
	if (trans) {
		trans->SetPosition(trans->GetPosition() + speed.speed * dt);
	}
}
return true;*/

//const float TELEPORT_THRESHOLD = 1000.f; // adjust based on your world scale

//for (auto& entityPair : speedComponentMap) {
//	Entity e = entityPair.first;
//	auto& speed = entityPair.second;
//	auto* trans = GetTransform(e);

//	// Compute friction
//	Vector2 dir = (speed.speed.LengthSqd() > 0.0f) ? speed.speed.Normalized() : Vector2{ 0.f, 0.f };
//	Vector2 frictionDir = { -dir.x, -dir.y };
//	Vector2 frictionForce = frictionDir * speed.friction * dt;

//	if (frictionForce.Length() > speed.speed.Length()) {
//		speed.speed = Vector2{ 0.f, 0.f };
//	}
//	else {
//		speed.speed += frictionForce;
//	}

//	// Clamp to max speed
//	if (speed.speed.Length() > speed.maxSpeed) {
//		speed.speed = speed.speed.Normalized() * speed.maxSpeed;
//	}

//	// Update position and handle teleport detection
//	if (trans) {
//		Vector2 delta = trans->GetPosition() - speed.prevPosition;
//		if (delta.LengthSqd() > TELEPORT_THRESHOLD) {
//			speed.speed = Vector2{ 0.f, 0.f }; // reset speed if teleported
//		}

//		trans->SetPosition(trans->GetPosition() + speed.speed * dt);
//		speed.prevPosition = trans->GetPosition(); // always sync prevPosition
//	}
//}

//return true;
