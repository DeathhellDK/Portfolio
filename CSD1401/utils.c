/***********************************************************
 file:	utils.c
 author:	Terril					t.weiliang@digipen.edu
			Zhi Jie					zhijie.lim@digipen.edu
			Kai Yang				 k.kaiyang@digipen.edu
			Tingkai					ye.t@digipen.edu

 brief:	definition of functions and struct used in utils.c
	Terril - isTerrainRectangleCollision, isEnemyRectangleCollision2
	Zhi Jie - IsPlayerInArea
	Kai Yang - AreQuadIntersecting, initEntityArray, addEntity, removeEntity, renderEntities, isPlayerRectangleCollision2
	Ting Kai - IsAreaClicked


 Copyright © 2024 DigiPen, All rights reserved.
***********************************************************/

#include <math.h>
#include "cprocessing.h"
#include "utils.h"

#define gridw CP_System_GetWindowWidth() - CP_System_GetWindowHeight() * 0.1f
#define gridh CP_System_GetWindowHeight() * 0.8f - CP_System_GetWindowHeight() * 0.05f
#define sysWidth CP_System_GetWindowHeight() * 0.05f 
#define sysHeight CP_System_GetWindowHeight() * 0.2f

int IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y)
{
	//check for click for buttons
	if (((click_x >= area_center_x && click_x <= (area_center_x + area_width)) && (click_y >= area_center_y && click_y <= (area_center_y + area_height))) == TRUE)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

BOOL AreQuadIntersecting(float quadPos1X, float quadPos1Y, float width1,float height1, float quadPos2X, float quadPos2Y, float width2, float height2)
{
	//quad 1 bottom intersect with quad 2 top || quad 1 top intersect with quad 2 bottom
	if (quadPos1Y + height1 / 2 > quadPos2Y - height2 / 2 || quadPos1Y - height1 / 2 < quadPos2Y + height2 / 2) {
		//quad 1 right intersect with quad 2 left
		if (quadPos1X + width1 / 2 > quadPos2X - width2 / 2) return TRUE;
		//quad 1 left intersect with quad 2 right
		if (quadPos1X - width1 / 2 < quadPos2X + width2 / 2 ) return TRUE;
	}
	return FALSE;
}

void initEntityArray(entity entityList[], int size)
{
	for (int i = 0;i < size;++i) {
		entityList[i].posX = 0;
		entityList[i].posY = 0;
		entityList[i].width = 0;
		entityList[i].height = 0;
		entityList[i].color = CP_Color_Create(255, 255, 0, 255);
		entityList[i].image = NULL;
		entityList[i].entityType = START;
		entityList[i].renderType = null;
		entities[i].timer = 0.f;
	}
}

//return the index of the array being used
int addEntity(entity newEntity)
{
	int size = sizeof(entities) / sizeof(entities[0]);
	for (int i = 0;i < size;++i) {
		if (entities[i].entityType != null) continue;
		entities[i] = newEntity;
		if (entities[i].entityType == circle)
			(entities[i].height == 0) ? (entities[i].height = entities[i].width) : (entities[i].width = entities[i].height);
		return i;
	}
	return 0;
}

void removeEntity(int index) {
	entities[index].posX = 0;
	entities[index].posY = 0;
	entities[index].width = 0;
	entities[index].height = 0;
	entities[index].color = CP_Color_Create(255, 255, 0, 255);
	entities[index].image = NULL;
	entities[index].entityType = START;
	entities[index].renderType = null;
	entities[index].timer = 0.f;
}


//this function gets an array of entities and will go through the array and render them
int renderEntities()
{
	float radius = 0.0;
	int renderOrder = END - 1;

	//quad calculations
	float topLeftX, topleftY, botLeftX, botleftY, topRightX, topRightY, botRightX, botRightY;
	int size = sizeof(entities) / sizeof(entities[0]);
	while (renderOrder != START) {
		for (int i = 0;i < size; ++i) {
			if (entities[i].entityType != renderOrder) continue;
			CP_Settings_Fill(entities[i].color);
			switch (entities[i].renderType)
			{
			case circle:
				entities[i].height == 0 ? (radius = entities[i].width) : (radius = entities[i].height);
				CP_Graphics_DrawCircle(entities[i].posX, entities[i].posY, radius);
				break;
			case quad:
				topLeftX = entities[i].posX - (entities[i].width / 2);
				topleftY = entities[i].posY + (entities[i].height / 2);

				botLeftX = entities[i].posX - (entities[i].width / 2);
				botleftY = entities[i].posY + (entities[i].height / 2);

				topRightX = entities[i].posX + (entities[i].width / 2);
				topRightY = entities[i].posY + (entities[i].height / 2);

				botRightX = entities[i].posX + (entities[i].width / 2);
				botRightY = entities[i].posY - (entities[i].height / 2);
				CP_Graphics_DrawQuad(topLeftX, topleftY, topRightX, topRightY, botLeftX, botleftY, botRightX, botRightY);
				break;
			case triangle:
				//need to calculate point from pos x and pos y
				CP_Graphics_DrawTriangle(1, 1, 1, 1, 1, 1);
				break;
			case image:
				CP_Image_Draw(entities[i].image, entities[i].posX-(entities[i].width/2), 
					entities[i].posY - (entities[i].height / 2), 
					entities[i].width, entities[i].height, 255);
				break;
			default:
				break;
			}
		}
		renderOrder--;
	}
	

	return 1;
}

int IsPlayerInArea(float hitbox_x, float hitbox_y, float areax, float areay, float areaw, float areah){
	if ((hitbox_x > areax && hitbox_x < areax + areaw) && (hitbox_y>areay && hitbox_y <areay+areah)) {
		return 1;
	}
	return 0;
}


// For collision
BOOL isTerrainRectangleCollision(Terrain r1, Terrain r2)
{
	// Top Left of First Rec
	float r1_x = r1.posX;
	float r1_y = r1.posY;

	// Top Left of Second Rect
	float r2_x = r2.posX;
	float r2_y = r2.posY;

	///CP_Graphics_DrawRect(r2_x, r2_y, r2.width, r2.height);

	if (r1_x + r1.width > r2_x &&     // r1 right edge past r2 left
		r1_x < r2_x + r2.width &&       // r1 left edge past r2 right
		r1_y + r1.height > r2_y &&       // r1 top edge past r2 bottom
		r1_y < r2_y + r2.height) {       // r1 bottom edge past r2 top
		return 1;
	}
	return 0;
}

BOOL isEnemyRectangleCollision2(entity r1, Terrain r2)
{
	// Top Left of First Rec
	float r1_x = r1.posX;
	float r1_y = r1.posY;

	// Top Left of Second Rect
	float r2_x = r2.posX;
	float r2_y = r2.posY;

	///CP_Graphics_DrawRect(r2_x, r2_y, r2.width, r2.height);

	if (r1_x + r1.width > r2_x &&     // r1 right edge past r2 left
		r1_x < r2_x + r2.width &&       // r1 left edge past r2 right
		r1_y + r1.height > r2_y &&       // r1 top edge past r2 bottom
		r1_y < r2_y + r2.height) {       // r1 bottom edge past r2 top
		return 1;
	}
	return 0;
}


BOOL isPlayerRectangleCollision2(Terrain r1, Terrain r2)
{
	// Top Left of First Rec
	float r1_x = r1.posX;
	float r1_y = r1.posY;

	// Top Left of Second Rect
	float r2_x = r2.posX;
	float r2_y = r2.posY;

	///CP_Graphics_DrawRect(r2_x, r2_y, r2.width, r2.height);

	if (r1_x + r1.width > r2_x &&     // r1 right edge past r2 left
		r1_x < r2_x + r2.width &&       // r1 left edge past r2 right
		r1_y + r1.height > r2_y &&       // r1 top edge past r2 bottom
		r1_y < r2_y + r2.height) {       // r1 bottom edge past r2 top
		return 1;
	}
	return 0;
}