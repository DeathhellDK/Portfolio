/***********************************************************
 file:	enemy.c
 author:	Kai Yang				 k.kaiyang@digipen.edu

 brief:	functions that pertain to the enemy behaviour 

 Copyright © 2024 DigiPen, All rights reserved.
***********************************************************/

#include "enemy.h"
#include "cprocessing.h"
#include "utils.h"
#include "math.h"
#include "bullet.h"
#include <stdio.h>

BOOL lineOfSight(double originX, double originY, double destinationX, double destinationY, double gridw, double gridh,
	float posX[], float posY[], float width[], float height[], int size)
{
	BOOL returnValue = TRUE;

	CP_Vector origin, destination;
	origin = CP_Vector_Set((float)originX, (float)originY);
	destination = CP_Vector_Set((float)destinationX, (float)destinationY);

	if (CP_Vector_Distance(origin, destination) > 300) return FALSE;

	double dx = destinationX - originX;
	double dy = destinationY - originY;

	int steps = (int)abs((int)dx) > (int)abs((int)dy) ? (int)abs((int)dx) / (int)(gridh * 0.05f) : (int)abs((int)dy) / (int)(gridh * 0.05f);

	double xInc = dx / steps;
	double yInc = dy / steps;

	double xPos = originX, yPos = originY;
	//check if intersect with wall grid
	for (int i = 0;i <= steps;++i) {
		//testing visuals
		for (int i = 0;i < size;++i) {
			if (CP_System_GetWindowHeight() * 0.05f + posX[i] * gridw <= xPos &&
				xPos <= CP_System_GetWindowHeight() * 0.05f + posX[i] * gridw + width[i] * gridw &&
				CP_System_GetWindowHeight() * 0.2f + posY[i] * gridh <= yPos &&
				yPos <= CP_System_GetWindowHeight() * 0.2f + posY[i] * gridh + height[i] * gridh) {
				returnValue = FALSE;
			}
		}

		xPos += xInc;
		yPos += yInc;
	}
	return returnValue;
}

CP_Vector enemyMove(double posX, double posY, double nextPosX, double nextPosY, double speed)
{
	CP_Vector newPos = CP_Vector_Set((float)posX, (float)posY);
	if (fabsf((float)posX - (float)nextPosX) < 1.f && fabsf((float)posY - (float)nextPosY) < 1.f)
		return CP_Vector_Set((float)nextPosX, (float)nextPosY);
	newPos = CP_Vector_Add(newPos, CP_Vector_Scale(CP_Vector_Normalize(CP_Vector_Set((float)nextPosX - (float)posX, (float)nextPosY - (float)posY)), CP_System_GetDt() * (float)speed));
	return newPos;
}

CP_Vector nextWaypoint(double posX, double posY, double gridSizeX, double gridSizeY, 
	float gridPosX[], float gridPosY[], float width[], float height[], int size)
{
	//to do update checks to see if direction is valid
	CP_Vector returnVec = CP_Vector_Set(0, 0);
	
	float boarderLeft = CP_System_GetWindowHeight() * 0.05f;
	float boarderRight = CP_System_GetWindowHeight() * 0.05f + (float) gridSizeX;
	float boarderTop = CP_System_GetWindowHeight() * 0.2f;
	float boarderBot = CP_System_GetWindowHeight() * 0.2f + (float) gridSizeY;

	BOOL validMove = FALSE;
	while (!validMove) {
		//get a random direction and provide a next position
		switch (CP_Random_RangeInt(1, 4))
		{
		case 1:
			//left
			returnVec = CP_Vector_Set((float)posX - (float)(gridSizeX * 0.1f), (float)posY);
			break;
		case 2:
			//right
			returnVec = CP_Vector_Set((float)posX + (float)(gridSizeX * 0.1f), (float)posY);
			break;
		case 3:
			//down
			returnVec = CP_Vector_Set((float)posX, (float)posY + (float)(gridSizeY * 0.1f));
			break;
		case 4:
			//up
			returnVec = CP_Vector_Set((float)posX, (float)posY - (float)(gridSizeY * 0.1f));
			break;
		default:
			returnVec = CP_Vector_Set((float)posX, (float)posY);
			break;
		}
		//checks if the move is a valid move
		//if within the bounds of the screen
		
		if (returnVec.x < boarderLeft || boarderRight < returnVec.x || returnVec.y < boarderTop || returnVec.y > boarderBot) continue;
		for (int i = 0;i < size;++i) {
			float xleft = CP_System_GetWindowHeight() * 0.05f + gridPosX[i] * (float) gridSizeX;
			float xRight = xleft + (width[i] * (float)gridSizeX);

			float yTop = CP_System_GetWindowHeight() * 0.2f + gridPosY[i] * (float)gridSizeY;
			float yBot = yTop + height[i] * (float)gridSizeY;

			if (xleft < returnVec.x && returnVec.x < xRight &&
				yTop < returnVec.y &&  returnVec.y < yBot ) {
				validMove = FALSE;
				break;
			}
			else validMove = TRUE;
		}
	}
	return returnVec;
}


void updateEnemy(float gridw,float gridh, float posX[], float posY[], float width[], float height[],int terrainSize,float playerPosX,float playerPosY, int array[],int arraySize)
{
	for (int i = 0;i < arraySize;++i) {
		array[i] = -1;
	}
	int attackCount = 0;
	for (int i = 0;i < arraySize;++i) {
		if (enemyList[i].enemyIndex == -1)continue;
		CP_Vector newPos = enemyMove(entities[enemyList[i].enemyIndex].posX, entities[enemyList[i].enemyIndex].posY,
			enemyList[i].nextWaypointX, enemyList[i].nextWaypointY, 100);
		entities[enemyList[i].enemyIndex].posX = newPos.x;
		entities[enemyList[i].enemyIndex].posY = newPos.y;
		
		//to do add in player pos and check if line of sight if have line of sight stop at next waypoint
		//----------------------------------------------------------------------------------------------
		if (lineOfSight(entities[enemyList[i].enemyIndex].posX, entities[enemyList[i].enemyIndex].posY, playerPosX, playerPosY,
			gridw, gridh, posX, posY, width, height, terrainSize)) {
			array[attackCount] = enemyList[i].enemyIndex;
			attackCount++;
			continue;
		}
		
		//this check if the enemy has reach its waypoint
		if (entities[enemyList[i].enemyIndex].posX == enemyList[i].nextWaypointX
			&& entities[enemyList[i].enemyIndex].posY == enemyList[i].nextWaypointY) {
			//gets new waypoint with rand
			CP_Vector nextPos = nextWaypoint(entities[enemyList[i].enemyIndex].posX, entities[enemyList[i].enemyIndex].posY,
				gridw, gridh, posX, posY, width, height, terrainSize);
			enemyList[i].nextWaypointX = nextPos.x;
			enemyList[i].nextWaypointY = nextPos.y;

		}
	}
}

void addEnemy(enemy enemy)
{
	int size = sizeof(enemyList) / sizeof(enemyList[0]);
	for (int i = 0;i < size;++i) {
		if (enemyList[i].enemyIndex != -1)continue;
		enemyList[i].enemyIndex = enemy.enemyIndex;
		enemyList[i].nextWaypointX = enemy.nextWaypointX;
		enemyList[i].nextWaypointY = enemy.nextWaypointY;
		return;
	}
}

void removeEnemy(int index)
{
	removeEntity(enemyList[index].enemyIndex);
	enemyList[index].enemyIndex = -1;
	enemyList[index].nextWaypointX = -1000;
	enemyList[index].nextWaypointY = -1000;
}

void enemyInit()
{
	int size = sizeof(enemyList) / sizeof(enemyList[0]);
	for (int i = 0;i < size;++i) {
		enemyList[i].enemyIndex = -1;
		enemyList[i].nextWaypointX = -1000;
		enemyList[i].nextWaypointY = -1000;
	}
}

void renderEnemyHitBox()
{
	int size = sizeof(enemyList) / sizeof(enemyList[0]);
	for (int i = 0;i < size;++i) {
		CP_Graphics_DrawRect(entities[i].posX - (entities[i].width / 2),
			entities[i].posY - (entities[0].height / 2),
			entities[i].width, entities[i].height);
	}
}
