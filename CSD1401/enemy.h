/***********************************************************
 file:	enemy.h
 author:	Kai Yang				 k.kaiyang@digipen.edu

 brief:	declaration of functions in enemy.c

 Copyright © 2024 DigiPen, All rights reserved.
***********************************************************/

#include "cprocessing.h"
#include "utils.h"
#include "bullet.h"

typedef struct {
	float nextWaypointX;
	float nextWaypointY;

	int enemyIndex;

	int enemyCount;
}enemy;

enemy enemyList[10];


BOOL lineOfSight(double originX, double originY, double destinationX, double destinationY, double gridw, double gridh, float posX[], float posY[], float width[], float height[],int size);
CP_Vector enemyMove(double posX, double posY, double nextPosX, double nextPosY,double speed);
CP_Vector nextWaypoint(double posX, double posY, double gridSizeX, double gridSizeY, float gridPosX[], float gridPosY[], float width[], float height[], int size);

void updateEnemy(float gridw, float gridh, float posX[], float posY[], float width[], float height[], int terrainSize, float playerPosX, float playerPosY, int array[], int arraySize);
void addEnemy(enemy enemy);
void removeEnemy(int index);
void enemyInit();

void renderEnemyHitBox();