/* Start Header ************************************************************************/
/*!
\file		SkillMenu.cpp
\author		Tan Wei Liang Terril(99%), 
			Tingkai(1% - Call the UnlockWeapon())

\date		March, 20, 2025
\brief		File contains the function to set the skill menu
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
#include <algorithm>
#include <map>
#include <random>
#include <ctime>
#include "SkillMenu.h"

/// <summary>
/// skillmap holds the relationships between the different skill and their respective parameter
/// </summary>
std::map<SKILL_LIST, SKILL_PARAMETER> skillmap = { {SKILL_LIST::HEALTH_RESTORE, {20}}, {SKILL_LIST::MAX_HP_INCREASE, {10}},
	{SKILL_LIST::MOVEMENT_SPEED_INCREASE, {15}}, {SKILL_LIST::DAMAGE_OUTPUT_INCREASE, {1}} , {SKILL_LIST::FIRING_CD_DECREASE, {2}} };

/// <summary>
/// weaponmap holds the realationships between the different weapon and their respective parameter
/// </summary>
std::map<WEAPON_LIST, WEAPON_PARAMETER> weaponmap = { {WEAPON_LIST::PISTOL, {2,10.0f}}, {WEAPON_LIST::RIFLE, {1, 2.0f}}, {WEAPON_LIST::SHOTGUN, {4, 15.0f}} };


/// <summary>
/// Option button default initalise
/// </summary>
/// <param name="option_btn"> Option Button shape struct </param>
/// <param name="posX"> default x position where you want it to be </param>
/// <param name="posY"> default y position where you want it to be </param>
/// <returns> Return the shape data </returns>
shape* OptionBtn_Init(shape* option_btn, float posX, float posY) {
	option_btn->pos_x = posX; // initialise the button position x
	option_btn->pos_y = posY; // initialise the button positon y
	option_btn->scale = 0.0f; // initialise the button size to default value
	option_btn->Rotation = 0; // initialise the button rotation to default value

	return option_btn;
}

/// <summary>
/// To covert each of the skill key into a string of skill name
/// </summary>
/// <param name="type"> a set of skill const in the SKILL_LIST enum class </param>
/// <returns> Return the string name base on the enum key in "type" </returns>
static std::string skillTypeTostring(SKILL_LIST type) {
	std::string skill_details_display; // To store the string

	// retrun the name of the skill base on the key
	switch (type) {
	case SKILL_LIST::HEALTH_RESTORE: return skill_details_display = "HP RECOVER";
	case SKILL_LIST::MAX_HP_INCREASE: return skill_details_display =  "MAX HP INCREASE";
	case SKILL_LIST::MOVEMENT_SPEED_INCREASE: return skill_details_display =  "MOVEMENT SPD INCREASE";
	case SKILL_LIST::DAMAGE_OUTPUT_INCREASE: return  skill_details_display = "DMG INCREASE";
	case SKILL_LIST::FIRING_CD_DECREASE: return  skill_details_display = "FIRING_CD_DECREASE";
	}
	return skill_details_display;
}

/// <summary>
/// To covert each of the weapon key into a string of weapon name
/// </summary>
/// <param name="type"> a set of weapon const in the WEAPON_LIST enum class </param>
/// <returns> Return the string name base on the enum key in "type" </returns>
std::string weaponTypeTostring(WEAPON_LIST type) {
	std::string weapon_details_display; // To store the string

	// retrun the name of the skill base on the key
	switch (type) {
	case WEAPON_LIST::PISTOL: return weapon_details_display = "PISTOl";
	case WEAPON_LIST::RIFLE: return weapon_details_display = "RIFLE";
	case WEAPON_LIST::SHOTGUN: return weapon_details_display = "SHOTGUN";
	}
	return weapon_details_display ;
}

/// <summary>
/// Randomly generate a list of skill and weapon for the player,
/// which will store in a skillWeapon vector string and will be add into
/// the vector displayskillWeapon which will be use to display the skill / weapon
/// </summary>
/// <param name="vector_data"> retreive the weapon skill item data from the vector array and store the skill and weapon inside </param>
/// <param name="skillWeapon_txt"> vector array to store the skill and weapon text </param>
/// <param name="displayskillWeapon_txt"> vector array that is a const character to store the string of text from the skillWeapon_txt vector </param>
void RandomSkillWeaponGenerator(std::vector<Weapon_Skill_Item>& vector_data , std::vector<std::string>& skillWeapon_txt, 
	std::vector<const char*> &displayskillWeapon_txt) {
	
	std::random_device rngseed; // generate a random number for the seed
	std::mt19937 rng(rngseed()); // generate a randome number base on the seed

	skillWeapon_txt.clear(); // Remove any exiting skill weapon text in the vector array
	displayskillWeapon_txt.clear(); // Remove any exiting skill weapon text for display in the vector array

	// Check if the weapon and skill vector is empty, and only if the vector is empty then
	// add the skill and weapon map into the vector
	if (vector_data.empty()) {
		// loop through each of the skill map and add each of the skill item into the vector data
		for (auto& skill : skillmap) {
			vector_data.push_back({ true, &skill.second, skillTypeTostring(skill.first) }); // add the skill item into the weapon skill item vector array
		}
		// loop through each of the weapon map and add each of the weapon item into the vector data
		for (auto& weapon : weaponmap) {
			vector_data.push_back({ false, &weapon.second, weaponTypeTostring(weapon.first) }); // add the weapon item into the weapon skill item vector array
		}
	}
	
	// shuffle the weapon skill item in a random order
	std::shuffle(vector_data.begin(), vector_data.end(), rng);
	
	skillWeapon_txt.reserve(3); // reserve at least 3 space in the skill text vector array
	displayskillWeapon_txt.reserve(3); // reserve at least 3 space in the weapon text vector array

	size_t limit = (vector_data.size() < 3) ? vector_data.size() : 3;

	// Loop through at least 3 time (displaying only the top 3 item in the weapon skill vector array)
	for (size_t i = 0; i < limit; ++i)
	{
		std::ostringstream os;

		// check if the item that is currently on the index is a skill or not
		if (vector_data[i].isSkill) {
			// if it is a skill, store the skill parameter into a new parameter struct so that it can print the skill effect
			SKILL_PARAMETER* skill = static_cast<SKILL_PARAMETER*>(vector_data[i].data); 
			os << vector_data[i].nameID << " BY: " << skill->skilleffectvalue; // store into a string steam
		}
		else {
			// if it is a weapon, store the weapon parameter into a new parameter struct so that it can print the weapon parameter data
			WEAPON_PARAMETER* weapon = static_cast<WEAPON_PARAMETER*>(vector_data[i].data);
			os << "UNLOCK: " << vector_data[i].nameID << " Dmg: " << weapon->damage << ", CD: " << weapon->firerate_cd << ")";
		}

		skillWeapon_txt.push_back(os.str()); // add the string steam into the skill weapon txt vector array
		displayskillWeapon_txt.push_back(skillWeapon_txt.back().c_str()); // convert into a constant character and store into the display skill weapon txt vector
	}
}

/// <summary>
/// Display the top 3 skill and weapon in the displayskillWeapon_txt vector array 
/// for the player to choose from
/// </summary>
/// <param name="player"> Retrieve the player data </param>
/// <param name="Mesh"> Retrieve the mesh data </param>
/// <param name="font_id"> Retrieve the font id data</param>
/// <param name="displayskillWeapon_txt"> Retrieve the elements in the vector</param>
void skillweaponmenu_Display(Player* player, AEGfxVertexList* Mesh, s8 font_id, std::vector<const char*>& displayskillWeapon_txt)
{
	#pragma region Skill_Weapon_Display
	// variable that will be use to do tranlate, rotation, scale and transformation for the menu
	AEMtx33 SkillWeaponMenu_transform, SkillWeaponMenu_angle, SkillWeaponMenu_scale, SkillWeaponMenu_translate,
		Option1_transform, Option1_angle, Option1_scale, Option1_translate,
		Option2_transform, Option2_angle, Option2_scale, Option2_translate,
		Option3_transform, Option3_angle, Option3_scale, Option3_translate;

	#pragma region DisplayMenu
	AEGfxSetColorToAdd(1.0f, 0.8f, 0.3f, 1.0f);
	AEMtx33Identity(&SkillWeaponMenu_transform);
	AEMtx33Rot(&SkillWeaponMenu_angle, 0.0f);
	AEMtx33Scale(&SkillWeaponMenu_scale, AEGfxGetWindowWidth() / 2.0f, AEGfxGetWindowHeight() / 1.5f);
	AEMtx33Trans(&SkillWeaponMenu_translate, player->player_shape.pos_x, player->player_shape.pos_y);
	AEMtx33Concat(&SkillWeaponMenu_transform, &SkillWeaponMenu_angle, &SkillWeaponMenu_scale);
	AEMtx33Concat(&SkillWeaponMenu_transform, &SkillWeaponMenu_translate, &SkillWeaponMenu_transform);
	AEGfxSetTransform(SkillWeaponMenu_transform.m);
	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES); 
	#pragma endregion

	#pragma region Option1
	AEGfxSetColorToAdd(0.2f, 0.8f, 1.0f, 1.0f);
	AEMtx33Identity(&Option1_transform);
	AEMtx33Rot(&Option1_angle, 0.0f);
	AEMtx33Scale(&Option1_scale, AEGfxGetWindowWidth() / 2.2f, AEGfxGetWindowHeight() / 10.0f);
	AEMtx33Trans(&Option1_translate, player->player_shape.pos_x, player->player_shape.pos_y + 200.0f);
	AEMtx33Concat(&Option1_transform, &Option1_angle, &Option1_scale);
	AEMtx33Concat(&Option1_transform, &Option1_translate, &Option1_transform);
	AEGfxSetTransform(Option1_transform.m);
	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);

	// Print the 1 postion of element store in the vector array
	AEGfxPrint(font_id, *(displayskillWeapon_txt.begin()), -0.3f, 0.42f, 0.25f, 0.0f, 0.0f, 0.0f, 1.0f);
	#pragma endregion

	#pragma region Option2
	AEMtx33Identity(&Option2_transform);
	AEMtx33Rot(&Option2_angle, 0.0f);
	AEMtx33Scale(&Option2_scale, AEGfxGetWindowWidth() / 2.2f, AEGfxGetWindowHeight() / 10.0f);
	AEMtx33Trans(&Option2_translate, player->player_shape.pos_x, player->player_shape.pos_y);
	AEMtx33Concat(&Option2_transform, &Option2_angle, &Option2_scale);
	AEMtx33Concat(&Option2_transform, &Option2_translate, &Option2_transform);
	AEGfxSetTransform(Option2_transform.m);
	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);

	// Print the 2 postion of element store in the vector array
	AEGfxPrint(font_id, *(displayskillWeapon_txt.begin() + 1), -0.3f, -0.02f, 0.25f, 0.0f, 0.0f, 0.0f, 1.0f);
	#pragma endregion

	#pragma region Option3
	AEMtx33Identity(&Option3_transform);
	AEMtx33Rot(&Option3_angle, 0.0f);
	AEMtx33Scale(&Option3_scale, AEGfxGetWindowWidth() / 2.2f, AEGfxGetWindowHeight() / 10.0f);
	AEMtx33Trans(&Option3_translate, player->player_shape.pos_x, player->player_shape.pos_y - 200.0f);
	AEMtx33Concat(&Option3_transform, &Option3_angle, &Option3_scale);
	AEMtx33Concat(&Option3_transform, &Option3_translate, &Option3_transform);
	AEGfxSetTransform(Option3_transform.m);
	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);

	// Print the 3 postion of element store in the vector array
	AEGfxPrint(font_id, *(displayskillWeapon_txt.begin() + 2), -0.3f, -0.45f, 0.25f, 0.0f, 0.0f, 0.0f, 1.0f);
	#pragma endregion

	#pragma endregion
}


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
void PlayerSelectionOption(std::vector<Weapon_Skill_Item>& vector_data, Player *player, float& player_tempo_fireRate_Tracker, int id) {
	Weapon_Skill_Item selectedchoice = vector_data.at(id); // Base on which button is been pressed, the skill effect/weapon unlock will be apply

	SKILL_PARAMETER* skill = static_cast<SKILL_PARAMETER*>(selectedchoice.data);

	
	if (selectedchoice.isSkill) {// Check if the the selected choice is a skill

		//SKILL_PARAMETER* skill = static_cast<SKILL_PARAMETER*>(selectedchoice.data); // If it is a skill, store the skill effect

		// Check base on the skill name id and the skill effect will be apply accodingly
		//if (selectedchoice.nameID == "HP RECOVER" && player->hp < player->max_hp) { // if player choose to restore health
		if (skill->skilleffectvalue == 20 && player->hp < player->max_hp) { // if player choose to restore health
			player->hp += skill->skilleffectvalue; // player health will be restore
			if (player->hp > player->max_hp) { player->hp = player->max_hp;} // check to ensure that the player health don't go beyone the max hp
		}
		//else if (selectedchoice.nameID == "MAX HP INCREASE") { // if player choose to increase their health
		else if (skill->skilleffectvalue == 10) { // if player choose to increase their health
			player->max_hp += skill->skilleffectvalue; // player max health will increase
			player->hp += skill->skilleffectvalue; // player current health will also increase 
		}

		// if player choose to increase speed, player movement speed will increase
		//else if (selectedchoice.nameID == "MOVEMENT SPD INCREASE") { player->movement_speed += skill->skilleffectvalue * static_cast<float>(AEFrameRateControllerGetFrameTime()); }
		else if (skill->skilleffectvalue == 15) { player->movement_speed += skill->skilleffectvalue * static_cast<float>(AEFrameRateControllerGetFrameTime()); }
		
		// if player choose to increase their damage, player damage will increase
		//else if (selectedchoice.nameID == "DMG INCREASE") { player->damage_output += skill->skilleffectvalue; }
		else if (skill->skilleffectvalue == 1) { player->damage_output += skill->skilleffectvalue; }

		// if player choose to reduce their fireing rate, player fire rate interval will be shorten
		//else if (selectedchoice.nameID == "FIRERATE_CD REDUCE") { player_tempo_fireRate_Tracker-=1.0f; }
		else if (skill->skilleffectvalue == 2) { player_tempo_fireRate_Tracker-=1.0f; }
	}
	else {
		
		// Unlock the selected weapon
		if (selectedchoice.nameID == "PISTOl") {
			UnlockWeapon(WEAPON_LIST::PISTOL);
		}
		else if (selectedchoice.nameID == "RIFLE") {
			UnlockWeapon(WEAPON_LIST::RIFLE);
		}
		else if (selectedchoice.nameID == "SHOTGUN") {
			UnlockWeapon(WEAPON_LIST::SHOTGUN);
		}

		vector_data.erase(vector_data.begin() + id); // remove the weapon data from the skill weapon item vector array
	}

	player->exp -= player->max_exp; // reset the player experience bar
	player->max_exp += player->playerMaxEXPInc; // Increase the player max exp
	player->lvUp = false; // set the player level up flag as false
}
