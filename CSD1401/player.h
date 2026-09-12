/***********************************************************
 file:	player.h
 author:	Terril					t.weiliang@digipen.edu

 brief:	declaration of functions and struct used in player.c

 Copyright © 2024 DigiPen, All rights reserved.
***********************************************************/

#include "game.h"
#pragma once

// Player struct declaration
typedef struct
{
	CP_Image playerSprite;
	int playerHealth, playerAmmo, playerMoveRemain, playerKills, EnemyTOKill;
	float playerMovementspeed, playerPosX, playerPosY, playerScale, playerRotation,
		player_HitboxPosX, player_HitboxPosY, player_HitboxScale, player_HitboxMovement;
	BOOL isAlive;
}PlayerParameter;

void Player_Init(PlayerParameter* player);
void Player_Action_Handler(PlayerParameter* _player, Terrain terrain[], int terrainCount);
