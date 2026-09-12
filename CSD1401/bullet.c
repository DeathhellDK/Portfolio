/***********************************************************
 file:	bullet.c
 author:	Terril					t.weiliang@digipen.edu

 brief:	bullet.c contain a list of function that will be retrieve from bullet.h which will be use to handle how the bullet
 will behave. The following function contains the breakdown of what is suppose to do:
 bullet_Init - Inital the parameter for the bullet base on the player bullet. For each of the (dynamic) bullet, it will contains its own unique parameter.

 bullet_Spawn - Set the current bullet that the player/enemy is going to fire and spawn the bullet base on the player/enemy current rotation and position.

 bullet_MovementHandler - Handle how the bullet will move base on the player/enemy rotation. In this function, the bullet will also check on the collision
 if it touches the wall or the player/enemy depending on its ID. The bullet will also have a timer countdown to prevent the bullet from traviling across the map.
 When the timer hits 0, the bullet will be disspear from the game world.

 Copyright © 2024 DigiPen, All rights reserved.
***********************************************************/

#include "cprocessing.h"
#include "utils.h"
#include "bullet.h"
#include "player.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"
#include "math.h"
#include "game.h"
#include "enemy.h"

#define DEFAULT 0.0f
#define BULLET_WIDTH (CP_System_GetWindowWidth() - CP_System_GetWindowHeight()) * 0.1f 
#define BULLET_HEIGHT (CP_System_GetWindowWidth() - CP_System_GetWindowHeight()) * 0.1f
#define BULLET_MOVEMENT_SCALE ((CP_System_GetWindowWidth() - CP_System_GetWindowHeight())*0.1f)*0.1f
#define BULLET_DAMAGE 2
#define BULLET_TIMER 1.0f
#define BULLET_OFFSETX 10.0f
#define BULLET_OFFSETW 0.6f
#define BULLET_OFFSETY 36.0f
#define BULLET_OFFSETH 0.12f
#define BULLET_ROTAOFFSET 18.0f
#define PI 3.14f

/*
* Initalise a parameter for bullet.
* These parameter will be reference back if there are any changes need to be made to its parameter
* This function acts as a storage.
*/
void bullet_Init(BulletParameter* bullet_ptr, int max_ammo, BULLETID ID)
{
	// Base on the ammo of the user, the bullet paramter will create a set of default value, which at as a storage so that it can be use later
	for (int i = 0; i < max_ammo; i++)
	{
		bullet_ptr[i].bulletPosX = DEFAULT;
		bullet_ptr[i].bulletPosY = DEFAULT;
		bullet_ptr[i].bulletRotation = DEFAULT;
		bullet_ptr[i].bulletMovementspeed = DEFAULT;
		bullet_ptr[i].bulletDamage = BULLET_DAMAGE;
		bullet_ptr[i].bulletTransparence = 255;
		bullet_ptr[i].bulletSpawnTime = BULLET_TIMER;
		bullet_ptr[i].bulletSprite = NULL;
		bullet_ptr[i].ID = ID;
	}
}

/*
* Function will handle the bullet spawning location base on the character area
*/
void bullet_Spawn(BulletParameter* bullet_parameter, float characterPosX, float characterPosY, float characterRotation, BULLETID ID)
{
	// Set that particular paramater of the location where the bullet can spawn
	bullet_parameter->bulletPosX = characterPosX;
	bullet_parameter->bulletPosY = characterPosY;
	bullet_parameter->bulletRotation = characterRotation;
	bullet_parameter->ID = ID;
	bullet_parameter->isRotate = TRUE;
	bullet_parameter->isTriggered = TRUE;
	bullet_parameter->isCD = TRUE;
	bullet_parameter->bulletMovementspeed = CP_System_GetDt() * (BULLET_MOVEMENT_SCALE * 50.0f);
	bullet_parameter->bulletSprite = CP_Image_Load("Assets/Bullet.png");
}

/* Funtion will handel how the bullet will move and as well as it will check for any collision have occur
* base on the bullet ID. 
* MC - Character
* NPC - Enemy
* This function handle the bullet dynamically so that both player and enemy can use it without having to make 2 spereate handler
*/
void bullet_MovementHandler(BulletParameter* bullet_parameter, PlayerParameter* player,Terrain terrain[], entity entities[], int terraincount, int max_ammo, BULLETID ID)
{
	int x_offset = 0;
	int y_offset = 0;
	int currentBulletNo = 0;
	BulletParameter* curr_bullet;

	/* For each of the bullet been fire by the ID user, it will move and check if it collide into anything
	* If there is not collision occur,the bullet will just despawn
	*/
	for (int i = 0; i < max_ammo; i++)
	{
		// set the current bullet index
		curr_bullet = &bullet_parameter[i];
		float bulletAngleRotation = curr_bullet->bulletRotation * PI / 180.0f;

		// Check for the current bullet index that suppose to be triggered.
		if (!curr_bullet->isTriggered && !curr_bullet->isCD)
		{
			continue;
		}
		
		// rotation base on where the enemy is facing and move base on that axis
		if (ID == NPC)
		{
			if (curr_bullet->bulletRotation == 0) {
				curr_bullet->bulletPosY -= curr_bullet->bulletMovementspeed;
			}
			if (curr_bullet->bulletRotation == 90) {
				curr_bullet->bulletPosX += curr_bullet->bulletMovementspeed;
			}
			if (curr_bullet->bulletRotation == 180) {
				curr_bullet->bulletPosY += curr_bullet->bulletMovementspeed;
			}
			if (curr_bullet->bulletRotation == 270) {
				curr_bullet->bulletPosX -= curr_bullet->bulletMovementspeed;
			}
		}

		// rotation base on where the player is facing and move base on that axis
		if (ID == MC)
		{
			if (bulletAngleRotation >= 0 && bulletAngleRotation < PI / 2)
			{
				curr_bullet->bulletPosY -= curr_bullet->bulletMovementspeed;
			}
			else if (bulletAngleRotation >= PI / 2 && bulletAngleRotation < PI)
			{
				curr_bullet->bulletPosX += curr_bullet->bulletMovementspeed;
			}
			else if (bulletAngleRotation >= PI && bulletAngleRotation < 3 * PI / 2)
			{
				curr_bullet->bulletPosY += curr_bullet->bulletMovementspeed;
			}
			else
			{
				curr_bullet->bulletPosX -= curr_bullet->bulletMovementspeed;
			}
		}

		// loop to check for the following collision: Terrain, Player and Enemy.
		for (int i = 0; i < terraincount; i++)
		{
			if (!curr_bullet->isCD) {
				break; 
			}

			Terrain bullet_rect;
			entity enemyHibox = entities[i];
			Terrain player_rect;

			bullet_rect.posX = curr_bullet->bulletPosX + 10.0f;
			bullet_rect.posY = curr_bullet->bulletPosY + 10.0f;
			bullet_rect.height = BULLET_HEIGHT-20.0f;
			bullet_rect.width = BULLET_WIDTH - 20.0f;

			player_rect.posX = player->player_HitboxPosX - (player->player_HitboxScale / 2) + x_offset;
			player_rect.posY = player->player_HitboxPosY - (player->player_HitboxScale / 2) + y_offset;
			player_rect.height = player->player_HitboxScale;
			player_rect.width = player->player_HitboxScale;

			enemyHibox.posX = entities[i].posX - entities[i].width / 2;
			enemyHibox.posY = entities[i].posY - entities[i].height / 2;
			enemyHibox.width = entities[i].width;
			enemyHibox.height = entities[i].height;

			// if the bullet collide with the player, player health will drop by 1
			if (ID == NPC)
			{
				if (isPlayerRectangleCollision2(player_rect, bullet_rect))
				{
					curr_bullet->isCD = FALSE;
					curr_bullet->bulletSpawnTime = 0;
					curr_bullet->bulletTransparence = 0;
					player->playerHealth -= 1;
					break;
				}
			}

			// if the bullet collide with the enemy, enemy dissaper and enemy count will drop by 1 and player will gain 1 kill
			if (ID == MC)
			{
				if (isEnemyRectangleCollision2(enemyHibox, bullet_rect))
				{
					curr_bullet->isCD = FALSE;
					curr_bullet->bulletSpawnTime = 0;
					curr_bullet->bulletTransparence = 0;
					removeEnemy(i);
					player->playerKills += 1;
					player->EnemyTOKill -= 1;
					break;
				}
			}

			// If the bullet hit the wall, the bullet will dissapear.
			if (isTerrainRectangleCollision(terrain[i], bullet_rect)) {
				curr_bullet->isCD = FALSE;
				curr_bullet->bulletSpawnTime = 0;
				curr_bullet->bulletTransparence = 0;
				break;
			}	
		}
		
		CP_Image_DrawAdvanced(curr_bullet->bulletSprite, curr_bullet->bulletPosX, curr_bullet->bulletPosY, BULLET_WIDTH, BULLET_HEIGHT, curr_bullet->bulletTransparence, curr_bullet->bulletRotation);

		// Once the bullet timer reach zero, the function will set the current bullet to be transparence so that it seem like dissapear
		if (curr_bullet->isCD == TRUE)
		{
			if (curr_bullet->bulletSpawnTime > 0)
			{
				curr_bullet->bulletSpawnTime -= CP_System_GetDt();
			}
			else
			{
				// Handle the bullet once the timer hits 0
				curr_bullet->isCD = FALSE;
				curr_bullet->bulletSpawnTime = 0;
				curr_bullet->bulletTransparence = 0;
			}
		}
	}
}