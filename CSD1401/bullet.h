/***********************************************************
 file:	bullet.h
 author:	Terril					t.weiliang@digipen.edu

 brief:	definition of functions and struct that will be used by the player and enemy.
 The function of will be used in bullet.c

 Copyright © 2024 DigiPen, All rights reserved.
***********************************************************/
#pragma once
#include "game.h"
#include "player.h"

/* 
* Struct for the entity.As the entity is using extern and bullet need to get the entity
* parameter details, the entity struct will be declare in bullet.h
*/
typedef struct {
	float posX;
	float posY;
	float width;
	float height;

	int entityType;
	int renderType;
	CP_Color color;
	//in case the entity is an image 
	CP_Image image;

	float timer;
} entity;

//Declare the id of the bullet
typedef enum {
	NOTHING,
	MC,
	NPC
}BULLETID;

// Decalaration of the bullet parameter struct
typedef struct
{
	CP_Image bulletSprite;
	int bulletDamage, bulletTransparence, bulletRemain;
	float bulletPosX, bulletPosY, bulletRotation, bulletSpeed, bulletMovementspeed, bulletSpawnTime,
		bullet_HitBoxX, bullet_HitBoxW, bullet_HitBoxY, bullet_HitBoxH, bullet_HitboxRotate;
	BULLETID ID;
	BOOL isTriggered; BOOL isCD; BOOL isRotate;

}BulletParameter;

void bullet_Init(BulletParameter* bullet_parameter, int max_ammo, BULLETID ID);
void bullet_Spawn(BulletParameter* bullet_parameter, float characterPosX, float characterPosY, float characterRotation, BULLETID ID);
void bullet_MovementHandler(BulletParameter* bullet_parameter, PlayerParameter *player,Terrain terrain[], entity entity[], int terraincount, int max_ammo, BULLETID ID);