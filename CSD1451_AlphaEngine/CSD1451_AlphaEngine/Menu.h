/* Start Header ************************************************************************/
/*!
\file		Menu.h
\author		Hao Peng
\date		March, 20, 2025
\brief		Header file containing menu button structures
			and functions for the game's main menu interface

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef menu_H
#define menu_H

typedef struct menu_buttons {
	float scale_x, scale_y;
	float pos_x, pos_y;
	float alpha_pos_x, alpha_pos_y;

	float movement;
	float movement_speed;

	float Rotation;
	unsigned int color;
	AEMtx33 transform;

} menu_buttons;

void startgame(AEGfxVertexList* Mesh);
void howtoplay(AEGfxVertexList* Mesh);
void options(AEGfxVertexList* Mesh);
void credits(AEGfxVertexList* Mesh);
void quitbutton(AEGfxVertexList* Mesh);

// Function to load resources for the Menu state
void Menu_Load();

// Function to initialize the Menu state
void Menu_Initialize();

// Function to update the Menu state
void Menu_Update();

// Function to draw the Menu state
void Menu_Draw();

// Function to free resources for the Menu state
void Menu_Free();

// Function to unload the Menu state
void Menu_Unload();

#endif // MENU_H

