/* Start Header ************************************************************************/
/*!
\file		enemy.cpp
\author		Zhi Jie 
\date		March, 20, 2025
\brief		The enemy system consists of several functions that handle enemy spawning, 
movement, and removal in the game. The addenemy function adds a new enemy to the wave vector, 
ensuring that it spawns around the player but not within a 200-radius protection zone. 
Type 2 enemies follow a circular spawning pattern around the player. The enemyspawn 
function is responsible for rendering enemies by generating a circular mesh and iterating 
through the wave vector to draw them at their respective positions. Additionally, it includes 
optional debugging features for spawn protection. When an enemy's HP reaches zero, the removeenemy 
function removes it from the wave vector. The enemyai function governs enemy behavior, making 
type 0 enemies move toward the player while avoiding clustering, and type 1 (boss) enemies move 
toward the player but begin circling at a set distance. The get_current_enemy_count function 
returns the number of enemies still alive, while all_enemies_killed checks whether all enemies 
have been defeated.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
//#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "Main.h"
#include "enemy.h"
#include <iostream>
#include <cstdlib>
#include <vector>

std::vector <enemy> wave;

// add enemy
void addenemy(float radius, int hp, int exp, float movementspd, int type, float player_pos_x, float player_pos_y) {
	float enemy_x = 0.0f, enemy_y = 0.0f;

	srand((unsigned int)time(0));
	// 0 for positive values, 1 for negative values

	if (type != 2) {
		int randomx = rand() % 2;
		//int randomy = rand() % 2;

		// enemy will always spawn around the player but never within a 200 radius around the player
		// range to be expanded later on
		int randomquarter = rand() % 4;

		if (randomquarter == 0) {
			enemy_y = float(rand() % 800 + (200 + player_pos_y)); // + player pos y
			if (randomx == 0) {
				enemy_x = float(rand() % 1200 + (0 + player_pos_x)); // + player pos x
			}
			else {
				enemy_x = -float(rand() % 1200 + (0 - player_pos_x)); // - player pos x
			}
		}
		if (randomquarter == 1) {
			enemy_y = -float(rand() % 800 + (200 - player_pos_y)); // - player pos y
			if (randomx == 0) {
				enemy_x = float(rand() % 1200 + (0 + player_pos_x)); // + player pos x
			}
			else {
				enemy_x = -float(rand() % 1200 + (0 - player_pos_x)); // - player pos x
			}
		}
		if (randomquarter == 2) {
			enemy_y = float(rand() % 800 + (0 + player_pos_y)); // + player pos y
			if (randomx == 0) {
				enemy_x = float(rand() % 1000 + (200 + player_pos_x)); // + player pos x
			}
			else {
				enemy_x = -float(rand() % 1000 + (200 - player_pos_x)); // - player pos x
			}
		}
		if (randomquarter == 3) {
			enemy_y = -float(rand() % 800 + (0 - player_pos_y)); // - player pos y
			if (randomx == 0) {
				enemy_x = float(rand() % 1000 + (200 + player_pos_x)); // + player pos x
			}
			else {
				enemy_x = -float(rand() % 1000 + (200 - player_pos_x)); // - player pos x
			}
		}

		enemy newenemy{ radius, enemy_x, enemy_y, hp, exp, movementspd, type };
		wave.push_back(newenemy);
	}
	else if (type == 2) {
		static float angle = 0.0f;
		float distance = 300.0f;


			enemy_x = player_pos_x + distance * cos(angle);
			enemy_y = player_pos_y + distance * sin(angle);

			angle += PI / 4;

			enemy newenemy{ radius, enemy_x, enemy_y, hp, exp, movementspd, type };
			wave.push_back(newenemy);

	}
	
}

// spawning of enemies
void enemyspawn() {

#pragma region CircleMesh

	// random enemy
	int numofTri_segments = 36;
	unsigned int defaultcolor = 0xFFFFFFFF;
	float center_posX = 0.0f;
	float center_posY = 0.0f;
	float radius = 0.5f;
	// Create a circle using the segments
	for (int i = 0; i < numofTri_segments; ++i)
	{
		// Calculate the angle for the current and next segment
		float angle1 = (2.0f * 3.14f * i) / numofTri_segments;
		float angle2 = (2.0f * 3.14f * (i + 1)) / numofTri_segments;

		// vertices for that particulare triangle
		float x1 = center_posX + radius * cosf(angle1);
		float y1 = center_posY + radius * sinf(angle1);
		float x2 = center_posX + radius * cosf(angle2);
		float y2 = center_posY + radius * sinf(angle2);

		AEGfxTriAdd(
			center_posX, center_posY, defaultcolor, 0.5f, 0.5f,  // Center vertex
			x1, y1, defaultcolor, 0.5f + x1, 0.5f + y1, // First outer vertex
			x2, y2, defaultcolor, 0.5f + x2, 0.5f + y2  // Second outer vertex
		);
	}

	AEGfxVertexList* pcMesh = 0;
	pcMesh = AEGfxMeshEnd();

#pragma endregion

	// used for debugging (make function take in playerposx and playerposy to use)
#pragma region SpawnProtection
//
//	// radius around player to see if enemy intersects spawn protection, enemies should and will not spawn inside the circle
//	AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
//	AEGfxSetColorToAdd(0.953f, 0.651f, 0.937f, 0.0f);
//
//	AEMtx33 transform;
//	AEMtx33Identity(&transform);
//
//	AEMtx33 spin;
//	AEMtx33Rot(&spin, PI / 2);
//
//	AEMtx33 scale;
//	AEMtx33Scale(&scale, 300.0f, 300.0f);
//
//	AEMtx33 translate;
//	AEMtx33Trans(&translate, 0.0, 0.0); // change 0 to player pos x and player pos y
//
//	AEMtx33Concat(&transform, &spin, &scale);
//	AEMtx33Concat(&transform, &translate, &transform);
//
//	AEGfxSetTransform(transform.m);
//
//	AEGfxMeshDraw(pcMesh, AE_GFX_MDM_TRIANGLES);
//
#pragma endregion

	// for loop to generate enemies at random location around player pos
	for (int i = 0; i < wave.size(); i++) {
		AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
		AEGfxSetColorToAdd(0.0f, 1.0f, 0.0f, 0.0f);

		AEMtx33 transform;
		AEMtx33Identity(&transform);

		AEMtx33 spin;
		AEMtx33Rot(&spin, PI / 2);

		AEMtx33 scale;
		AEMtx33Scale(&scale, wave[i].radius, wave[i].radius);

		AEMtx33 translate;
		AEMtx33Trans(&translate, wave[i].posx, wave[i].posy);

		AEMtx33Concat(&transform, &spin, &scale);
		AEMtx33Concat(&transform, &translate, &transform);

		AEGfxSetTransform(transform.m);

		AEGfxMeshDraw(pcMesh, AE_GFX_MDM_TRIANGLES);

		// printf("enemy spawned! %i\n", i);
	}
	
	AEGfxMeshFree(pcMesh);
}

// remove the enemy when its hp is 0

void removeenemy(int index) {
	wave.erase(wave.begin() + index);
}

void enemyai(float player_pos_x, float player_pos_y) {
	// Minimum distance between enemies
	// const float min_dist_enemy = 50.0f; 
	const float min_dist_boss = 400.0f;

	for (int i = int(wave.size() - 1); i >= 0; i--) {
		float distance_x = player_pos_x - wave[i].posx;
		float distance_y = player_pos_y - wave[i].posy;
		float distance = sqrt(distance_x * distance_x + distance_y * distance_y);
		float angle = atan2(-distance_y, -distance_x);

		// basic enemy type ai, move towards player
		if (wave[i].type == 0) {
			
			float move_x = 0, move_y = 0;
			if (distance > 0) {
				move_x = (distance_x / distance) * wave[i].movementspd;
				move_y = (distance_y / distance) * wave[i].movementspd;
			}

			// Prevent cluster between enemies
			float enemy_dist_x = 0, enemy_dist_y = 0;
			for (int j = 0; j < wave.size(); j++) {
				if (i == j) continue;

				float diff_x = wave[i].posx - wave[j].posx;
				float diff_y = wave[i].posy - wave[j].posy;
				float enemy_distance = sqrt(diff_x * diff_x + diff_y * diff_y);

				if (enemy_distance < wave[i].radius && enemy_distance > 0) {
					enemy_dist_x += diff_x / enemy_distance;
					enemy_dist_y += diff_y / enemy_distance;
				}
			}

			float separation = sqrt(enemy_dist_x * enemy_dist_x + enemy_dist_y * enemy_dist_y);
			if (separation > 0) {
				enemy_dist_x = (enemy_dist_x / separation) * wave[i].movementspd;
				enemy_dist_y = (enemy_dist_y / separation) * wave[i].movementspd;
			}

			wave[i].posx += move_x + enemy_dist_x;
			wave[i].posy += move_y + enemy_dist_y;
		}

		// boss type ai
		else if (wave[i].type == 1) {
			if (distance > min_dist_boss) {
				// Move towards the player if the boss is farther than 100 units
				float move_x = (distance_x / distance) * wave[i].movementspd;
				float move_y = (distance_y / distance) * wave[i].movementspd;

				wave[i].posx += move_x;
				wave[i].posy += move_y;
			}
			else {
				// Once within 100 units, start circling around the player
				// Calculate the angle to the player
				float circle_speed = 0.01f * wave[i].movementspd;
				float newangle = angle;
				// 
				// << angle << std::endl;

				//float angle = 0;// atan2(distance_y, distance_x); // Calculate current angle to player
				newangle += circle_speed; // Increment the angle for rotation

				// Calculate the boss's new position in a circular path
				wave[i].posx = player_pos_x + min_dist_boss * cos(newangle);
				wave[i].posy = player_pos_y + min_dist_boss * sin(newangle);
			}
		}
	}
}

// returns enemy count of current wave
int get_current_enemy_count(const std::vector<enemy>& waves) {
	int aliveCount = 0;

	for (const enemy& e : waves) {  // Iterate over each enemy in the wave
		if (e.hp > 0) { 
			aliveCount++;
		}
	}
	// std::cout << aliveCount << "\n";
	return aliveCount;
}

// function to cehck if all enemies are dead
bool all_enemies_killed(const std::vector<enemy>& waves) {
	return get_current_enemy_count(waves) == 0;
}