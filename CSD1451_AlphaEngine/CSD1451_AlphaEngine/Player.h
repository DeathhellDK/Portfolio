/* Start Header ************************************************************************/
/*!
\file		Player.h
\author		Tan Wei Liang Terril
\date		March, 20, 2025
\brief		File contain the struct for player and function to set the player spawn, 
			draw the player and draw the aim icon

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef P_H
#define P_H

#include "Main.h"

/// <summary>
/// Define a struct for the player
/// </summary>
typedef struct Player
{
	int exp, max_exp, default_speed, damage_output, score, bulletPattern;
	float movement_speed, firing_cd, hp, max_hp;
	int const playerMaxEXPInc = 15;
	shape player_shape;
	bool lvUp;
	bool scoreSaved;

	AEMtx33 player_transform, player_scale, player_rotation, player_translate;
}Player;

/// <summary>
/// Set the spawn for the player
/// </summary>
/// <param name="player"> Fet the player struct data object </param>
/// <param name="player_currentHp"> player current health </param>
/// <param name="player_maxHp"> player maximum health </param>
/// <param name="damage_output"> player damage </param>
/// <param name="player_firecd">player attack interval </param>
/// <param name="player_currentexp"> player current experience point </param>
/// <param name="player_maxexp"> player maximum experience point </param>
/// <param name="player_defaultSpeed"> player default speed </param>
/// <returns></returns>
Player *player_setspawn(Player* player, float player_currentHp, float player_maxHp, int damage_output,
	float player_firecd, int player_currentexp, int player_maxexp, int player_defaultSpeed);

/// <summary>
/// Draw the player object
/// </summary>
/// <param name="player"> Get the player struct data object </param>
/// <param name="mesh"> Shape of the mesh to represent the player </param>
void draw_player(Player *player, AEGfxVertexList* mesh);

/// <summary>
/// Draw the aim icon object
/// </summary>
/// <param name="player"> Get the player struct data object </param>
/// <param name="mesh"> Shape of the mesh to represent the aim icon</param>
void draw_player_aimIcon(Player *player, AEGfxVertexList* mesh);

#endif // DEBUG
