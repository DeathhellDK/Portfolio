/***********************************************************
 file:	utils.h
 author:	Terril					t.weiliang@digipen.edu
			Zhi Jie					zhijie.lim@digipen.edu
			Kai Yang				 k.kaiyang@digipen.edu
			Tingkai					ye.t@digipen.edu

 brief:	declaration of functions used in utils.c

 Copyright © 2024 DigiPen, All rights reserved.
***********************************************************/

#pragma once
#include "cprocessing.h"
#include "bullet.h"
#include "player.h"
#include "game.h"


//setting entity type to determine render order during run time
enum ENTITY_TYPE
{
	//used as the end point for render checks and the default type set for all new entities
	START,
	BULLET,
	PLAYER,
	ENEMY,
	UI,
	//used as a starting point for render checks
	END
};

// render type for entity
// this is used to identify 
enum RENDER_TYPE
{
	null = 0,
	circle,
	quad,
	triangle,
	image
};

extern entity entities[10];

int IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y);
//collision check 
BOOL AreQuadIntersecting(float quadPos1X, float quadPos1Y, float width1, float height1, float quadPos2X, float quadPos2Y, float width2, float height2);
int IsPlayerInArea(float hitbox_x, float hitbox_y, float areax, float areay, float areaw, float areah);


//initialise entity array to default values
void initEntityArray(entity entityList[], int size);
//add entities to the entity array
int addEntity(entity newEntity);
//remove entity
void removeEntity(int index);
//render all entities 
int renderEntities();

// For collision
// Terrain in this case for this function is represent the wall and the bullet
BOOL isTerrainRectangleCollision(Terrain rectA, Terrain rectB);

// Terrain in this case for this function represent the enemy and the bullet
BOOL isEnemyRectangleCollision2(entity r1, Terrain r2);

BOOL isPlayerRectangleCollision2(Terrain r1, Terrain r2);