/* Start Header ************************************************************************/
/*!
\file		enemy.h
\author		Zhi Jie
\date		March, 20, 2025
\brief		This file contains the declaration of functions used in enemy.cpp

Structures:
- enemy: Represents an individual enemy's attributes such as position, health, and speed.

Functions:
- void enemyspawn();
- void addenemy(float radius, int hp, int exp, float movementspd, int type, float player_pos_x, float player_pos_y);
- void removeenemy(int index);
- void enemyai(float player_pos_x, float player_pos_y);
- int get_current_enemy_count(const std::vector<enemy>& waves);
- bool all_enemies_killed(const std::vector<enemy>& waves);

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include <vector>
#include "Main.h"

// struct for enemy
struct enemy {
	float radius;
	float posx, posy;
	int hp, exp;
	float movementspd;
	int type;
};

void enemyspawn();

void addenemy(float radius, int hp, int exp, float movementspd, int type, float player_pos_x, float player_pos_y);

void removeenemy(int index);

void enemyai(float player_pos_x, float player_pos_y);

int get_current_enemy_count(const std::vector<enemy>& waves);

bool all_enemies_killed(const std::vector<enemy>& waves);

extern std::vector <enemy> wave;