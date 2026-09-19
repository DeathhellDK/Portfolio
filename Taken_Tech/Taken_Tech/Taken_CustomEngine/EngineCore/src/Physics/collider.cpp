/**
* @file collider.cpp
* @author Jethro Sung
* @co_author Tan Wei Liang Terril
* @email sung.h, t.weiliangterril
* @date 2025-09-24
*
* @brief Implementation of the collider class for handling collision detection
* between shapes
*
* @version 1.1
*
* @update version history
* @version 1.0 - Implementation of function for basic shape collision detection (rectangle, triangle, circle).
* @version 1.1 - Implementation function for OBB using SAT.
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "Physics/collider.h"
#include "Physics/collision.hpp"
#include "Core/transform.h"

/**
* @brief Checks Collision between two collider
* 
* @param other Other collider to test aganist
* @param posA Position of the first collider
* @param posB Position of the second collider
* @param rotationA Rotation of the first collider
* @param rotationB Rotation of the second collider
*/
bool Collider::CheckCollision(const Collider& other, const Vector2& posA, const Vector2& posB, float rotationA, float rotationB) const {
	switch (type) {
		case ColliderType::Box:
			if (other.type == ColliderType::Box)
			{
				if(rotationA != 0.0f || rotationB != 0.0f)
				{
					//Oriented Bounding Box collision check
					return Collision_Mgr::checkCollisionOBBToOBB(posA, size * 0.5f, rotationA, posB, other.size * 0.5f, rotationB);
				}
				else {
					//Rect with Rect
				// Keep in mind we hv to use const_cast here because the Collision_Mgr functions expect non-const references, so we need to cast away the constness of posA and posB, and thus
				//ensure that the function check Collision is able to work with the Collision_Mgr functions without any issues.
					return Collision_Mgr::checkCollisionRectToRect(posA, size.x, size.y, posB, other.size.x, other.size.y);
				}
				
			}
			else if (other.type == ColliderType::Circle)
			{
				//Rect with Circle
				//FOr future implementation
				return Collision_Mgr::checkCollisionRectToCircle(posA, size.x, size.y, posB, other.size.x);
			}
			else if (other.type == ColliderType::Triangle)
			{
				//Rect with Triangle
				//For future implementation
				return Collision_Mgr::checkCollisionRectToTrig(posA, size.x, size.y, triPt1, triPt2, triPt3);
			}
			break;
		case ColliderType::Circle:
			if (other.type == ColliderType::Box)
			{
				//Circle with Rect
				//For future implementation
				return Collision_Mgr::checkCollisionRectToCircle(posA, size.x, size.y, posB, other.size.x);
			}
			else if (other.type == ColliderType::Triangle)
			{
				//Circle with Triangle
				//For future implementation
				return Collision_Mgr::checkCollisionCircleToTrig(posA, size.x, triPt1, triPt2, triPt3);
			}
			else if (other.type == ColliderType::Circle)
			{
				//Circle with Circle
				return Collision_Mgr::checkColllisionCircleToCricle(posA, size.x, posA, other.size.x);
			}
			break;
		case ColliderType::Triangle:
			if (other.type == ColliderType::Box)
			{
				//Triangle With Box
				//For future implementation
				return Collision_Mgr::checkCollisionRectToTrig(posA, size.x, size.y, triPt1, triPt2, triPt3);
			}
			else if (other.type == ColliderType::Circle)
			{
				//Triangle with Circle
				//For future implementation
				return Collision_Mgr::checkCollisionCircleToTrig(posA, size.x, triPt1, triPt2, triPt3);
			}
			else if (other.type == ColliderType::Triangle)
			{
				//Triangle with Triangle
				//return Collision_Mgr::checkCollisionTrigToTrig(
				//	//First Triangle pos + offset
				//	posA.x + triPt1.x, posA.y + triPt1.y,
				//	posA.x + triPt2.x, posA.y + triPt2.y,
				//	posA.x + triPt3.x, posA.y + triPt3.y,
				//	//Second Triangle pos + offset
				//	posB.x + other.triPt1.x, posB.y + other.triPt1.y,
				//	posB.x + other.triPt2.x, posB.y + other.triPt2.y,
				//	posB.x + other.triPt3.x, posB.y + other.triPt3.y
				//	);

				return Collision_Mgr::checkCollisionTrigToTrig(
					//First Triangle pos + offset
					Vector2(posA.x + triPt1.x, posA.y + triPt1.y),
					Vector2(posA.x + triPt2.x, posA.y + triPt2.y),
					Vector2(posA.x + triPt3.x, posA.y + triPt3.y),
					//Second Triangle pos + offset
					Vector2(posB.x + other.triPt1.x, posB.y + other.triPt1.y),
					Vector2(posB.x + other.triPt2.x, posB.y + other.triPt2.y),
					Vector2(posB.x + other.triPt3.x, posB.y + other.triPt3.y)
				);
			}
			break;
	}
	return false;
}