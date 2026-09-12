//---------------------------------------------------------
// file:	mainmenu.c
// author:	Ye Tingkai
// email:	ye.t@digipen.edu
//
// brief:	Contains the working functions to display: 
//			-	Main menu
//			-	Retry menu when player loses (Called in game.c)
//			-	Credits page (Built directly into functions used for gamestage loading)
//			-	How to play page.
//			
//			Sub functions:
//			-	Statsboard (Function called in Retry menu)
//			-	Main menu button (Reusable button, called in how_to_play and credits page)
//			-	Free Image assets (Called in game.c, mainmenu.c)
//			-	General initialisation (Initialises colour for buttons, background image , textsize for main menu)
// 
// Copyright © 2024 DigiPen, All rights reserved.
//---------------------------------------------------------

#include "cprocessing.h"
#include "mainmenu.h"
#include "game.h"
#include "utils.h"
#include "player.h"
#include "loading.h"
#include <stdio.h>
#include <stdlib.h>


#pragma region general_init
CP_Color butn_red;
CP_Color bckground_grey;
CP_Color text_black;
CP_Font butn_font;

CP_Image win_image;
CP_Image lose_image;

CP_Image menu_image;

CP_Image moves_icon;
CP_Image ammo_icon;
CP_Image enemy_icon;
CP_Image heart_icon;
CP_Image hazard_areas_icon;
CP_Image wall_icon;
CP_Image green_zone_icon;

int rd_num;
#pragma endregion

#pragma region Main menu related functions

//This is used only when application first initialises and shows main menu
void draw_main_menu()
{
	//draws the menu background image
	CP_Settings_ImageMode(CP_POSITION_CORNER);
	int menu_image_width = CP_Image_GetWidth(menu_image);
	int menu_image_height = CP_Image_GetHeight(menu_image);
	CP_Image_Draw(menu_image, 0.f, 0.f, (float)menu_image_width, (float)menu_image_height, 255);

	//draw buttons
	butn_play_draw();
	draw_credit_handler();
	butn_howToPlay_draw_handler();
	butn_exit_draw(); 
	

	//onclick play/exit/how button, set next gamestage/quit game
	play_handler();
	exit_handler();
}

// Main menu button - handler/draws button
void Main_Menu()
{
	//button is overlayed during gameplay menu
	//MAIN_MENU button drawing settings
	CP_Settings_Fill(butn_red);
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Graphics_DrawRect(50, 50, 150, 50);

	//draw play again text settings
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_Set(butn_font);
	CP_Settings_Fill(text_black);
	CP_Font_DrawText("Main Menu", 125, 75);

	//Check if MAIN_MENU button is clicked
	if ((CP_Input_MouseClicked() && IsAreaClicked(50, 50, 150, 50, CP_Input_GetMouseX(), CP_Input_GetMouseY())))
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
		free_img_assets();
	}
}

//draws play button for main menu
void butn_play_draw()
{
	//button drawing settings
	CP_Settings_Fill(butn_red);
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Graphics_DrawRect(700, 400, 200, 75);

	//button drawing settings
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_Set(butn_font);
	CP_Settings_Fill(text_black);
	CP_Font_DrawText("Play", 800, 435);
}

//Check if PLAY button is clicked in main menu
void play_handler()
{
	//Check if PLAY button is clicked
	if ((CP_Input_MouseClicked() && IsAreaClicked(700, 400, 200, 75, CP_Input_GetMouseX(), CP_Input_GetMouseY())))
	{
		CP_Engine_SetNextGameState(Game_Init, Game_Update, Game_Exit);
	}
}

//draws exit button for main menu
void butn_exit_draw()
{
	//button drawing settings
	CP_Settings_Fill(butn_red);
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Graphics_DrawRect(700, 700, 200, 75);

	//button drawing settings
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_Set(butn_font);
	CP_Settings_Fill(text_black);
	CP_Font_DrawText("EXIT", 800, 735);
}

//Check if EXIT button is clicked in main menu
void exit_handler()
{
	//onclick exit button, close application
	if (CP_Input_MouseClicked() && IsAreaClicked(700, 700, 200, 75, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		CP_Engine_Terminate();
	}
}

//draws credits button and checks if it is clicked
void draw_credit_handler() {

	//button drawing settings
	CP_Settings_Fill(butn_red);
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Graphics_DrawRect(700, 500, 200, 75);

	//button drawing settings
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_Set(butn_font);
	CP_Settings_Fill(text_black);
	CP_Font_DrawText("Credits", 800, 535);

	//onclick credits button, go to credits page
	if (CP_Input_MouseClicked() && IsAreaClicked(700, 500, 200, 75, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		CP_Engine_SetNextGameState(credits_Init, credits_Update, credits_Exit);
	}
}
#pragma endregion

#pragma region WIN/RETRY menu function
// Win menu when user win
void Win_Menu(PlayerParameter* player)
{
	//if (_Player->playerHealth > 0 && score > min_score) {}

	//overlay background on game page
	CP_Settings_Fill(bckground_grey);
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Graphics_DrawRect(0, 0, 1600, 900);

	//draw play again button
	CP_Settings_Fill(butn_red);
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Graphics_DrawRect(725, 550, 150, 100);

	//draw play again text settings
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_Set(butn_font);
	CP_Settings_Fill(text_black);
	CP_Font_DrawText("Play again ?", 800, 600);

	//Check if PLAY_AGAIN button is clicked
	if ((CP_Input_MouseClicked() && IsAreaClicked(725, 550, 150, 100, CP_Input_GetMouseX(), CP_Input_GetMouseY())))
	{
		CP_Engine_SetNextGameState(load_init, load_update, load_exit);
	}

	//draw go to main menu button
	CP_Settings_Fill(butn_red);
	CP_Graphics_DrawRect(675, 700, 250, 100);
	CP_Settings_Fill(text_black);
	CP_Font_DrawText("Quit to Main Menu", 800, 750);

	//Check if MAIN_MENU button is clicked
	if ((CP_Input_MouseClicked() && IsAreaClicked(675, 700, 250, 100, CP_Input_GetMouseX(), CP_Input_GetMouseY())))
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
	}

	//display lose image
	CP_Settings_ImageMode(CP_POSITION_CORNER);
	int win_width = CP_Image_GetWidth(win_image);
	int win_height = CP_Image_GetHeight(win_image);
	CP_Image_Draw(win_image, 300.f, 200.f, (float)win_width, (float)win_height, 255);

	display_score_stats(player);
}

// Retry menu when user loose
void Retry_Menu(PlayerParameter* player)
{
	//overlay background on game page
	CP_Settings_TextSize(30);
	CP_Settings_Fill(bckground_grey);
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Graphics_DrawRect(0, 0, 1600, 900);

	load_win_lose_img();

	//retry button drawing settings
	CP_Settings_Fill(butn_red);
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Graphics_DrawRect(725, 550, 150, 100);

	//retry text settings
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_Set(butn_font);
	CP_Settings_Fill(text_black);
	CP_Font_DrawText("RETRY", 800, 600);
	encouraging_death_stmts_gen(); 
		
	//Check if RETRY button is clicked
	if ((CP_Input_MouseClicked() && IsAreaClicked(725, 550, 150, 100, CP_Input_GetMouseX(), CP_Input_GetMouseY())))
	{
		
		CP_Engine_SetNextGameState(load_init, load_update, load_exit);
		
	}

	//draw go to main menu button
	CP_Settings_Fill(butn_red);
	CP_Graphics_DrawRect(675, 700, 250, 100);
	CP_Settings_Fill(text_black);
	CP_Font_DrawText("Quit to Main Menu", 800, 750);

	//Check if MAIN_MENU button is clicked
	if ((CP_Input_MouseClicked() && IsAreaClicked(675, 700, 250, 100, CP_Input_GetMouseX(), CP_Input_GetMouseY())))
	{
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
	}

	//display lose image
	CP_Settings_ImageMode(CP_POSITION_CORNER);
	int lose_width = CP_Image_GetWidth(lose_image);
	int lose_height = CP_Image_GetHeight(lose_image);
	CP_Image_Draw(lose_image, 300.f, 200.f, (float)lose_width, (float)lose_height, 255);

	//Display stats
	display_score_stats(player);
}

//loads win lose images
void load_win_lose_img()
{
	win_image = CP_Image_Load("Assets/win_img.png");
	lose_image = CP_Image_Load("Assets/lose_img.png");
}

//random number generator for death_stmts_gen
void rand_num_gen_death()
{
	int min = 0, max = 19;
	rd_num = rand() % (max - min + 1) + min;
}

//generates a death text to screen
void encouraging_death_stmts_gen()
{
	const char *death_stmts[20] = 
	{
		"Curb your enthusiasm",
		"Practice makes perfect",
		"You need the power of luck on your side",
		"Luck matters",
		"Skill issue ? I hope not.",
		"Revenge is best served cold",
		"Were you even trying ?",
		"Mission failed, we'll get 'em next time",
		"Challenge against fate again ?",
		"It was a bad case of RNG",
		"I was just one move short, I swear !",
		"Tis just a flesh wound",
		"You will be him, push harder",
		"Lack of morning coffee ?",
		"Almost there. Time to lock in.",
		"Perhaps a toilet break is all you need.",
		"Rats. Almost made it.",
		"Have a break, Have some skill",
		"So close...",
		"Have Patience -Sun Tzu, The Art of War"
	};

	CP_Font_DrawText(death_stmts[rd_num], 800, 500);
	
}

//displays score and stats of player in the current game session
void display_score_stats(PlayerParameter* player)
{
	//display area drawing settings
	CP_Settings_Fill(bckground_grey);
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Graphics_DrawRect(850, 150, 300, 300);

	//display stats
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_LEFT, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_Set(butn_font);
	CP_Settings_Fill(text_black);

	//display statistics using player pointer 
	CP_Font_DrawText("Scoreboard", 925, 175);

	char ammo[50] = {0};
	sprintf_s(ammo, 50, "Ammo Left: %d", player->playerAmmo);
	CP_Font_DrawText(ammo, 875, 250);

	/*char attack[50] = {0};
	sprintf_s(attack, 50, "Attacks Left: %d", player->playerAttack);
	CP_Font_DrawText(attack, 875, 275);*/

	char movement[50] = {0};
	sprintf_s(movement, 50, "Movements Left: %d", player->playerMoveRemain);
	CP_Font_DrawText(movement, 875, 300);

	//currently static
	//CP_Font_DrawText("Points: 9000/30000", 875, 350);
	//CP_Font_DrawText("Enemies killed: 3", 875, 375);

}

#pragma endregion

#pragma region Miscallaneous functions

//general initilisation for game assets
void general_init()
{
	CP_System_SetWindowSize(1600, 900);

	butn_red = CP_Color_Create(255, 125, 125, 255);
	bckground_grey = CP_Color_Create(211, 211, 211, 255);
	text_black = CP_Color_Create(0, 0, 0, 255);

	CP_Settings_TextSize(30);
	butn_font = CP_Font_Load("Assets/Exo2-Regular.ttf");

	menu_image = CP_Image_Load("Assets/menu_img.png");
}

//frees up assets used
void free_img_assets()
{
	CP_Image_Free(&menu_image);

	CP_Image_Free(&win_image);
	CP_Image_Free(&lose_image);

	CP_Image_Free(&moves_icon);
	CP_Image_Free(&ammo_icon);
	CP_Image_Free(&enemy_icon);
	CP_Image_Free(&heart_icon);
	CP_Image_Free(&wall_icon);
	CP_Image_Free(&green_zone_icon);
	CP_Image_Free(&hazard_areas_icon);
}

#pragma endregion

#pragma region How_to_play related functions

//standalone handler button called by main menu
void butn_howToPlay_draw_handler()
{
	//button drawing settings
	CP_Settings_Fill(butn_red);
	CP_Settings_RectMode(CP_POSITION_CORNER);
	CP_Graphics_DrawRect(700, 600, 200, 75);

	//button text drawing settings
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_Set(butn_font);
	CP_Settings_Fill(text_black);
	CP_Font_DrawText("HOW TO PLAY", 800, 635);

	//Check if HOW TO PLAY button is clicked
	if ((CP_Input_MouseClicked() && IsAreaClicked(700, 600, 200, 75, CP_Input_GetMouseX(), CP_Input_GetMouseY())))
	{
		CP_Engine_SetNextGameState(HowTo_Menu_Init, HowTo_Menu_Update, HowTo_Menu_Exit);
	}
}

//draw sprites and display text to explain game
void howToPlay_screen()
{
	//draw main menu button
	Main_Menu();

	//text settings
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_LEFT, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_Set(butn_font);
	CP_Settings_Fill(text_black);

	//explain moves
	CP_Settings_ImageMode(CP_POSITION_CORNER);
	int moves_width = CP_Image_GetWidth(moves_icon);
	int moves_height = CP_Image_GetHeight(moves_icon);
	CP_Image_Draw(moves_icon, 50.f, 125.f, (float)moves_width, (float)moves_height, 255);
	CP_Font_DrawText("This is your moves left.", 240, 175);
	CP_Font_DrawText("Tap WASD keys to move", 240, 200);
	CP_Font_DrawText("Use them sparingly !", 240, 225);

	//explain ammo
	int ammo_width = CP_Image_GetWidth(ammo_icon);
	int ammo_height = CP_Image_GetHeight(ammo_icon);
	CP_Image_Draw(ammo_icon, 70.f, 300.f, (float)ammo_width, (float)ammo_height, 255);
	CP_Font_DrawText("This is your ammo left.", 240, 350);
	CP_Font_DrawText("Left mouse button to fire", 240, 375);
	CP_Font_DrawText("Use them sparingly !", 240, 400);

	//explain enemy
	int enemy_width = CP_Image_GetWidth(enemy_icon);
	int enemy_height = CP_Image_GetHeight(enemy_icon);
	CP_Image_Draw(enemy_icon, 50.f, 475.f, (float)enemy_width, (float)enemy_height, 255);
	CP_Font_DrawText("Displays enemies left.", 240, 525);
	CP_Font_DrawText("Kill to earn points enough to win", 240, 550);

	//explain heart
	int heart_width = CP_Image_GetWidth(heart_icon);
	int heart_height = CP_Image_GetHeight(heart_icon);
	CP_Image_Draw(heart_icon, 50.f, 675.f, (float)heart_width, (float)heart_height, 255);
	CP_Font_DrawText("Your health.", 240, 675);
	CP_Font_DrawText("1 full heart = 2 health points", 240, 700);
	CP_Font_DrawText("1 half heart = 1 health points", 240, 725);
	CP_Font_DrawText("Do not take any damage", 240, 750);
	CP_Font_DrawText("This game is hard.", 240, 775);

	//explain hazard_areas
	int hazard_areas_width = CP_Image_GetWidth(hazard_areas_icon);
	int hazard_areas_height = CP_Image_GetHeight(hazard_areas_icon);
	CP_Image_Draw(hazard_areas_icon, 740.f, 125.f, (float)hazard_areas_width, (float)hazard_areas_height, 255);
	CP_Font_DrawText("Hazard areas.", 1050, 150);
	CP_Font_DrawText("Entering this area will deduct your health.", 1050, 175);

	//explain wall
	int wall_width = CP_Image_GetWidth(wall_icon);
	int wall_height = CP_Image_GetHeight(wall_icon);
	CP_Image_DrawAdvanced(wall_icon, 750.f, 250.f, (float)wall_width, (float)wall_height, 255, 90.f);
	CP_Font_DrawText("A wall, impassable.", 1050, 350);
	CP_Font_DrawText("Either what is between you and them,", 1050, 375);
	CP_Font_DrawText("Or what traps you with them.", 1050, 400);

	//explain green_zone
	int green_zone_width = CP_Image_GetWidth(green_zone_icon);
	int green_zone_height = CP_Image_GetHeight(green_zone_icon);
	CP_Image_Draw(green_zone_icon, 725.f, 475.f, (float)green_zone_width, (float)green_zone_height, 255);
	CP_Font_DrawText("The green zone.", 1050, 525);
	CP_Font_DrawText("kill enough and enter to pass", 1050, 550);
	CP_Font_DrawText("Or kill all and get a higher score", 1050, 575);

}

//initialisation for how to play screen
void howTo_init()
{
	CP_System_SetWindowSize(1600, 900);

	butn_red = CP_Color_Create(255, 125, 125, 255);
	bckground_grey = CP_Color_Create(211, 211, 211, 255);
	text_black = CP_Color_Create(0, 0, 0, 255);

	CP_Settings_TextSize(30);
	butn_font = CP_Font_Load("Assets/Exo2-Regular.ttf");
}

//load image assets for howToPlay
void load_howToPlay_img()
{
	moves_icon = CP_Image_Load("Assets/moves_img.png");
	ammo_icon = CP_Image_Load("Assets/ammo_img.png");
	enemy_icon = CP_Image_Load("Assets/enemy_img.png");
	heart_icon = CP_Image_Load("Assets/heart_img.png");
	wall_icon = CP_Image_Load("Assets/wall_img.png");
	green_zone_icon = CP_Image_Load("Assets/green_zone_img.png");
	hazard_areas_icon = CP_Image_Load("Assets/dmg_area_img.png");
}

void HowTo_Menu_Init(void)
{
	howTo_init();
	load_howToPlay_img();
}

void HowTo_Menu_Update(void)
{
	//refresh bckground
	CP_Graphics_ClearBackground(bckground_grey);
	howToPlay_screen();
}

void HowTo_Menu_Exit(void)
{
	free_img_assets();
}

#pragma endregion

#pragma region Credits related functions

//initialises background, text
void credits_Init(void)
{
	//initialisation
	CP_System_SetWindowSize(1600, 900);

	butn_red = CP_Color_Create(255, 125, 125, 255);
	bckground_grey = CP_Color_Create(211, 211, 211, 255);
	text_black = CP_Color_Create(0, 0, 0, 255);

	CP_Settings_TextSize(30);
	butn_font = CP_Font_Load("Assets/Exo2-Regular.ttf");
}

//displays credits
void credits_Update(void)
{
	//refresh background
	CP_Graphics_ClearBackground(bckground_grey);

	CP_Settings_TextSize(30);
	Main_Menu();

	//set text size
	CP_Settings_TextSize(50);

	//credits here
	char digipen[200] = { 0 };
	sprintf_s(digipen, 200, "All content %s 2024 DigiPen Institute of Technology Singapore, all rights reserved.", "\xC2\xA9");
	CP_Font_DrawText(digipen, 800, 200);

	CP_Font_DrawText("Game Instructors:", 800, 300);
	CP_Font_DrawText("Ding Xiang Cheng", 800, 400);
	CP_Font_DrawText("Gerald Wong", 800, 450);

	CP_Font_DrawText("Contributors:", 800, 550);
	CP_Font_DrawText("Terril Tan", 800, 650);
	CP_Font_DrawText("Lim Zhi Jie", 800, 700);
	CP_Font_DrawText("Koh Kai Yang", 800, 750);
	CP_Font_DrawText("Ye Tingkai", 800, 800);

}

void credits_Exit(void){}

#pragma endregion

#pragma region Main Menu Init functions

void Main_Menu_Init(void)
{
	general_init();
}

void Main_Menu_Update(void)
{
	//refresh bckground
	CP_Graphics_ClearBackground(bckground_grey);

	draw_main_menu();
}

void Main_Menu_Exit(void)
{
	free_img_assets();
}

#pragma endregion

