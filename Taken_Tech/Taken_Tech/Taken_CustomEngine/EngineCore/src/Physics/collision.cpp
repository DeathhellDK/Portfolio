/**
* @file collision.cpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-09-24
*
* @brief Implementation of Collision Manager class functions
* 
* @version 1.1
* 
* @update version history
* @version 1.0 - Implement function for basic shape collision detection (rectangle, triangle, circle).
* @version 1.1 - Implement function for OBB and convex mesh collision detection using SAT.
* 
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Physics/collision.hpp"
#include <cmath>

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
bool Collision_Mgr::checkCollisionRectToRect(const Vector2& rect1, float rect1_w1, float rect1_h1, const Vector2& rect2, float rect2_w2, float rect2_h2)
{
    /// Rectangle collision check
    return rect1.x < rect2.x + rect2_w2 && rect1.x + rect1_w1 > rect2.x && rect1.y < rect2.y + rect2_h2 && rect1.y + rect1_h1 > rect2.y;
}

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
bool Collision_Mgr::checkCollisionTrigToTrig(const Vector2& tri1_p1, const Vector2& tri1_p2, const Vector2& tri1_p3, const Vector2& tri2_p1, const Vector2& tri2_p2, const Vector2& tri2_p3)
{
    /// Vertex check for the first triangle
    if (checkPointposInTrig(tri1_p1, tri2_p1, tri2_p2, tri2_p3)) return true;
    if (checkPointposInTrig(tri1_p2, tri2_p1, tri2_p2, tri2_p3)) return true;
    if (checkPointposInTrig(tri1_p3, tri2_p1, tri2_p2, tri2_p3)) return true;

    /// Vertex check for the second triangle
    if (checkPointposInTrig(tri2_p1, tri1_p1, tri1_p2, tri1_p3)) return true;
    if (checkPointposInTrig(tri2_p2, tri1_p1, tri1_p2, tri1_p3)) return true;
    if (checkPointposInTrig(tri2_p3, tri1_p1, tri1_p2, tri1_p3)) return true;

    return false;
}

/**
   * @brief Check collision between two circles
   *
   * @param c1 Center position of the first circle
   * @param c1_r Radius of the first circle
   * @param c2 Center position of the second circle
   * @param c2_r Radius of the second circle
   */
bool Collision_Mgr::checkColllisionCircleToCricle(const Vector2& c1, float c1_r, const Vector2& c2, float c2_r) {
    /// Circle collision check
    float checkDistance = checkDistanceCircleToRad(c1, c2);
    float totalRadius = c1_r + c2_r;

    return checkDistance < totalRadius * totalRadius;
}

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
bool Collision_Mgr::checkCollisionRectToTrig(const Vector2& rect1, float rect1_w1, float rect1_h1, 
    const Vector2& tri1_p1, const Vector2& tri1_p2, const Vector2& tri1_p3) {

    /// Vertex check for the triangle
    if (checkPointposInRect(tri1_p1, rect1, rect1_w1, rect1_h1)) return true;
    if (checkPointposInRect(tri1_p2, rect1, rect1_w1, rect1_h1)) return true;
    if (checkPointposInRect(tri1_p3, rect1, rect1_w1, rect1_h1)) return true;

    /// Vertex check for the rectangle
    if (checkPointposInTrig(rect1, tri1_p1, tri1_p2, tri1_p3)) return true;
    if (checkPointposInTrig(Vector2(rect1.x + rect1_w1, rect1.y), tri1_p1, tri1_p2, tri1_p3)) return true;
    if (checkPointposInTrig(Vector2(rect1.x, rect1.y + rect1_h1), tri1_p1, tri1_p2, tri1_p3)) return true;

    return false;
}

/**
   * @brief Check collision between rectangle and circle
   *
   * @param rect1 x and y position of the rectangle
   * @param rect1_w Width of the rectangle
   * @param rect1_h Height of the rectangle
   * @param c1 Center position of the circle
   * @param c1_r Radius of the circle
   */
bool Collision_Mgr::checkCollisionRectToCircle(const Vector2& rect1, float rect1_w, float rect1_h, const Vector2& c1, float c1_r) {

    float clampedX = std::fmax(rect1.x, std::fmin(c1.x, rect1.x + rect1_w));
    float clampedY = std::fmax(rect1.y, std::fmin(c1.y, rect1.y + rect1_h));

    Vector2 closestPoint(clampedX, clampedY);

    float distance = (c1 - closestPoint).LengthSqd();

    return distance < c1_r * c1_r;
}

/**
   * @brief Check collision between circle and triangle
   *
   * @param c1 Center position of the circle
   * @param c1_r Radius of the circle
   * @param tri1_p1 First vertex of the triangle
   * @param tri1_p2 Second vertex of the triangle
   * @param tri1_p3 Third vertex of the triangle
   */
bool Collision_Mgr::checkCollisionCircleToTrig(const Vector2& c1, float c1_r, const Vector2& tri1_p1, const Vector2& tri1_p2, const Vector2& tri1_p3) {

    /// Check if any of the triangle vertex is in the circle
    if (checkPointposInTrig(c1, tri1_p1, tri1_p2, tri1_p3)) return true;

    /// Check the distance from the circle center to the triangle edges
    if (checkDistanceTolineSegment(c1, tri1_p1, tri1_p2) < c1_r) return true;
    if (checkDistanceTolineSegment(c1, tri1_p2, tri1_p3) < c1_r) return true;
    if (checkDistanceTolineSegment(c1, tri1_p3, tri1_p1) < c1_r) return true;

    return false;

}

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
bool Collision_Mgr::checkCollisionOBBToOBB(const Vector2& center1, const Vector2& halfsize1, float rotation1, 
    const Vector2& center2, const Vector2& halfsize2, float rotation2)
{
	/// Function to get the corners of the OBB
    auto getCorners = [](const Vector2& center, const Vector2& halfsize, float rotation) {
		float cos = std::cos(rotation);
		float sin = std::sin(rotation); /// Compute sine

		/// Rotation matrix components
		Vector2 y(cos, sin);
        Vector2 x(-sin, cos);

        std::vector<Vector2> corners(4);

        /// Top Right
        corners[0] = center + x * halfsize.x + y * halfsize.y;

        /// Top Left
        corners[1] = center - x * halfsize.x + y * halfsize.y;

        /// Bottom Left
        corners[2] = center - x * halfsize.x - y * halfsize.y;

        /// Bottom Right
        corners[3] = center + x * halfsize.x - y * halfsize.y;

        return corners;
    };

	/// Get corners of both OBBs
	auto corners1 = getCorners(center1, halfsize1, rotation1);
    auto corners2 = getCorners(center2, halfsize2, rotation2);

	/// axes to be tested
    std::vector<Vector2> axes = {
        (corners1[1] - corners1[0]).Normalized(),
        (corners1[3] - corners1[0]).Normalized(),
        (corners2[1] - corners2[0]).Normalized(),
        (corners2[3] - corners2[0]).Normalized()
    };

	/// Check for overlap on each axis
    for (const auto& axis : axes) {
        if (!overlapOnAxis(axis, corners1, corners2)) {
            return false;
        }
    }
    return true;

}

/**
* @brief Check collision between two convex meshes using Separating Axis Theorem (SAT)
* 
* @param mesh1 Vertices of the first convex mesh
* @param mesh2 Vertices of the second convex mesh
*/
bool Collision_Mgr::checkCollisionMeshToMesh(const std::vector<Vector2>& mesh1, const std::vector<Vector2>& mesh2)
{
	/// Helper lambda to get the edges and their normals 
    auto getEdges = [](const std::vector<Vector2>& mesh) {
        std::vector<Vector2> axis;

		/// Loop through each edge of the mesh
        for (size_t i = 0; i < mesh.size(); ++i) {

            /// Current vertex
			Vector2 p1 = mesh[i];
			
            /// Next vertex (wrap around)
            Vector2 p2 = mesh[(i + 1) % mesh.size()];

            /// Edge vector
			Vector2 edge = p2 - p1;

            /// Perpendicular normal vector
			Vector2 normal = Vector2(-edge.y, edge.x);

            /// Store normalized axis
			axis.push_back(normal.Normalized());
        }
        return axis;
    };

    std::vector<Vector2> axes;

	/// Get edges (normals) from both meshes
    auto edges1 = getEdges(mesh1);
    auto edges2 = getEdges(mesh2);

	/// Combine axes from both meshes
    axes.insert(axes.end(), edges1.begin(), edges1.end());
    axes.insert(axes.end(), edges2.begin(), edges2.end());

	/// Check for overlap on each axis
    for (const auto& axis : axes) {
        if (!overlapOnAxis(axis, mesh1, mesh2)) {
            return false;
        }
    }

    return true;
}



////------------------------- Helper Functions -------------------------////

/**
   * @brief Check if a point in rectangle
   *
   * @param point The point to be checked
   * @param rect1 The bottom-left corner of the rectangle
   * @param rect1_w1 Width of the rectangle
   * @param rect1_h1 Height of the rectangle
   */
bool Collision_Mgr::checkPointposInRect(const Vector2& point, const Vector2& rect1, float rect1_w1, float rect1_h1) {

    return point.x > rect1.x && point.x < rect1.x + rect1_h1 && point.y > rect1.y && point.y < rect1.y + rect1_w1;
}

/**
   * @brief Check if a point in triangle
   *
   * @param point The point to be checked
   * @param tri1_p1 First vertex of the triangle
   * @param tri1_p2 Second vertex of the triangle
   * @param tri_p3 Third vertex of the triangle
   */
bool Collision_Mgr::checkPointposInTrig(const Vector2& point, const Vector2& tri1_p1, const Vector2& tri1_p2, const Vector2& tri1_p3) {

    /// Cross product method to check if the point is in the triangle
    float cp_result1 = (point.x - tri1_p2.x) * (tri1_p1.y - tri1_p2.y) - (point.y - tri1_p2.y) * (tri1_p1.x - tri1_p2.x);
    float cp_result2 = (point.x - tri1_p3.x) * (tri1_p2.y - tri1_p3.y) - (point.y - tri1_p3.y) * (tri1_p2.x - tri1_p3.x);
    float cp_result3 = (point.x - tri1_p1.x) * (tri1_p3.y - tri1_p1.y) - (point.y - tri1_p1.y) * (tri1_p3.x - tri1_p1.x);

    /// Check if the point is on different side of the triangle edges
    bool checkNegative = (cp_result1 < 0) || (cp_result2 < 0) || (cp_result3 < 0);
    bool checkPositive = (cp_result1 > 0) || (cp_result2 > 0) || (cp_result3 > 0);

    return checkNegative && checkPositive;
}

/**
    * @brief Check the shortest distance from circle center to the edge of another circle
    *
    * @param c1 Center position of the first circle
    * @param rad Radius of the first circle
    */
float Collision_Mgr::checkDistanceCircleToRad(const Vector2& c1, const Vector2& rad) {
    float dx = c1.x - rad.x;
    float dy = c1.y - rad.y;

    return dx * dx + dy * dy;
}

/**
* @brief Check the shortest distance from a point to a line segment
*
* @param point The point to be checked
* @param p1 The starting point of the line segment
*/
float Collision_Mgr::checkDistanceTolineSegment(const Vector2& point, const Vector2& p1, const Vector2& p2)
{
    Vector2 segment = p2 - p1;
    Vector2 distance = point - p1;

    float disResult = segment.LengthSqd();

    /// Avoid division by zero
    if (disResult == 0.0f) return distance.Length();

    float pt_Line = distance.DotProduct(segment) / disResult;

    Vector2 projection;

    /// Clamp the projection point to be within the line segment
    if (pt_Line < 0.0f) projection = p1;
    else if (pt_Line > 1.0f) projection = segment;
    else  projection = p1 + segment * pt_Line;

    /// Return the distance from the point to the projection point
    return (point - projection).Length();
}

/**
* @brief Determines if two external surface of the shapes overlap along a axis
* 
* @param axis Axis to project the shape onto
* @param verts1 Vertices of the first shape
* @param verts2 Vertices of the second shape
* 
*/
bool Collision_Mgr::overlapOnAxis(const Vector2& axis, const std::vector<Vector2>& verts1, const std::vector<Vector2>& verts2)
{
    /// Helper lambda to project verices onto an axis
    auto projectOntoAxis = [&](const std::vector<Vector2>& verts, float& min, float& max) {
        ///Init min and max to the projection of the first vertex
        min = max = verts[0].DotProduct(axis);
        for (const auto& vert : verts) {
            float projection = vert.DotProduct(axis);
            min = std::min(min, projection);
            max = std::max(max, projection);
        }
    };

    float min1, max1, min2, max2;

    /// Project both sets of vertices onto the axis
    projectOntoAxis(verts1, min1, max1);
    projectOntoAxis(verts2, min2, max2);

    return !(max1 < min2 || max2 < min1);
}

