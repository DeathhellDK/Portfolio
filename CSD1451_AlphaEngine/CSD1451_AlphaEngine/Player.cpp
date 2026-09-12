/* Start Header ************************************************************************/
/*!
\file		PLayer.cpp
\author		Tan Wei Liang Terril
\date		March, 20, 2025
\brief		File contain the function to set the player spawn,
			draw the player and draw the aim icon
			- Set the spawn for the player
			- Draw the player object
			- Draw the aim icon object

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "Player.h"

Player* player_setspawn(Player* player, float player_currentHp, float player_maxHp, int damage_output,
	float player_firecd, int player_currentexp, int player_maxexp, int player_defaultSpeed)
{
	player->player_shape.pos_x = 0; // player spawn x position
	player->player_shape.pos_y = 0; // player spawn y position
	player->player_shape.scale = 50.0f; // player size 
	player->player_shape.Rotation = 0.0f; // player rotation
	player->default_speed = player_defaultSpeed; // player default speed
	player->hp = player_currentHp; // player hp 
	player->max_hp = player_maxHp; // player total max hp
	player->damage_output = damage_output; // player damage output
	player->exp = player_currentexp; // player exp
	player->max_exp = player_maxexp; // player total max exp to lv up
	player->firing_cd = player_firecd; // how fast can the player fire
	player->movement_speed = player->default_speed * static_cast<float>(AEFrameRateControllerGetFrameTime()); // How fast the player moving
	player->lvUp = false; // check if player has level up
	player->score = 0;
	player->scoreSaved = false;
	player->bulletPattern = 0; // Initialize player with single bullet pattern

	return player; // Return player struct paramater
}

/// <summary>
/// Draw the player object
/// </summary>
/// <param name="player"> Get the player struct data </param>
/// <param name="mesh"> The shape of the mesh object </param>
void draw_player(Player *player, AEGfxVertexList* mesh)
{
		AEGfxSetColorToAdd(0.0f, 0.1f, 0.5f, 1.0f); // Add the color
		AEMtx33Identity(&player->player_transform); // player identity matrix
		AEMtx33Scale(&player->player_scale, player->player_shape.scale, player->player_shape.scale); // player scale
		AEMtx33Rot(&player->player_rotation, player->player_shape.Rotation); // player rotation
		AEMtx33Trans(&player->player_translate, player->player_shape.pos_x, player->player_shape.pos_y); // player translation
		AEMtx33Concat(&player->player_transform, &player->player_rotation, &player->player_scale); // transform, rotate and scale
		AEMtx33Concat(&player->player_transform, &player->player_translate, &player->player_transform); // transform, translate and transform
		AEGfxSetTransform(player->player_transform.m); // set the final player transformation
		AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES); // Draw the player object
}

/// <summary>
/// Draw the player aim icon
/// </summary>
/// <param name="player"> Get the player struct data </param>
/// <param name="mesh"> The shape of the mesh object </param>
void draw_player_aimIcon(Player* player, AEGfxVertexList* mesh) {

	AEMtx33 Aim_transform, Aim_scale, Aim_angle, Aim_translate; // create tempo varaible for transformation, scaling, rotation and translation
	AEGfxSetColorToAdd(0.6f, 0.9f, 0.9f, 1.0f); // add the color
	AEMtx33Identity(&Aim_transform); // aim icon identity matrix
	AEMtx33Scale(&Aim_scale, player->player_shape.scale /2 , player->player_shape.scale / 2); // aim icon scale base off half of player scale
	AEMtx33Rot(&Aim_angle, player->player_shape.Rotation); // aim icon rotation base off player rotation
	// aim icon translation base off player rotation and offset from player position
	AEMtx33Trans(&Aim_translate, player->player_shape.pos_x + cosf(player->player_shape.Rotation + 1.5708f) * 50.0f,
		player->player_shape.pos_y + sinf(player->player_shape.Rotation + 1.5708f) * 50.0f); 
	AEMtx33Concat(&Aim_transform, &Aim_scale, &Aim_angle); // transform, scale and rotate
	AEMtx33Concat(&Aim_transform, &Aim_translate, &Aim_transform); // transform, translate and transform
	AEGfxSetTransform(Aim_transform.m); // set the final aim icon transformation
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES); // Draw the aim icon object
}

