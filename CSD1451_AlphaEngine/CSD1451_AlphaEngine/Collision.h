/* Start Header ************************************************************************/
/*!
\file		Collision.h
\author		Jia Hao
\date		March, 20, 2025
\brief      Declares structures, enumerations, and functions for collision detection and boundary rendering.

This header defines collider-related types such as Collider, TriangleCollider, and their associated enums. 
It declares functions for initializing and updating colliders, as well as detecting collisions between 
circles, rectangles, and triangles. It also declares functions to render the game screen boundaries.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef Collision_H
#define Collision_H
 
#include "AEEngine.h"

//enum for ColliderType
typedef enum ColliderType{
	PROJECTILE,
	CIRCLE,
	//MELEE_HITBOX,
	PLAYER_HITBOX
}ColliderType;

//struct for rectangle
/*typedef struct rectangle {
	float width;
	float height;
} rectangle;*/

//struct for circle
typedef struct circle{
	float radius;
} circle;
 
//enum for type
typedef enum ShapeColliderType {
	//COLLIDER_RECTANGLE,
	COLLIDER_CIRCLE
}ShapeCollider;


// union for shape
typedef union ColliderShape {
	//rectangle Rectangle;
	circle Circle;
}ColliderShape;

//struct for Collider 
typedef struct Collider {
	ColliderShape shape;
	AEVec2 position;
	ShapeColliderType ShapeType;
	ColliderType type;
} Collider;

// Hao Peng's function for options buttons, dont change pls
// Enum for triangle collision types
typedef enum {
	TRIANGLE_POINT,
	TRIANGLE_TRIANGLE
} TriangleCollisionType;

// Triangle collision structure
typedef struct {
	AEVec2 v1;     // First vertex
	AEVec2 v2;     // Second vertex
	AEVec2 v3;     // Third vertex
	TriangleCollisionType type;
} TriangleCollider;


//int checkRectRectCollision(const Collider& rect1, const Collider& rect2);
int checkCircleCircleCollision(const Collider& circle1, const Collider& circle2);
//int checkOffScreen(Collider projectile);
//int checkCircleRectCollision(const Collider& circle, const Collider& rect);
//Collider CreateRectCollider(float width, float height);
Collider CreateCircleCollider(float radius);
Collider CreateCollider(float radius, AEVec2* position, ColliderType type);
void UpdateCollider(Collider* col, AEVec2 position, ColliderType type);
int Collide(const Collider& col1, const Collider& col2);
// Hao Peng's function for options buttons and boundary, dont change pls
int CheckTrianglePointCollision(TriangleCollider* triangle, AEVec2* point);
int CheckTriangleTriangleCollision(TriangleCollider* triangle1, TriangleCollider* triangle2);
void Left_Boundary(AEGfxVertexList* Mesh);
void Right_Boundary(AEGfxVertexList* Mesh);
void Top_Boundary(AEGfxVertexList* Mesh);
void Bottom_Boundary(AEGfxVertexList* Mesh);

#endif