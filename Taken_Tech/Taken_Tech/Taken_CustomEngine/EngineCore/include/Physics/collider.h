/**
 * @file      collider.h
 * @author	  Tan Wei Liang Terril
 * @co-author Jethro Sung
 * @email	  t.weiliangterril, sung.h
 * @date	  04/04/26
 *
 * @brief     Declares the Collider component and supported collider shapes used
 *            by the engine collision system.
 *
 *            This component stores collision data for box, circle, and triangle
 *            shapes, along with trigger and burrow-state flags used by gameplay
 *            and physics logic when resolving or filtering interactions.
 */
#pragma once
#include "Core/component.h"
#include "Math/vect2.h"

enum class ColliderType {
	Box,
	Circle,
	Triangle
}; // we gonna use this to define the type of collider

class Collider : public Component {
	public:
	ColliderType type;
	Vector2 size; // For Box: width and height, For Circle: x = radius, y = unused
	Vector2 triPt1, triPt2, triPt3; // For Triangle collider, the three points of the triangle
	bool isTrigger; // If true, collider is a trigger and does not cause physical collisions
	bool isBurrowed; // If true, collider is currently burrowed (intangible)
	bool isPassableWhenBurrowed; // If true, burrowed entities can pass through this collider
	
	Collider(Entity ownerId, Vector2 s, bool trigger = false)
		: Component(ownerId), type(ColliderType::Box), size(s), isTrigger(trigger), isBurrowed(false), isPassableWhenBurrowed(false) {
	}

	Collider(Entity ownerId, float radius, bool trigger = false)
		: Component(ownerId), type(ColliderType::Circle), size(Vector2(radius, 0.f)), isTrigger(trigger), isBurrowed(false), isPassableWhenBurrowed(false) {
	}

	Collider(Entity ownerId, Vector2 p1, Vector2 p2, Vector2 p3, bool trigger = false)
		: Component(ownerId), type(ColliderType::Triangle), size(Vector2(0.f, 0.f)),
		triPt1(p1), triPt2(p2), triPt3(p3), isTrigger(trigger), isBurrowed(false), isPassableWhenBurrowed(false) {
	}

	/**
	 * @brief Checks for collision between this collider and another.
	 * @param other The other collider to check against.
	 * @param posA The world position of this collider.
	 * @param posB The world position of the other collider.
	 * @param rotationA The rotation of this collider.
	 * @param rotationB The rotation of the other collider.
	 * @return True if a collision is detected.
	 */
	bool CheckCollision(const Collider& other, const Vector2& posA, const Vector2& posB, float rotationA = 0.0f, float rotationB = 0.0f) const; //we gonna use this to check collision between two collider, 
	//we can know the obj cause of the ColliderType enum, allowing us to check the collision based on the type of collider
};