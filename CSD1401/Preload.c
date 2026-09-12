//---------------------------------------------------------
// file:	Preload.c
// author:	Ye Tingkai
// email:	ye.t@digipen.edu
//
// brief:	Displays the digipen logo for 2 seconds before loading
//			into main menu.
//
// Copyright © 2024 DigiPen, All rights reserved.
//---------------------------------------------------------


#include "loading.h"
#include "cprocessing.h"
#include "stdbool.h"
#include "mainmenu.h"

BOOL isReseting;
float timer;
CP_Image digipen_logo;
CP_Color bckground_black;

//loads the digipen logo, sets the timer
void preload_init(void)
{
	isReseting = TRUE;
	timer = 2.0f;

	digipen_logo = CP_Image_Load("Assets/DigiPen_Singapore_WEB_WHITE.png");
	bckground_black = CP_Color_Create(0, 0, 0, 255);
}

//draws the digipen logo for 2 seconds before going into main menu of the game
void preload_update(void)
{
	CP_Graphics_ClearBackground(bckground_black);

	if (isReseting == TRUE)
	{
		timer -= CP_System_GetDt();
	}

	//draws the digipen logo
	CP_Settings_ImageMode(CP_POSITION_CORNER);
	int digipen_logo_width = CP_Image_GetWidth(digipen_logo);
	int digipen_logo_height = CP_Image_GetHeight(digipen_logo);
	CP_Image_Draw(digipen_logo, 30.f, 225.f, (float)digipen_logo_width, (float)digipen_logo_height, 255);

	if (timer <= 0.0f)
	{
		isReseting = FALSE;
		timer = 0.0f;
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
	}
}

//free up the digipen logo
void preload_exit(void)
{
	timer = 0.0f;
	isReseting = FALSE;
	CP_Image_Free(&digipen_logo);
}