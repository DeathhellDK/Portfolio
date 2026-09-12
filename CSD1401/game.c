/***********************************************************
 file:	game.c
 author:	Terril					t.weiliang@digipen.edu
			Zhi Jie					zhijie.lim@digipen.edu
			Kai Yang				 k.kaiyang@digipen.edu

 brief: Game.c contains the drawing of the game graphics, game ui. 
		Calls upon functions from player.h, bullet.h, utils.h, enemy.h

 Copyright © 2024 DigiPen, All rights reserved.
***********************************************************/

#include "cprocessing.h"
#include "game.h"
#include "player.h"
#include "mainmenu.h"
#include "loading.h"
#include "enemy.h"
#include "bullet.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

entity entities[10];

#define NUM_TERRAIN 9

#pragma region general_init
CP_Color bckground_grey;
CP_Color text_black;
CP_Font butn_font;
#pragma endregion

float gridw, gridh, gridstartx, gridstarty;

float terrainx[] = { 0.0f, 0.1f, 0.1f, 0.5f, 0.7f, 0.0f, 0.0f, 0.0f, 1.0f}; // Do not remove the last 4 value: 0.0f, 0.0f, 0.0f, 1.0f
float terrainy[] = { 0.2f, 0.4f, 0.6f, 0.2f, 0.7f, 0.0f, 0.0f, 1.0f, 0.0f}; // Do not remove the last 4 value: 0.0f, 0.0f, 1.0f, 0.0f
float terrainwidth[] = { 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 1.0f, 0.001f, 1.0f, 0.001f }; // Do not remove the last 4 value: 1.0f, 0.001f, 1.0f, 0.001f
float terrainheight[] = { 0.1f, 0.1f, 0.3f, 0.3f, 0.3f, 0.001f, 1.0f, 0.001f, 1.0f }; // Do not remove the last 4 value: 0.001f, 1.0f, 0.001f, 1.0f
int prevenemycount = 0;
int hasentered, hasentered1, hasentered2, hasentered3, clear, score, effect;

CP_Font font;
CP_Image life = NULL;
CP_Image moves = NULL;
CP_Image attacks = NULL;
CP_Image enemies = NULL;
CP_Image hp0 = NULL;
CP_Image hp1 = NULL;
CP_Image hp2 = NULL;
CP_Image dmgarea = NULL;
CP_Image clearpt = NULL;
CP_Image star0 = NULL;
CP_Image star1 = NULL;
CP_Image win = NULL;

Terrain terrain[NUM_TERRAIN];

float gameterrainPosX = 0.0f;
float gameterrainPosY = 0.0f;
float damageterrainPoxX = 0.0f;
float damgaeterrainPosY = 0.0f;

/// This file is to create and control the player behavior
PlayerParameter player, * _player;

// Create a pointer to a dynamically allocated array of bullets
BulletParameter playerBullet, * _playerBullet;
BulletParameter enemyBullet, * _enemyBullet;
int enemyBulletSize = 0;
int bullet_index = 0;

int enemy_index = 0;
int bulletCounter = 0;
int enemyBulletcounter = 0;

float originPosX = 0;
float originPosY = 0;

void Game_Init(void)
{
	//zhi jie code
	gridstartx = CP_System_GetWindowHeight() * 0.05f;
	gridstarty = CP_System_GetWindowHeight() * 0.2f;
	gridw = CP_System_GetWindowWidth() - CP_System_GetWindowHeight() * 0.1f;
	gridh = (CP_System_GetWindowHeight() * 0.8f - CP_System_GetWindowHeight() * 0.05f);
	life = CP_Image_Load("./Assets/life.png");
	attacks = CP_Image_Load("./Assets/attack.png");
	moves = CP_Image_Load("./Assets/move.png");
	enemies = CP_Image_Load("./Assets/enemy.png");
	hp0 = CP_Image_Load("./Assets/hp0.png");
	hp1 = CP_Image_Load("./Assets/hp1.png");
	hp2 = CP_Image_Load("./Assets/hp2.png");
	dmgarea = CP_Image_Load("./Assets/damage.png");
	clearpt = CP_Image_Load("./Assets/clear.png");
	star0 = CP_Image_Load("./Assets/star0.png");
	star1 = CP_Image_Load("./Assets/star.png");
	win = CP_Image_Load("./Assets/win_img.png");
	font = CP_Font_Load("Assets/Exo2-Regular.ttf");

	gameterrainPosX = CP_System_GetWindowHeight() * 0.05f;
	gameterrainPosY = CP_System_GetWindowHeight() * 0.2f;
	damageterrainPoxX = CP_System_GetWindowHeight() * 0.05f + 0.5f * gridw;
	damgaeterrainPosY = CP_System_GetWindowHeight() * 0.2f + 0.5f * gridh;

	for (int i = 0; i < NUM_TERRAIN; i++)
	{
		terrain[i].posX = CP_System_GetWindowHeight() * 0.05f + terrainx[i] * gridw;
		terrain[i].posY = CP_System_GetWindowHeight() * 0.2f + terrainy[i] * gridh;
		terrain[i].width = terrainwidth[i] * gridw;
		terrain[i].height = terrainheight[i] * gridh;
	}
	enemyBulletSize = 150;
	printf("enemy bullet size: %d\n", enemyBulletSize);

	bullet_index = 0;
	// For debug
	if (bullet_index == 0)
	{
		printf("Bullet counter reset\n");
	}

	_player = &player;
	_playerBullet = &playerBullet;
	_enemyBullet = &enemyBullet;
	
	Player_Init(_player);
	bulletCounter = _player->playerAmmo;
	enemyBulletcounter = enemyBulletSize;
	_playerBullet = malloc(sizeof(BulletParameter) * bulletCounter);
	_enemyBullet = malloc(sizeof(BulletParameter) * enemyBulletcounter);
	bullet_Init(_playerBullet, bulletCounter,MC);
	bullet_Init(_enemyBullet, enemyBulletcounter,NPC);

	//kai yang code
	initEntityArray(entities, sizeof(entities) / sizeof(entities[0]));
	enemyInit();

	float middleX = gridw * 0.1f * 0.5f;
	float middleY = gridh * 0.1f * 0.5f;
	entity newEntity = { .posX = CP_System_GetWindowHeight() * 0.05f + middleX * 17,.posY = CP_System_GetWindowHeight() * 0.2f + 1 * (middleY) ,.renderType = image,.entityType = ENEMY,.height = 50 ,.width = 50,.image = enemies };

	enemy newEnemy = { .enemyIndex = addEntity(newEntity),.nextWaypointX = CP_System_GetWindowHeight() * 0.05f + middleX * 17,
		.nextWaypointY = CP_System_GetWindowHeight() * 0.2f + 1*(middleY)};
	addEnemy(newEnemy);

	entity newEntity2 = { .posX = CP_System_GetWindowHeight() * 0.05f + middleX * 9,.posY = CP_System_GetWindowHeight() * 0.2f + 1 * (middleY) ,.renderType = image,.entityType = ENEMY,.height = 50 ,.width = 50,.image = enemies };

	enemy newEnemy2 = { .enemyIndex = addEntity(newEntity2),.nextWaypointX = CP_System_GetWindowHeight() * 0.05f + middleX * 9,
		.nextWaypointY = CP_System_GetWindowHeight() * 0.2f + 1 * (middleY) };
	addEnemy(newEnemy2);
	
	entity newEntity3 = { .posX = CP_System_GetWindowHeight() * 0.05f + middleX * 11,.posY = CP_System_GetWindowHeight() * 0.2f + 17 * (middleY) ,.renderType = image,.entityType = ENEMY,.height = 50 ,.width = 50,.image = enemies };

	enemy newEnemy3 = { .enemyIndex = addEntity(newEntity3),.nextWaypointX = CP_System_GetWindowHeight() * 0.05f + middleX * 11,
		.nextWaypointY = CP_System_GetWindowHeight() * 0.2f + 17 * (middleY) };
	addEnemy(newEnemy3);

	entity newEntity4 = { .posX = CP_System_GetWindowHeight() * 0.05f + middleX * 19,.posY = CP_System_GetWindowHeight() * 0.2f + 9 * (middleY) ,.renderType = image,.entityType = ENEMY,.height = 50 ,.width = 50,.image = enemies };

	enemy newEnemy4 = { .enemyIndex = addEntity(newEntity4),.nextWaypointX = CP_System_GetWindowHeight() * 0.05f + middleX * 19,
		.nextWaypointY = CP_System_GetWindowHeight() * 0.2f + 9 * (middleY) };
	addEnemy(newEnemy4);

	hasentered = 0, hasentered1 = 0, hasentered2 = 0, hasentered3 = 0, clear = 0, score = 0, _player->EnemyTOKill = 4, effect = 0, prevenemycount = _player->EnemyTOKill;
}

void Game_Update(void)
{
	//kai yang code
	CP_Graphics_ClearBackground(CP_Color_Create(160, 160, 160, 255));
	CP_Settings_ImageMode(CP_POSITION_CORNER);

	// hp 
	health();

	//zhijie code
	gameicons();

	// area/grid for player to move in (blue)
	CP_Settings_Fill(CP_Color_Create(0, 191, 255, 255));
	CP_Graphics_DrawRect(gridstartx, gridstarty, gridw, gridh);

	// damage area (red)
	CP_Settings_Fill(CP_Color_Create(255, 40, 0, 255));
	CP_Graphics_DrawRect(gridstartx + 0.5f * gridw,gridstarty + 0.5f * gridh, 0.3f * gridw, 0.2f * gridh);
	CP_Image_Draw(dmgarea, gridstartx + 0.5f * gridw, gridstarty + 0.5f * gridh, 0.3f * gridw, 0.2f * gridh, 255);
	CP_Graphics_DrawRect(gridstartx + 0.1f * gridw, gridstarty + 0.2f * gridh, 0.2f * gridw, 0.2f * gridh);
	CP_Image_Draw(dmgarea, gridstartx + 0.1f * gridw, gridstarty + 0.2f * gridh, 0.2f * gridw, 0.2f * gridh, 255);
	CP_Graphics_DrawRect(gridstartx + 0.2f * gridw, gridstarty - 0.001f* gridh, 0.2f * gridw, 0.2f * gridh);
	CP_Image_Draw(dmgarea, gridstartx + 0.2f * gridw, gridstarty - 0.001f * gridh, 0.2f * gridw, 0.2f * gridh, 255);

	// terrain (dark grey)
	CP_Settings_Fill(CP_Color_Create(128, 128, 128, 255));

	for (int i = 0; i < NUM_TERRAIN; i++)
	{
		CP_Graphics_DrawRect(terrain[i].posX, terrain[i].posY, terrain[i].width, terrain[i].height);
	}

	// clearpoint (green)
	CP_Settings_Fill(CP_Color_Create(0, 255, 0, 255));
	CP_Graphics_DrawRect(gridstartx + 0.9f * gridw, gridstarty + 0.6f * gridh, 0.1f * gridw, 0.1f * gridh);
	CP_Image_Draw(clearpt, gridstartx + 0.9f * gridw, gridstarty + 0.6f * gridh, 0.1f * gridw, 0.1f * gridh, 255);	

	// cheat buttons
	if (CP_Input_KeyTriggered(KEY_P)) {
		for (int i = 0;i < sizeof(entities) / sizeof(entities[0]);++i) {
			if (entities[i].entityType == ENEMY) {
				removeEnemy(i);
				break;
			}
		}
		_player->EnemyTOKill -= 1;
	}

	if (CP_Input_KeyTriggered(KEY_O)) {
		_player->playerHealth = 0;
	}


	// checking for player inside areas
	// damage area check
	if (IsPlayerInArea(_player->player_HitboxPosX, _player->player_HitboxPosY, gridstartx + 0.5f * gridw, gridstarty + 0.5f * gridh, 0.1f * gridw, 0.2f * gridh)
	|| IsPlayerInArea(_player->player_HitboxPosX, _player->player_HitboxPosY, gridstartx + 0.1f * gridw, gridstarty + 0.3f * gridh, 0.1f * gridw, 0.1f * gridh)
	|| IsPlayerInArea(_player->player_HitboxPosX, _player->player_HitboxPosY, gridstartx + 0.2f * gridw, gridstarty, 0.1f * gridw, 0.2f * gridh)) {
		if (!hasentered) {
			_player->playerHealth -= 1;
			hasentered = 1;
		}
	}
	else hasentered = 0;
	if (IsPlayerInArea(_player->player_HitboxPosX, _player->player_HitboxPosY, gridstartx + 0.6f * gridw, gridstarty + 0.5f * gridh, 0.1f * gridw, 0.2f * gridh) 
		|| IsPlayerInArea(_player->player_HitboxPosX, _player->player_HitboxPosY, gridstartx + 0.2f * gridw, gridstarty + 0.3f * gridh, 0.1f * gridw, 0.1f * gridh)
		|| IsPlayerInArea(_player->player_HitboxPosX, _player->player_HitboxPosY, gridstartx + 0.3f * gridw, gridstarty, 0.1f * gridw, 0.2f * gridh)) {
		if (!hasentered1) {
			_player->playerHealth -= 1;
			hasentered1 = 1;
		}
	}
	else hasentered1 = 0;
	if (IsPlayerInArea(_player->player_HitboxPosX, _player->player_HitboxPosY, gridstartx + 0.7f * gridw, gridstarty + 0.5f * gridh, 0.1f * gridw, 0.2f * gridh)) {
		if (!hasentered2) {
			_player->playerHealth -= 1;
			hasentered2 = 1;
		}
	}
	else hasentered2 = 0;

	// clear area check
	if (IsPlayerInArea(_player->player_HitboxPosX, _player->player_HitboxPosY, gridstartx + 0.9f * gridw, gridstarty + 0.6f * gridh, 0.1f * gridw, 0.1f * gridh)) {
		if (!hasentered3) {
			if (_player->EnemyTOKill == 0) score += 1;
			if (_player->playerMoveRemain > 20) score += 1;
			clear = 1;
			score += 1;
			hasentered3 = 1;
		}
	}
	else hasentered3 = 0, clear = 0;

	// effect upon killing enemy generates a random number which corresponds to the effect obtained
	CP_Settings_TextSize(30);
	if (_player->EnemyTOKill < prevenemycount) {
		effect = CP_Random_RangeInt(1, 12);
		prevenemycount = _player->EnemyTOKill;
		switch (effect) {
		case 1:
			_player->playerAmmo += 1;
			break;
		case 2:
			_player->playerAmmo += 1;
			break;
		case 3:
			_player->playerAmmo += 1;
			break;
		case 4:
			_player->playerMoveRemain += 5;
			break;
		case 5:
			_player->playerMoveRemain += 5;
			break;
		case 6:
			_player->playerMoveRemain += 5;
			break;
		case 7:
			_player->playerHealth += 1;
			break;
		case 8:
			_player->playerHealth += 1;
			break;
		case 9:
			_player->playerHealth += 1;
			break;
		case 10:
			_player->playerAmmo -= 1;
			break;
		case 11:
			_player->playerMoveRemain -= 5;
			break;
		case 12:
			_player->playerHealth -= 1;
			break;
		}
	}

	// Draws the effect obtained at the bottom left of the screen
	if (effect >= 1 && effect <= 3) {
		CP_Settings_Fill(CP_Color_Create(0, 255, 0, 255));
		CP_Font_DrawText("You got 1 extra ammo!", 175, CP_System_GetWindowHeight() * 0.975f);
	}
	if (effect >= 4 && effect <= 6) {
		CP_Settings_Fill(CP_Color_Create(0, 255, 0, 255));
		CP_Font_DrawText("You got 5 extra moves!", 175, CP_System_GetWindowHeight() * 0.975f);
	}
	if (effect >= 7 && effect <= 9) {
		CP_Settings_Fill(CP_Color_Create(0, 255, 0, 255));
		CP_Font_DrawText("You got 1 extra health!", 175, CP_System_GetWindowHeight() * 0.975f);
	}
	if (effect == 10) {
		CP_Settings_Fill(CP_Color_Create(255, 40, 0, 255));
		CP_Font_DrawText("You lost 1 ammo!", 175, CP_System_GetWindowHeight() * 0.975f);
	}
	if (effect == 11) {
		CP_Settings_Fill(CP_Color_Create(255, 40, 0, 255));
		CP_Font_DrawText("You lost 5 moves!", 175, CP_System_GetWindowHeight() * 0.975f);
	}
	if (effect == 12) {
		CP_Settings_Fill(CP_Color_Create(255, 40, 0, 255));
		CP_Font_DrawText("You lost 1 extra health!", 175, CP_System_GetWindowHeight() * 0.975f);
	}
	if (effect == 0) {
		CP_Font_DrawText("", 165, CP_System_GetWindowHeight() * 0.975f);
	}

	Player_Action_Handler(_player, terrain, NUM_TERRAIN);
	if (CP_Input_MouseTriggered(MOUSE_BUTTON_1) && _player->playerAmmo > 0)
	{
		if (_player->isAlive == TRUE)
		{
			bullet_Spawn(&_playerBullet[bullet_index], _player->playerPosX, _player->playerPosY, _player->playerRotation,MC);
			bullet_index++;
			_player->playerAmmo -= 1;
		}
	}

	//kai yang code
	int temp[10];
	updateEnemy(gridw, gridh, terrainx, terrainy, terrainwidth, terrainheight, (int)sizeof(terrainx) / (int)sizeof(terrainx[0]), _player->playerPosX, _player->playerPosY, temp, 10);
	;
	CP_Vector upVec = CP_Vector_Set(0, 1);
	for (int attackingIndex = 0;attackingIndex < 10;++attackingIndex) {
		if (temp[attackingIndex] > -1) {
			int index = temp[attackingIndex];
			//rotation
			CP_Vector dir = CP_Vector_Set(_player->player_HitboxPosX - entities[index].posX, _player->player_HitboxPosY - entities[index].posY);
			float angle = CP_Vector_AngleCCW(upVec, CP_Vector_Set(dir.x, -dir.y));

			if (_player->isAlive == TRUE)
			{
				if (entities[index].timer >= 1.0f) {
					if (315.f <= angle || angle <= 44.f) {
						//shoot up
						bullet_Spawn(&_enemyBullet[enemy_index],
							entities[index].posX - entities[index].width / 2 - 10.0f,
							entities[index].posY - entities[index].height / 2 - 10.0f, 0, NPC);
					}
					if (45.f <= angle && angle <= 134.f) {
						//shoot right
						bullet_Spawn(&_enemyBullet[enemy_index],
							entities[index].posX - entities[index].width / 2 - 10.0f,
							entities[index].posY - entities[index].height / 2 - 10.0f, 90, NPC);
					}
					if (135.f <= angle && angle <= 224.f) {
						//shoot down
						bullet_Spawn(&_enemyBullet[enemy_index],
							entities[index].posX - entities[index].width / 2 - 10.0f,
							entities[index].posY, 180, NPC);
					}
					if (225.f <= angle && angle <= 314.f) {
						//shoot left
						bullet_Spawn(&_enemyBullet[enemy_index],
							entities[index].posX - entities[index].width / 2 - 10.0f,
							entities[index].posY - entities[index].height / 2 - 10.0f, 270, NPC);
					}

					entities[index].timer = 0.f;
					enemyBulletcounter--;
					enemy_index++;
				}
			}
		}
	}
	
	bullet_MovementHandler(_enemyBullet, _player, terrain, entities, NUM_TERRAIN, enemyBulletcounter, NPC);

	bullet_MovementHandler(_playerBullet, _player, terrain, entities, NUM_TERRAIN, bulletCounter, MC);

	renderEntities(entities, sizeof(entities) / sizeof(entities[0]));

	for (int i = 0; i < (sizeof(entities)/sizeof(entities[0])); i++)
	{
		if (entities[i].entityType == ENEMY)
		{
			entities[i].timer += CP_System_GetDt();
		}
	}

	// player lost
	if ((_player->playerHealth == 0) || (_player->playerMoveRemain == 0)) {
		
		Retry_Menu(_player);
	}

	// stage clear pop-up 
	if (clear == 1) {
		_player->playerMovementspeed = CP_System_GetDt() * (((CP_System_GetWindowWidth() - CP_System_GetWindowHeight()) * 0.1f) * 0.2f * 0.0f);
		CP_Settings_ImageMode(CP_POSITION_CENTER);
		CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
		CP_Settings_Fill(CP_Color_Create(105, 105, 105, 255));
		CP_Graphics_DrawRect(CP_System_GetWindowHeight() * 0.05f, CP_System_GetWindowHeight() * 0.05f, gridw, 0.9f * CP_System_GetWindowHeight());
		CP_Image_Draw(win, CP_System_GetWindowWidth() * 0.45f, CP_System_GetWindowHeight() * 0.55f, 750, 500, 255);
		CP_Settings_Fill(CP_Color_Create(255, 125, 125, 255));
		CP_Graphics_DrawRect(CP_System_GetWindowWidth() * 0.25f, CP_System_GetWindowHeight() * 0.8f, CP_System_GetWindowWidth() * 0.1f, CP_System_GetWindowHeight() * 0.05f);
		CP_Graphics_DrawRect(CP_System_GetWindowWidth() * 0.45f, CP_System_GetWindowHeight() * 0.8f, CP_System_GetWindowWidth() * 0.1f, CP_System_GetWindowHeight() * 0.05f);
		CP_Graphics_DrawRect(CP_System_GetWindowWidth() * 0.65f, CP_System_GetWindowHeight() * 0.8f, CP_System_GetWindowWidth() * 0.1f, CP_System_GetWindowHeight() * 0.05f);
		CP_Settings_Fill(CP_Color_Create(0, 0, 0, 255));
		CP_Font_DrawText("Main Menu", CP_System_GetWindowWidth() * 0.3f, CP_System_GetWindowHeight() * 0.825f);
		if (CP_Input_MouseClicked() && IsAreaClicked(CP_System_GetWindowWidth() * 0.25f, CP_System_GetWindowHeight() * 0.8f, CP_System_GetWindowWidth() * 0.1f, CP_System_GetWindowHeight() * 0.05f, CP_Input_GetMouseX(), CP_Input_GetMouseY())) {
			CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
		}
		CP_Font_DrawText("Retry", CP_System_GetWindowWidth() * 0.5f, CP_System_GetWindowHeight() * 0.825f);
		if (CP_Input_MouseClicked() && IsAreaClicked(CP_System_GetWindowWidth() * 0.45f, CP_System_GetWindowHeight() * 0.8f, CP_System_GetWindowWidth() * 0.1f, CP_System_GetWindowHeight() * 0.05f, CP_Input_GetMouseX(), CP_Input_GetMouseY())) {

			CP_Engine_SetNextGameState(load_init, load_update, load_exit);
		}
		CP_Font_DrawText("EXIT GAME", CP_System_GetWindowWidth() * 0.7f, CP_System_GetWindowHeight() * 0.825f);
		if (CP_Input_MouseClicked() && IsAreaClicked(CP_System_GetWindowWidth() * 0.65f, CP_System_GetWindowHeight() * 0.8f, CP_System_GetWindowWidth() * 0.1f, CP_System_GetWindowHeight() * 0.05f, CP_Input_GetMouseX(), CP_Input_GetMouseY())) 
		{
			CP_Engine_Terminate();
		}
		switch (score) {
		case 1:
			CP_Settings_ImageMode(CP_POSITION_CORNER);
			CP_Image_Draw(star1, 0.25f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			CP_Image_Draw(star0, 0.45f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			CP_Image_Draw(star0, 0.65f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			break;
		case 2:
			CP_Settings_ImageMode(CP_POSITION_CORNER);
			CP_Image_Draw(star1, 0.25f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			CP_Image_Draw(star1, 0.45f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			CP_Image_Draw(star0, 0.65f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			break;
		case 3:
			CP_Settings_ImageMode(CP_POSITION_CORNER);
			CP_Image_Draw(star1, 0.25f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			CP_Image_Draw(star1, 0.45f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			CP_Image_Draw(star1, 0.65f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			break;
		default:
			CP_Settings_ImageMode(CP_POSITION_CORNER);
			CP_Image_Draw(star0, 0.25f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			CP_Image_Draw(star0, 0.45f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			CP_Image_Draw(star0, 0.65f * gridw, CP_System_GetWindowHeight() * 0.15f, 0.1f * gridw, 0.1f * gridw, 255);
			break;
		}
	}


}

void gameicons() {
	// back to main menu button
	CP_Settings_Fill(CP_Color_Create(0, 220, 0, 255));
	CP_Settings_TextSize(30);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Graphics_DrawRect(CP_System_GetWindowHeight() * 0.05f, CP_System_GetWindowHeight() * 0.05f, 0.1f * gridw, 0.1f * gridh);
	CP_Settings_Fill(CP_Color_Create(0, 0, 0, 255));
	CP_Font_DrawText("Main Menu", CP_System_GetWindowHeight() * 0.135f, CP_System_GetWindowHeight() * 0.085f);
	if (CP_Input_MouseClicked() && IsAreaClicked(CP_System_GetWindowHeight() * 0.05f, CP_System_GetWindowHeight() * 0.05f, 0.1f * gridw, 0.1f * gridh, CP_Input_GetMouseX(), CP_Input_GetMouseY())) {
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
	}

	// ui icons
	CP_Settings_TextSize(70);
	CP_Image_Draw(life, 0.2f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);

	// move count
	CP_Image_Draw(moves, 0.5f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
	char movebuffer[50] = { 0 };
	sprintf_s(movebuffer, 50, "%i", _player->playerMoveRemain);
	CP_Font_DrawText(movebuffer, 0.575f * CP_System_GetWindowWidth(), 0.1f * CP_System_GetWindowHeight());

	// attack count
	CP_Image_Draw(attacks, 0.65f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
	char attackbuffer[50] = { 0 };
	sprintf_s(attackbuffer, 50, "%i", _player->playerAmmo);
	CP_Font_DrawText(attackbuffer, 0.725f * CP_System_GetWindowWidth(), 0.1f * CP_System_GetWindowHeight());

	// enemy count
	CP_Image_Draw(enemies, 0.8f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
	char enemybuffer[50] = { 0 };
	sprintf_s(enemybuffer, 50, "%i", _player->EnemyTOKill);
	CP_Font_DrawText(enemybuffer, 0.875f * CP_System_GetWindowWidth(), 0.1f * CP_System_GetWindowHeight());
}

void health() {
	// hp 
	switch (_player->playerHealth) {
	case 0:
		CP_Image_Draw(hp0, 0.25f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp0, 0.3f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp0, 0.35f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		break;
	case 1:
		CP_Image_Draw(hp1, 0.25f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp0, 0.3f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp0, 0.35f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		break;
	case 2:
		CP_Image_Draw(hp2, 0.25f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp0, 0.3f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp0, 0.35f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		break;
	case 3:
		CP_Image_Draw(hp2, 0.25f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp1, 0.3f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp0, 0.35f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		break;
	case 4:
		CP_Image_Draw(hp2, 0.25f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp2, 0.3f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp0, 0.35f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		break;
	case 5:
		CP_Image_Draw(hp2, 0.25f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp2, 0.3f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp1, 0.35f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		break;
	case 6:
		CP_Image_Draw(hp2, 0.25f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp2, 0.3f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		CP_Image_Draw(hp2, 0.35f * CP_System_GetWindowWidth(), 0.05f * CP_System_GetWindowHeight(), CP_System_GetWindowWidth() * 0.05f, CP_System_GetWindowWidth() * 0.05f, 255);
		break;
	default:
		if (_player->playerHealth > 6) {
			_player->playerHealth = 6;
		}
		if (_player->playerHealth < 0) {
			_player->playerHealth = 0;
		}
	}
}

void Game_Exit(void)
{
	CP_Image_Free(&life);
	CP_Image_Free(&attacks);
	CP_Image_Free(&moves);
	CP_Image_Free(&enemies);
	CP_Image_Free(&hp0);
	CP_Image_Free(&hp1);
	CP_Image_Free(&hp2);
	CP_Image_Free(&dmgarea);
	CP_Image_Free(&clearpt);
	CP_Image_Free(&star0);
	CP_Image_Free(&star1);
	CP_Image_Free(&win);

	free(_playerBullet);
	free(_enemyBullet);
}