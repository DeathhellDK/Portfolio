/**
* @file BroadCollision.hpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-10-22
*
* @brief Header file for BroadCollision Classs used in 
* broad-phase collision detection.
* Class implements a spatital partitioning system 
* for broad-phrase collision detection base on a uniform grid
*
* @version 1.0
* - Implementation of a broad-phase grid collision system.
* 
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#ifndef BROAD_COLLISION_HPP /// Header guard
#define BROAD_COLLISION_HPP

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>
#include "Math/vect2.h"
#include "Core/component.h"

/**
* @class BroadCollision
* @brief Manages broad-phase collision detection using a spatial hash grid.
*/
class BroadCollision {
public:

	/**
	* @brief Constructs a BroadCollision System
	* @param size The size of each grid cell
	* 
	* @note Avoid using large size cells as it may increase false detection
	* in collision detection.
	*/
	 BroadCollision(float size = 100.0f); /// default grid size

	 /**
	 * @brief Clears all entities and stored pairs from the the grid.
	 */
	 void clear();
	 
	 /**
	 * @brief Inserts an entity into the grid based on its position and size
	 * 
	 * @param id The unique entity identifier
	 * @param position The 2D position of the entity
	 * @param size The width and height of the entity
	 */
	 void insert(Entity id, const Vector2& position, const Vector2& size);


	 /**
	 * @brief Computes potential collision pairs using the populated grid.
	 */
	 std::vector<std::pair<Entity, Entity>> computePotentialCollisions();

private:

	/**
	 * @brief size of each spatial grid cell
	 */
	float cellSize;

	/**
	* @brief Hash grid mapping a unique cell key to enities withing the current cell.
	*/
	std::unordered_map<long long, std::vector<Entity>> grid;

	/**
	* @brief Keeps track of the entity pairs checked to prevent duplicates.
	*/
	std::unordered_set<unsigned long long> seenPairs;


	/**
	* @breif Computes a unqiue hash value for a grid cell coordinates
	* 
	* @param x - The x coordinate of the cell
	* @param y - The y coordinate of the cell
	*/
	long long cellHash(int x, int y) const;

};

#endif // !BROAD_COLLISION_HPP
