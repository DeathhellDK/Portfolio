/**
* @file ForceManager.cpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-10-27
*
* @brief Implementation of ForceManager class functions
*
* @version 1.0
*
* @update version history
* @version 1.0 - Implementation of function ForceManager class
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Physics/ForceManager.hpp"
#include "Core/transform.h"
#include "Physics/PhysicsSpeedComponent.hpp"
#include "Physics/PhysicsMassComponent.hpp"
#include <algorithm>



/**
* @brief Updates all entities by applying forces,
* calculating acceleration, updating velocity,
* and moving the entity in the world.
*
* @param dt Delta time in seconds for the current frame
*/
void ForceManager::Update(float dt)
{
	/// Get all entities with speedComponent
	auto forceEntity = forceProxy.GetAllEntitiesWithSpeedComponent();

	for (Entity e : forceEntity) {
		/// Skip if entity do not has a speedComponent
		if (!forceProxy.HasSpeedComponent(e)) {
			continue;
		}

		/// Get the Transform compoonent
		Transform* transform = forceProxy.GetTransform(e);

		/// Get the speed compoonent
		SpeedComponent* speedComp = forceProxy.GetSpeedComponent(e);

		/// Get the mass compoonent
		MassComponent* massComp = forceProxy.HasMassComponent(e) ? forceProxy.GetMassComponent(e) : nullptr;

		/// Skip the enetity if transform or speed component is missing 
		if (!transform || !speedComp) {
			continue;
		}

		/// Update if entities that are dynamic
		if (speedComp->isDynamic) {


			/// Current velocity
			Vector2 vel = speedComp->speed;
			/// use mass from massComponent or default value
			float mass = massComp ? massComp->mass : 2.0f; /// default mass to 1.0f if no mass component
			Vector2 accel = vel / mass;

			if (accel.Length() > speedComp->maxSpeed)
				accel = accel.Normalized() * speedComp->maxSpeed;

			/// Upadte speed baesd on acceleration
			speedComp->speed += accel * dt;

			if (speedComp->speed.Length() > speedComp->maxSpeed)
				speedComp->speed = speedComp->speed.Normalized() * speedComp->maxSpeed;
			

			/// Update the entity position base on the new updated speed
			transform->SetPosition(transform->GetPosition() + speedComp->speed * dt);



		}
	}
}
