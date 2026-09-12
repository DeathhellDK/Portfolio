/* Start Header ************************************************************************/
/*!
\file		SkillMenu.h
\author		Tan Wei Liang Terril
\date		March, 20, 2025
\brief		File contain the function to set the skill menu
			- Initialise the skill menu
			- Display the skill menu
			- Player selection option
			- Randomly generate the skill and weapon for the player
			- Player selection option
			- Option button initalise

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef SKILL_H
#define SKILL_H

#pragma once

#include "Player.h"
#include "weapon.h" // To retreieve the weapon enum class and weapon parameter struct

/// <summary>
/// A enum class to store the possible skill that the player can choose from,
/// these skill will affect the player parameter base on what they chose,
/// ranging from healing to increase their damage
/// </summary>
enum class SKILL_LIST { 
	HEALTH_RESTORE, 
	MAX_HP_INCREASE, 
	MOVEMENT_SPEED_INCREASE, 
	DAMAGE_OUTPUT_INCREASE,
	FIRING_CD_DECREASE,
	//MULTIPLE_BULLETS
};

/// <summary>
/// A struct class to store the skill value which will affect the player by.
/// </summary>
struct SKILL_PARAMETER { 
	int skilleffectvalue; 
	float cooldown_reduction;

	// Constructor to match the map initialization
	SKILL_PARAMETER(int effect, float cooldown = 10.0f)
		: skilleffectvalue(effect), cooldown_reduction(cooldown) {}
};


/// <summary>
/// Option button default initalise
/// </summary>
/// <param name="option_btn"> Option Button shape struct </param>
/// <param name="posX"> default x position where you want it to be </param>
/// <param name="posY"> default y position where you want it to be </param>
/// <returns></returns>
shape* OptionBtn_Init(shape* option_btn, float posX, float posY);

/// <summary>
/// Randomly generate a list of skill and weapon for the player,
/// which will store in a skillWeapon vector string and will be add into
/// the vector displayskillWeapon which will be use to display the skill / weapon
/// </summary>
/// <param name="vector_data"> retreive the weapon skill item data from the vector array and store the skill and weapon inside </param>
/// <param name="skillWeapon_txt"> vector array to store the skill and weapon text </param>
/// <param name="displayskillWeapon_txt"> vector array that is a const character to store the string of text from the skillWeapon_txt vector </param>
void RandomSkillWeaponGenerator(std::vector<Weapon_Skill_Item>& vector_data, std::vector<std::string>& skillWeapon_txt, 
	std::vector<const char*>& displayskillWeapon_txt);

/// <summary>
/// Display the top 3 skill and weapon in the displayskillWeapon_txt vector array 
/// for the player to choose from
/// </summary>
/// <param name="player"> Retrieve the player data </param>
/// <param name="Mesh"> Retrieve the mesh data </param>
/// <param name="font_id"> Retrieve the font id data</param>
/// <param name="displayskillWeapon_txt"> Retrieve the elements in the vector</param>
void skillweaponmenu_Display(Player* player, AEGfxVertexList* Mesh, s8 font_id, std::vector<const char*>& displayskillWeapon_txt);

/// <summary>
/// Skill and weapon that the player can choose and base on
/// the item that they choose they will gain the following:
/// If it is a skill:
///		- Player can gain the effect such as healing and movement speed;
/// If it is a weapon:
///		- Player can unlock that particlar weapon and use it
/// </summary>
/// <param name="vector_data"> retreive the weapon skill item data from the vector array and store the skill and weapon inside </param>
/// <param name="player"> retrieve the player parameter </param>
/// <param name="player_tempo_fireRate_Tracker"> retrieve the firerate_cd tracker for the player </param>
/// <param name="id"> Retreive the id of which button is been press</param>
void PlayerSelectionOption(std::vector<Weapon_Skill_Item>& vector_data, Player *player, float &player_tempo_fireRate_Tracker, int id);
shape* OptionBtn_Init(shape* option_btn, float posX, float posY);


#endif // !SKILL_H
