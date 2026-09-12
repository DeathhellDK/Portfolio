//---------------------------------------------------------
// file:	mainmenu.h
// author:	Ye Tingkai
// email:	ye.t@digipen.edu
//
// brief:	Contains the definitions for mainmenu.c
//			-	Main menu
//			-	Win/lose menu
//			-	How to play menu
//			-	Credit menu (Under gamestage definitions)
//
// Copyright © 2024 DigiPen, All rights reserved.
//---------------------------------------------------------
#include "player.h"

#pragma region gamestage definitions

void Main_Menu_Init(void);
void Main_Menu_Update(void);
void Main_Menu_Exit(void);

void HowTo_Menu_Init(void);
void HowTo_Menu_Update(void);
void HowTo_Menu_Exit(void);

void credits_Init(void);
void credits_Update(void);
void credits_Exit(void);

#pragma endregion

#pragma region main menu related definitions
//draws main menu button in game page that goes to mainmenu
void Main_Menu(void);

//combines multiple small draw functions into one main menu
void draw_main_menu(void);

//draws play button - used in main menu
void butn_play_draw(void);

//used in main menu
void play_handler(void);

//draws exit button - used in main menu
void butn_exit_draw(void);

//used in main menu
void exit_handler(void);

//used in main menu
void draw_credit_handler(void);
#pragma endregion

#pragma region win lose menu related definitions
//Win menu when user win
void Win_Menu(PlayerParameter* player);

// Retry menu when user loose
void Retry_Menu(PlayerParameter* player);

//displays all the stats of the player at end game including points earned
void display_score_stats(PlayerParameter* player);

//loads win lose images
void load_win_lose_img(void);

//generates a random number for death_stmts
void rand_num_gen_death(void);

//generates death statements and draws textbox
void encouraging_death_stmts_gen(void);
#pragma endregion

#pragma region how_to_play related definitions
//used in main menu
void butn_howToPlay_draw_handler(void);

//display and explains the contents of the game
void howToPlay_screen(void);

//initialise howToPlay gamestate
void howTo_init(void);

//loads assets for howToPlay only
void load_howToPlay_img(void);
#pragma endregion

#pragma region miscallaneous definitions
//general initialisation for miscallaneous variables
void general_init(void);

//free up loaded assets
void free_img_assets(void);

#pragma endregion





