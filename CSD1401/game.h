/***********************************************************
 file:	game.h
 author:	Terril					t.weiliang@digipen.edu
			Zhi Jie					zhijie.lim@digipen.edu

 brief:	Where the game runs, (add more)

 Copyright © 2024 DigiPen, All rights reserved.
***********************************************************/

#pragma once

#define gridW CP_System_GetWindowWidth() - CP_System_GetWindowHeight() * 0.1f
#define gridH CP_System_GetWindowHeight() * 0.8f - CP_System_GetWindowHeight() * 0.05f

typedef struct
{
    float posX, posY, width, height;

} Terrain;


void Game_Init(void);

void Game_Update(void);

void Game_Exit(void);

void gameicons(void);

void health(void);

void Game_Update2(void);