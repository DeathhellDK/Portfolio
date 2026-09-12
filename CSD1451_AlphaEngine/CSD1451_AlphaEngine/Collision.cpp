/* Start Header ************************************************************************/
/*!
\file		Audio.cpp
\author		Jia Hao
\date		March, 20, 2025
\brief      Implements core collision detection and screen boundary rendering.

This file provides functions to create and update different collider types 
(circle, rectangle, triangle), and check for collisions between them. It 
includes general collision logic for circle-circle, rectangle-rectangle, 
circle-rectangle, triangle-point, and triangle-triangle scenarios.

Additionally, this file draws the fixed boundaries (left, right, top, bottom) 
of the game screen using transformation matrices and mesh rendering utilities. 
These boundaries help visually define the playable area.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "Collision.h"
#include "AEEngine.h"

//create collider for rectangle
//create early before midterm but in the end did not apply to game as all entities are circle 
/*Collider CreateRectCollider(float width, float height) {
	Collider collider = {};
	collider.ShapeType = COLLIDER_RECTANGLE;
	collider.shape.Rectangle.width = width;
	collider.shape.Rectangle.height = height;
	return collider;
	//printf("Collider type is %d", collider.ShapeType);
}*/

//create collider for circle
Collider CreateCircleCollider(float radius) {
	//printf("Inside CreateCircleCollider: radius = %f\n", radius);
	Collider collider = {};
	collider.ShapeType = COLLIDER_CIRCLE;
	collider.shape.Circle.radius = radius;
	
	//printf("After Inside CreateCircleCollider: radius = %f\n", collider.shape.Circle.radius);
	return collider;
}

//the function will choose the corresponding create circle/rectangle collider function by the input colliderType
Collider CreateCollider(float radius, AEVec2* position, ColliderType type) {
	Collider col;
	switch (type)
	{
	case PROJECTILE:
		col = CreateCircleCollider(radius);
		break;
	case CIRCLE:
		col = CreateCircleCollider(radius);
		break;
	/*case MELEE_HITBOX:
		col = CreateRectCollider(w, h);
		break;*/
	case PLAYER_HITBOX:
		col = CreateCircleCollider(radius);
		break;
	default:
		//printf("Error: Invalid ColliderType %d\n", type);
		col = CreateCircleCollider(radius); // Assign a default shape to avoid undefined behavior.
		break;
	}
	col.type = type;
	col.position = *position;
	return col;
}

//update collider position
void UpdateCollider(Collider* col, AEVec2 position, ColliderType type) {
	
		col->position.x = position.x;
		col->position.y = position.y;
	
	col->type = type;
}



//function to check collision for rect and rect
//create early before midterm but in the end did not apply to game as all entities are circle
/*int checkRectRectCollision(const Collider& rect1, const Collider& rect2) {
	float rect1maxX = rect1.position.x + (rect1.shape.Rectangle.width/2.0f);
	float rect1maxY = rect1.position.y + (rect1.shape.Rectangle.height / 2.0f);
	float rect1minX = rect1.position.x - (rect1.shape.Rectangle.width / 2.0f);
	float rect1minY = rect1.position.y - (rect1.shape.Rectangle.height / 2.0f);

	float maxX = rect2.position.x + rect2.shape.Rectangle.width / 2.0f;
	float maxY = rect2.position.y + rect2.shape.Rectangle.height / 2.0f;

	float minX = rect2.position.x - rect2.shape.Rectangle.width / 2.0f;
	float minY = rect2.position.y - rect2.shape.Rectangle.height / 2.0f;
	//printf("rect1 x is: %f, rect1 x+w/2 is: %f\n", rect1minX, rect1maxX);
	//printf("rect1 y is: %f, rect1 y+h/2 is: %f\n", rect1minY, rect1maxY);
	//printf("rect2 x is: %f, rect2 x+w/2 is: %f\n", minX, maxX);
	//printf("rect2 y is: %f, rect2 y+h/2 is: %f\n", minY, maxY);

	if (rect1minX  < maxX &&
		rect1maxX  > minX &&
		rect1minY  < maxY &&
		rect1maxY  > minY)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}*/


//create early before midterm but in the end did not apply to game as all entities are circle
//check circle circle collision
int checkCircleCircleCollision(const Collider& circle1, const Collider& circle2)
{
	AEVec2 circle1Pos = circle1.position;
	AEVec2 circle2Pos = circle2.position;

	// Calculate the distance between circle centers
	float distance = AEVec2Distance(&circle1Pos, &circle2Pos);

	// Get the sum of both circle radii
	float combinedRadius = circle1.shape.Circle.radius + circle2.shape.Circle.radius;

	// If distance is less than combined radii, they collide
	return distance < combinedRadius ? 1 : 0;
}



//check circle rect collision
//create early before midterm but in the end did not apply to game as all entities are circle
/*int checkCircleRectCollision(const Collider& circle, const Collider& rect) {
	float rectLeft = rect.position.x - rect.shape.Rectangle.width / 2;
	float rectRight = rect.position.x + rect.shape.Rectangle.width / 2;
	float rectTop = rect.position.y - rect.shape.Rectangle.height / 2;
	float rectBottom = rect.position.y + rect.shape.Rectangle.height / 2;

	// Find the closest point on the rectangle to the circle
	float closestX = (circle.position.x < rectLeft) ? rectLeft :
		(circle.position.x > rectRight) ? rectRight :
		circle.position.x;

	float closestY = (circle.position.y < rectTop) ? rectTop :
		(circle.position.y > rectBottom) ? rectBottom :
		circle.position.y;

	// Compute distance between the circle's center and the closest point
	float distanceX = circle.position.x - closestX;
	float distanceY = circle.position.y - closestY;

	// Compute squared distance and compare with squared radius
	float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
	return distanceSquared < (circle.shape.Circle.radius * circle.shape.Circle.radius);
}*/



//this function decide which shape collision function to call by the 2 input collider

int Collide(const Collider& col1, const Collider& col2) {
	//if (col1.type == PROJECTILE) {
		//printf("bullet position is: (%f, %f)\n", col1.position.x, col1.position.y);
	//}

	//printf("enemy position is: (%f, %f)\n", col2.position->x, col2.position->y);
	// Circle vs Circle Collision
	if (col1.ShapeType == COLLIDER_CIRCLE && col2.ShapeType == COLLIDER_CIRCLE) {
		return checkCircleCircleCollision(col1, col2);
		//return AETestCircleToCircle(col1.position, col1.shape.Circle.radius,
		//	col2.position, col2.shape.Circle.radius);


	}
	// Rectangle vs Rectangle Collision
	/*else if (col1.ShapeType == COLLIDER_RECTANGLE && col2.ShapeType == COLLIDER_RECTANGLE) {
		return checkRectRectCollision(col1, col2);
	}*/
	// Fix: Only call checkCircleRectCollision if one is a rectangle
	/*else if ((col1.ShapeType == COLLIDER_CIRCLE && col2.ShapeType == COLLIDER_RECTANGLE) ||
		(col1.ShapeType == COLLIDER_RECTANGLE && col2.ShapeType == COLLIDER_CIRCLE)) {
		return checkCircleRectCollision(col1, col2);
	}*/
	return 0; // No collision detected
}

// Hao Peng' function for options buttons, dont change pls

 //Triangle point collision detection
int CheckTrianglePointCollision(TriangleCollider* triangle, AEVec2* point) {
	// Calculate denominator (area of the triangle)
	float denominator = ((triangle->v2.y - triangle->v3.y) * (triangle->v1.x - triangle->v3.x) +
		(triangle->v3.x - triangle->v2.x) * (triangle->v1.y - triangle->v3.y));

	// Calculate barycentric coordinates a, b, c
	float a = ((triangle->v2.y - triangle->v3.y) * (point->x - triangle->v3.x) +
		(triangle->v3.x - triangle->v2.x) * (point->y - triangle->v3.y)) / denominator;

	float b = ((triangle->v3.y - triangle->v1.y) * (point->x - triangle->v3.x) +
		(triangle->v1.x - triangle->v3.x) * (point->y - triangle->v3.y)) / denominator;

	float c = 1.0f - a - b; // The third barycentric coordinate

	// Return true if point is inside the triangle (all barycentric coordinates >= 0)
	return (a >= 0.0f) && (b >= 0.0f) && (c >= 0.0f);
}


// Triangle-triangle collision detection
int CheckTriangleTriangleCollision(TriangleCollider* triangle1, TriangleCollider* triangle2) {
	// Check if any vertex of triangle1 is inside triangle2
	if (CheckTrianglePointCollision(triangle2, &triangle1->v1) ||
		CheckTrianglePointCollision(triangle2, &triangle1->v2) ||
		CheckTrianglePointCollision(triangle2, &triangle1->v3)) {
		return 1; // Collision detected
	}

	// Check if any vertex of triangle2 is inside triangle1
	if (CheckTrianglePointCollision(triangle1, &triangle2->v1) ||
		CheckTrianglePointCollision(triangle1, &triangle2->v2) ||
		CheckTrianglePointCollision(triangle1, &triangle2->v3)) {
		return 1; // Collision detected
	}

	return 0; // No collision
}


#pragma region Left Boundary
void Left_Boundary(AEGfxVertexList* Mesh) {

	AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
	AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

	AEMtx33 Left_Boundary_transform;
	AEMtx33Identity(&Left_Boundary_transform);

	AEMtx33 Left_Boundary_spin;
	AEMtx33Rot(&Left_Boundary_spin, 0.0f);

	AEMtx33 Left_Boundary_scale;
	AEMtx33Scale(&Left_Boundary_scale, 20.0f, 960.0f);

	AEMtx33 Left_Boundary_translate;
	AEMtx33Trans(&Left_Boundary_translate, -840.0f, 0.0f);

	AEMtx33Concat(&Left_Boundary_transform, &Left_Boundary_spin, &Left_Boundary_scale);
	AEMtx33Concat(&Left_Boundary_transform, &Left_Boundary_translate, &Left_Boundary_transform);

	AEGfxSetTransform(Left_Boundary_transform.m);

	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
}
#pragma endregion

#pragma region Right Boundary
void Right_Boundary(AEGfxVertexList* Mesh) {

	AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
	AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

	AEMtx33 Right_Boundary_transform;
	AEMtx33Identity(&Right_Boundary_transform);

	AEMtx33 Right_Boundary_spin;
	AEMtx33Rot(&Right_Boundary_spin, 0.0f);

	AEMtx33 Right_Boundary_scale;
	AEMtx33Scale(&Right_Boundary_scale, 20.0f, 960.0f);

	AEMtx33 Right_Boundary_translate;
	AEMtx33Trans(&Right_Boundary_translate, 840.0f, 0.0f);

	AEMtx33Concat(&Right_Boundary_transform, &Right_Boundary_spin, &Right_Boundary_scale);
	AEMtx33Concat(&Right_Boundary_transform, &Right_Boundary_translate, &Right_Boundary_transform);

	AEGfxSetTransform(Right_Boundary_transform.m);

	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
}
#pragma endregion

#pragma region Top Boundary
void Top_Boundary(AEGfxVertexList* Mesh) {

	AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
	AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

	AEMtx33 Top_Boundary_transform;
	AEMtx33Identity(&Top_Boundary_transform);

	AEMtx33 Top_Boundary_spin;
	AEMtx33Rot(&Top_Boundary_spin, 0.0f);

	AEMtx33 Top_Boundary_scale;
	AEMtx33Scale(&Top_Boundary_scale, 1700.0f, 20.0f);

	AEMtx33 Top_Boundary_translate;
	AEMtx33Trans(&Top_Boundary_translate, 0.0f, 490.0f);

	AEMtx33Concat(&Top_Boundary_transform, &Top_Boundary_spin, &Top_Boundary_scale);
	AEMtx33Concat(&Top_Boundary_transform, &Top_Boundary_translate, &Top_Boundary_transform);

	AEGfxSetTransform(Top_Boundary_transform.m);

	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
}
#pragma endregion

#pragma region Bottom Boundary
void Bottom_Boundary(AEGfxVertexList* Mesh) {

	AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
	AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

	AEMtx33 Bottom_Boundary_transform;
	AEMtx33Identity(&Bottom_Boundary_transform);

	AEMtx33 Bottom_Boundary_spin;
	AEMtx33Rot(&Bottom_Boundary_spin, 0.0f);

	AEMtx33 Bottom_Boundary_scale;
	AEMtx33Scale(&Bottom_Boundary_scale, 1700.0f, 20.0f);

	AEMtx33 Bottom_Boundary_translate;
	AEMtx33Trans(&Bottom_Boundary_translate, 0.0f, -490.0f);

	AEMtx33Concat(&Bottom_Boundary_transform, &Bottom_Boundary_spin, &Bottom_Boundary_scale);
	AEMtx33Concat(&Bottom_Boundary_transform, &Bottom_Boundary_translate, &Bottom_Boundary_transform);

	AEGfxSetTransform(Bottom_Boundary_transform.m);

	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
}
#pragma endregion