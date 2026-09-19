/**
* @file BroadCollision.cpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-10-22
*
* @brief Implementation of the BroadCollision class for
* broad-phase collision detection
*
* @version 1.0
* - Implementation of a BroadCollision class function.
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Physics/BroadCollision.hpp"

/**
* @brief Constructs a BroadCollision object with the given cell size
* @param Size of each grid cell used for spatial partitioning
*/
BroadCollision::BroadCollision(float size)
	: cellSize(size) {
}

/**
* @brief Clears all stored grid data and seen collision pairs
*/
void BroadCollision::clear() {
	grid.clear();
	seenPairs.clear();
}

/**
* @brief Generates a unique hash key for a grid cell coordinate
* 
* @param x - The x-coordinate of the grid cell
* @param y - The y-coordinate of the grid cell
*/
long long BroadCollision::cellHash(int x, int y) const {
	/// Large prime number is used to randomize the x-coordinates
	const long long prime1 = 73856093;

	/// Large prime number is used to randomize the y-coordinates
	const long long prime2 = 19349663;

	/// Return the value which is multiplies each coordinate by a unique large prime number
	return (static_cast<long long>(x) * prime1) ^ (static_cast<long long>(y) * prime2);
}

/**
* @brief Inserts and entity into a grid cells it occupies
* 
* @param id - The enity unique id
* @param postion - The 2D position of the entity
* @param size - The 2D size of the entity
*/
void BroadCollision::insert(Entity id, const Vector2& position, const Vector2& size) {
	/// Leftmost grid cell 
	int minX = static_cast<int>(std::floor(position.x / cellSize));
	
	///Topmost grid cell
	int minY = static_cast<int>(std::floor(position.y / cellSize));

	///Rightmost grid cell
	int maxX = static_cast<int>(std::floor((position.x + size.x) / cellSize));
	
	///Bottomost grid cell
	int maxY = static_cast<int>(std::floor((position.y + size.y) / cellSize));
	
	/// Loop through every grid cell and insert the entity ID
	/// into each cell entity list
	for (int x = minX; x <= maxX; ++x) {
		for (int y = minY; y <= maxY; ++y) {
			long long hash = cellHash(x, y);
			/// Store the entity id in the corresponding grid cell
			grid[hash].push_back(id);
		}
	}
}

/**
* @brief Computes all potential collision pairs from the spatial grid
*/
std::vector<std::pair<Entity, Entity>> BroadCollision::computePotentialCollisions() {
	
	/// Store all the potential collision pairs
	std::vector<std::pair<Entity, Entity>> potentialCollisions;
	
	/// Reset the set of checked pairs
	seenPairs.clear();

	/// Check all the entity in the grid and add 
	/// pair that hasn't been handled before
	for (auto& [key, list] : grid) {
		for(size_t i = 0; i < list.size(); ++i) {
			for(size_t j = i + 1; j < list.size(); ++j) {
				unsigned long long a = static_cast<unsigned long long>(list[i]);
				unsigned long long b = static_cast<unsigned long long>(list[j]);
				
				/// Combie the two ID into a single 64-bit key
				unsigned long long pairKey = static_cast<unsigned long long>(std::min(a, b)) << 32 | std::max(a, b);

				/// Check if the current pair have been handled before
				if(seenPairs.find(pairKey) == seenPairs.end()) {
					/// add it into the contianer and deem 
					/// that the current pair have been handled
					seenPairs.insert(pairKey);
					
					/// Record the potential pair
					potentialCollisions.emplace_back(list[i], list[j]);
				}
			}
		}
	}
	/// return the potential collision pairs
	return potentialCollisions;
}