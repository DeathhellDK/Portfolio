/**************************************************************
 file:	loading.c
 author:	Ye Tingkai
 email:	ye.t@digipen.edu

 brief:	Contains the functions for the display screen on launch

 Copyright © 2024 DigiPen, All rights reserved.
***************************************************************/

#include "loading.h"
#include "game.h"
#include "cprocessing.h"
#include "stdbool.h"

BOOL isReseting;
float timer;

void load_init(void)
{
	isReseting = TRUE;
	timer = 0.5f;
	
}

void load_update(void)
{
	if (isReseting == TRUE)
	{
		timer -= CP_System_GetDt();
	}

	if (timer <= 0.0f)
	{
		isReseting = FALSE;
		timer = 0.0f;
		CP_Engine_SetNextGameState(Game_Init, Game_Update, Game_Exit);
	}
}

void load_exit(void)
{
	timer = 0.0f;
	isReseting = FALSE;
}