/**
* @file collision.hpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-09-24
*
* @brief Header file for Collision Manager class
* 
* @version 1.1
* 
* @update version history
* @version 1.0 - Created a class of basic shape collision detection (rectangle, triangle, circle).
* @version 1.1 - Added OBB and convex mesh collision detection using SAT.
* 
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#ifndef COLLISION_HPP
#define COLLISION_HPP

#include "Math/vect2.h"
#include <vector>

/**
* @class Collision_Mgr
* @brief Manage shape collision detection.
*/
class Collision_Mgr {
public:

    /**
    * @brief Check collision between two rectangles
    *
    * @param rect1 x and y position of the first rectangle
    * @param rect1_w1 Width of the first rectangle
    * @param rect1_h1 Height of the first rectangle
    *
    * @param rect2 x and y position of the second rectangle
    * @param rect2_w2 Width of the second rectangle
    * @param rect2_h2 Height of the second rectangle
    */
    static bool checkCollisionRectToRect(const Vector2& rect1, float rect1_w1, float rect1_h1,
        const Vector2& rect2, float rect2_w2, float rect2_h2);

    /**
    * @brief Check collision between two triangles
    *
    * @param tri1_p1 First vertex of the first triangle
    * @param tri1_p2 Second vertex of the first triangle
    * @param tri1_p3 Third vertex of the first triangle
    *
    * @param tri2_p1 First vertex of the second triangle
    * @param tri2_p2 Second vertex of the second triangle
    * @param tri2_p3 Third vertex of the second triangle
    */
    static bool checkCollisionTrigToTrig(const Vector2& tri1_p1, const Vector2& tri1_p2, const Vector2& tri_p3,
        const Vector2& tri2_p1, const Vector2& tri2_p2, const Vector2& tri3_p3);

    /**
    * @brief Check collision between two circles
    *
    * @param c1 Center position of the first circle
    * @param c1_r Radius of the first circle
    * @param c2 Center position of the second circle
    * @param c2_r Radius of the second circle
    */
    static bool checkColllisionCircleToCricle(const Vector2& c1, float c1_r, const Vector2& c2, float c2_r);

    /// Different Shape collision check

    /**
    * @brief Check collision between rectangle and triangle
    *
    * @param rect1 x and y position of the rectangle
    * @param rect1_w1 Width of the rectangle
    * @param rect1_h1 Height of the rectangle
    * @param tri1_p1 First vertex of the triangle
    * @param tri1_p2 Second vertex of the triangle
    * @param tri1_p3 Third vertex of the triangle
    */
    static bool checkCollisionRectToTrig(const Vector2& rect1, float rect1_w1, float rect1_h1,
        const Vector2& tri1_p1, const Vector2& tri1_p2, const Vector2& tri1_p3);


    /**
    * @brief Check collision between circle and triangle
    *
    * @param c1 Center position of the circle
    * @param c1_r Radius of the circle
    * @param tri1_p1 First vertex of the triangle
    * @param tri1_p2 Second vertex of the triangle
    * @param tri1_p3 Third vertex of the triangle
    */
    static bool checkCollisionCircleToTrig(const Vector2& c1, float c1_r,
        const Vector2& tri1_p1, const Vector2& tri1_p2, const Vector2& tri1_p3);

    /**
    * @brief Check collision between rectangle and circle
    *
    * @param rect1 x and y position of the rectangle
    * @param rect1_w Width of the rectangle
    * @param rect1_h Height of the rectangle
    * @param c1 Center position of the circle
    * @param c1_r Radius of the circle
    */
    static bool checkCollisionRectToCircle(const Vector2& rect1, float rect1_w, float rect1_h,
        const Vector2& c1, float c1_r);


    
	//--- Verison 1.1 update ---//

    /**
    * @brief Oriented Bounding Box (OBB) collision checks using Separating Axis Theorem (SAT)
	* @param center1 Center position of the first OBB
	* @param halfsize1 Half-size (extents) of the first OBB
	* @param rotation1 Rotation angle (in radians) of the first OBB
    * 
	* @param center2 Center position of the second OBB
	* @param halfsize2 Half-size (extents) of the second OBB
	* @param rotation2 Rotation angle (in radians) of the second OBB
    */
    static bool checkCollisionOBBToOBB(const Vector2& center1, const Vector2& halfsize1, float rotation1,
        const Vector2& center2, const Vector2& halfsize2, float rotation2);

    /**
	* @brief Check collision between two convex meshes using Separating Axis Theorem (SAT)
	* @param mesh1 Vertices of the first convex mesh
	* @param mesh2 Vertices of the second convex mesh
    */
    static bool checkCollisionMeshToMesh(const std::vector<Vector2>& mesh1, const std::vector<Vector2>& mesh2);

private:
    /**
    * @brief Check if a point in rectangle
    *
    * @param point The point to be checked
    * @param rect1 The bottom-left corner of the rectangle
    * @param rect1_w1 Width of the rectangle
    * @param rect1_h1 Height of the rectangle
    */
    static bool checkPointposInRect(const Vector2& point, const Vector2& rect1, float rect1_w1, float rect1_h1);

    /**
    * @brief Check if a point in triangle
    *
    * @param point The point to be checked
    * @param tri1_p1 First vertex of the triangle
    * @param tri1_p2 Second vertex of the triangle
    * @param tri_p3 Third vertex of the triangle
    */
    static bool checkPointposInTrig(const Vector2& point, const Vector2& tri1_p1, const Vector2& tri1_p2, const Vector2& tri_p3);

    /**
    * @brief Check the shortest distance from circle center to the edge of another circle
    *
    * @param c1 Center position of the first circle
    * @param rad Radius of the first circle
    */
    static float checkDistanceCircleToRad(const Vector2& c1, const Vector2& rad);

    /**
    * @brief Check the shortest distance from a point to a line segment
    *
    * @param point The point to be checked
    * @param p1 The starting point of the line segment
    */
    static float checkDistanceTolineSegment(const Vector2& point, const Vector2& p1, const Vector2& p2);


	//--- Verison 1.1 update ---//
    /**
	* @brief Project vertices onto an axis and check for overlap
    * 
	* @param axis The axis to project onto
	* @param verts1 Vertices of the first shape
	* @param verts2 Vertices of the second shape
    */
    static bool overlapOnAxis(const Vector2& axis, const std::vector<Vector2>& verts1, const std::vector<Vector2>& verts2);
};


#endif
