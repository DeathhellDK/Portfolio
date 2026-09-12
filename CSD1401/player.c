/***********************************************************
 file:	player.c
 author:	Terril					t.weiliang@digipen.edu

 brief: player.c contain a list of function where it will handle how the player behavior
 1. Player_Health_Handler will reduce the player health base on the damage that the player have taken from the game object
 2. Player_Action_Handler will use to handle where the player will face and move when the user press the following key: W,A,S,D. 
 Futhermore, when the player were to walk into the wall, the handler will prevent the player to go beyond it. Handler will also preven the 
 player from taking any action upon death.

 Copyright © 2024 DigiPen, All rights reserved.
***********************************************************/

#include "cprocessing.h"
#include "player.h"
#include "bullet.h"
#include "utils.h"
#include "math.h"
#include <stdio.h>
#include <stdbool.h>

/*
* Define the preprocessor macro for the player so that player can use
*/
#define PLAYER_MOVEMENT_SCALE ((CP_System_GetWindowWidth() - CP_System_GetWindowHeight())*0.1f)*0.2f
#define PLAYER_POSITION_X_SCALE CP_System_GetWindowHeight() * 0.05f + 0.08f * gridW
#define PLAYER_POSITION_Y_SCALE CP_System_GetWindowHeight() * 0.2f + 0.6f * gridH 
#define PLAYER_HEIGHT_SCALE (CP_System_GetWindowWidth() - CP_System_GetWindowHeight()) * 0.1f
#define PLAYER_DEFAULT_ROTATION 0
#define PLAYER_TRANSPARENCE 255
#define PLAYER_MOVE_USE 1

#define PLAYER_HITBOX_POSX CP_System_GetWindowHeight() * 0.05f + 0.101f * gridW
#define PLAYER_HITBOX_POSY CP_System_GetWindowHeight() * 0.2f + 0.852f * gridH
#define PLAYER_HITBOX_OFFSETX 34.0f
#define PLAYER_HITBOX_OFFSETY 35.0f

// Function to handle when player take damage from enemy/ terrain 
void Player_Health_Handler(PlayerParameter* player, int damageRecieved)
{
	player->playerHealth -= damageRecieved;
}

/*
* Function to create the parameter of the player:
* Player position X
* Player position Y
* player scale
* player ammo
* player health
* player attack
*/
void Player_Init(PlayerParameter* player)
{
	// Player default parameter value 
	player->playerPosX = PLAYER_POSITION_X_SCALE;
	player->playerPosY = PLAYER_POSITION_Y_SCALE;
	player->playerScale = 0.082f * gridW + 0.1f * gridH;
	player->playerRotation = PLAYER_DEFAULT_ROTATION;
	player->isAlive = TRUE;
	player->EnemyTOKill = 0;
	player->playerHealth = 6;
	player->playerAmmo = CP_Random_RangeInt(4, 12);
	player->playerKills = 0;
	player->playerMoveRemain = CP_Random_RangeInt(135, 150);
	player->playerMovementspeed = CP_System_GetDt() * (PLAYER_MOVEMENT_SCALE * 100.0f);

	// Player default hitbox parameter value
	player->player_HitboxPosX = PLAYER_HITBOX_OFFSETX + PLAYER_POSITION_X_SCALE;
	player->player_HitboxPosY = PLAYER_HITBOX_OFFSETY + PLAYER_POSITION_Y_SCALE;
	player->player_HitboxScale = 0.05f * PLAYER_HITBOX_POSX + 0.05f * PLAYER_HITBOX_POSY;
	player->player_HitboxScale /= 15;
	CP_Graphics_DrawCircle(player->player_HitboxPosX, player->player_HitboxPosY, player->player_HitboxScale);
	player->playerSprite = CP_Image_Load("Assets/Player.png");
	CP_Image_DrawAdvanced(player->playerSprite, player->playerPosX, player->playerPosY, player->playerScale, PLAYER_HEIGHT_SCALE, PLAYER_TRANSPARENCE, player->playerRotation);
}

/*
* Function handle the following behaviour:
* - Player Movement
* - Player Remaing Ammo
* - Player Remaining Move
* - Player Life
* - When the player Ammo/ Move/ Life drop to 0, Player will not be able to move
*/
void Player_Action_Handler(PlayerParameter* player, Terrain terrain[], int terrainCount)
{
	float x_offset = 0;
	float y_offset = 0;
	float rotation_offset = player->playerRotation;
	// If the player is alive and there is move remaing, the player will be able to move. If not, the player will be remove from the screen
	if (player->playerMoveRemain > 0 && player->isAlive == TRUE)
	{
		// If the player have life remain, the player is able to move in the following direction: Up, down, left, right
		if (player->playerHealth != 0)
		{
			if (CP_Input_KeyTriggered(KEY_W))
			{
				rotation_offset = PLAYER_DEFAULT_ROTATION;
				y_offset -= player->playerMovementspeed;
			}
			if (CP_Input_KeyTriggered(KEY_S))
			{
				rotation_offset = PLAYER_DEFAULT_ROTATION + 180;
				y_offset += player->playerMovementspeed;
			}
			if (CP_Input_KeyTriggered(KEY_A))
			{
				rotation_offset = -(PLAYER_DEFAULT_ROTATION + 90);
				x_offset -= player->playerMovementspeed;
			}
			if (CP_Input_KeyTriggered(KEY_D))
			{
				rotation_offset = PLAYER_DEFAULT_ROTATION + 90;
				x_offset += player->playerMovementspeed;
			}

			//Player will check for terrain collision. If the player were to walk towards the wall, the player will be unable to proceed further
			for (size_t i = 0; i < terrainCount; i++)
			{
				Terrain player_rect;

				player_rect.posX = player->player_HitboxPosX - (player->player_HitboxScale / 2) + x_offset;
				player_rect.posY = player->player_HitboxPosY - (player->player_HitboxScale / 2) + y_offset;
				player_rect.height = player->player_HitboxScale;
				player_rect.width = player->player_HitboxScale;
				if (isTerrainRectangleCollision(terrain[i], player_rect)) {
					//printf("\nPlayer Collide into the terrain\n");
					y_offset = x_offset = 0;
				}
			}

			// Player will be unable to move if there collided into the wall
			player->player_HitboxPosX += x_offset;
			player->playerPosX += x_offset;
			player->player_HitboxPosY += y_offset;
			player->playerPosY += y_offset;
			player->playerRotation = rotation_offset;

			// If there did not collide into the wall, the player will use up 1 step.
			if (y_offset != 0 || x_offset != 0)
			{
				player->playerMoveRemain -= 1;
			}
			CP_Graphics_DrawCircle(player->player_HitboxPosX, player->player_HitboxPosY, player->player_HitboxScale);
			CP_Image_DrawAdvanced(player->playerSprite, player->playerPosX, player->playerPosY, player->playerScale, PLAYER_HEIGHT_SCALE, PLAYER_TRANSPARENCE, player->playerRotation);
		}
		else
		{
			// If the player health drop to 0
			player->isAlive = FALSE;

			// Prevent from the player to move
			player->playerMovementspeed = CP_System_GetDt() * 0;

			// Free the player sprite
			CP_Image_Free(&player->playerSprite);
		}
	}
	else
	{
		// if the player ran out of move
		player->isAlive = FALSE;

		// Prevent from the player to move
		player->playerMovementspeed = CP_System_GetDt() * 0;

		// Free the player sprite
		CP_Image_Free(&player->playerSprite);
	}
}

