/* Start Header ************************************************************************/
/*!
\file		Main.h
\author		Tan Wei Liang Terril
\date		March, 20, 2025
\brief		Fileontain the function to set the main game loop
			- Initialise the game
			- Update the game
			- Draw the game
			- Exit the game
			- Handle the mouse input
			- Handle the keyboard input
			- Handle the collision detection
			- Handle the bullet update and draw
			- Handle the enemy spawn and draw
			- Handle the player spawn and draw
			- Handle the skill menu display and selection
			- Handle the weapon menu display and selection

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef main_H
#define main_H

#include <iostream>
#include <vector>
#include <sstream>
#include "AEEngine.h"
#include "collision.h"

constexpr int MAX_AMMO_SIZE = 1000;
constexpr int  MAX_PAST_SCORES = 5;  // Max number of past scores to display

typedef struct shape {
	float scale;
	float pos_x, pos_y;

	float Rotation;
	//unsigned int color;
	//AEMtx33 transform;
	AEVec2 vec_pos;

} shape;

typedef struct Mouse {
	s32 mouseX;
	s32 mouseY;
	AEVec2 vec_Mouse;
}Mouse;

enum class BULLET_FIRED_BY {
	PLAYER,
	ENEMY
	/*PISTOL,
	RIFLE,
	SHOTGUN*/
};

// Enum for different bullet patterns
enum BULLET_PATTERN {
	SINGLE,     // Single bullet (original)
	PARALLEL,   // Two bullets side by side
	SPREAD,     // Two bullets at an angle
	TRIPLE      // Three bullets (one forward, two angled)
};

typedef struct Bullet {
	// Damage: How much damage you want it to do,
	// Speed: How fast you want it to travel
	// lifespan: How long do you want the bullet to last before it 'dies'
	int damage, speed;
	float lifespan;
	shape bulletShape; 
	BULLET_FIRED_BY id;
	Collider collider;

	// bullet constructor to init bullet position
	Bullet(int dmg, int spd, float lp, float posX, float posY, BULLET_FIRED_BY objectID, float rotation): damage(dmg), speed(spd), lifespan(lp), id(objectID) {
		bulletShape.pos_x = posX;
		bulletShape.pos_y = posY;
		bulletShape.Rotation = rotation;
		bulletShape.scale = 25.0f;
		AEVec2 bulletPos = { posX, posY };
		collider = CreateCollider(bulletShape.scale/2.0f, &bulletPos, PROJECTILE); // Initialize collider
	}

}Bullet;

/// <summary>
/// Define a struct to store both weapon and skill,
/// which will be use to show what the player can choose from.
/// </summary>
struct Weapon_Skill_Item {
	bool isSkill; // Check if it is a skill, if it is a skill then true will be assign to it
	void* data; // pointer to store a weapon or a skill data dynamically
	std::string nameID; // Identifier if it is a weapon or a skill
};

/// <summary>
/// Define a struct to store the player score base on the amount of enemy killer, wave cleared and survival bonus.
/// </summary>
typedef struct ScoreBoard {
	int EnemyKilled;
	int WaveCleared;
	int survivalBonus;
	// First int: min, Second int:second
	std::pair<int, int>surviveDuration; // Store min and second in the respective 
}ScoreBoard;


#endif // end of main_H




