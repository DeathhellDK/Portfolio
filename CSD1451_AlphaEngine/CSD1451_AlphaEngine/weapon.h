/* Start Header ************************************************************************/
/*!
\file		weapon.h
\author		Ye Tingkai , 
            Tan Wei Liang Terril(1% - Set up the struct WEAPON_PARAMETER and enum class WEAPON_LIST )
\date		March, 20, 2025
\brief		This file contains the definitions for weapon.cpp
            Contains definitions for:

            - Weapon enums
            - Weapon parameter for use in skillmenu display

            - weapon state (locked/unlocked, cooldowns)
            - initialise weapons
            - unlock weapons
            - update weapon cooldowns
            - fire weapons
            - update rifle burst cooldwon
            - fire unlocked weapon if unlocked and off cooldown
            - check if weapon is unlocked

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef weapon_H
#define weapon_H

#include "string"
#include "AEEngine.h"
#include "Player.h"
#include "Main.h"
#include <vector>

/// <summary>
/// List of Weapon class that player will be able to unlock
/// </summary>
enum class WEAPON_LIST { PISTOL, RIFLE, SHOTGUN};

/// <summary>
/// struct of weapon parameter member
/// </summary>
struct WEAPON_PARAMETER {
    int damage;
    float firerate_cd;
};


/// <summary>
/// Initialize all weapons at the start of the game
/// </summary>
void InitializeWeapons();


/// <summary>
/// Unlock a specific weapon when selected from the skill menu
/// </summary>
/// <param name="WEAPON_LIST type"> - Unlocks a type of weapon </param>
/// <returns></returns>
bool UnlockWeapon(WEAPON_LIST type);

/// <summary>
//// Update cooldowns for all weapons
/// </summary>
void UpdateWeaponCooldowns();

/// <summary>
/// Fire a specific weapon
/// </summary>
/// <param name="WEAPON_LIST type"> - Checks for type of weapon </param>
/// <param name="bulletVector"> - Vector list of bullets to use </param>
/// <param name="playerX"> - Player coordinate on window screen </param>
/// <param name="playerY"> - Player coordinate on window screen </param>
/// <param name="playerDamage"> - Damage done by player </param>
/// <param name="playerRotation"> - Direction player is facing </param>
/// <returns></returns>
void FireWeapon(WEAPON_LIST type, std::vector<Bullet>& bulletVector, float playerX, float playerY,
    int playerDamage, float playerRotation);


/// <summary>
/// Rifle 3 round burst update
/// </summary>
/// <param name="bulletVector"> - Vector list of bullets to use </param>
/// <param name="playerX"> - Player coordinate on window screen </param>
/// <param name="playerY"> - Player coordinate on window screen </param>
/// <param name="playerDamage"> - Damage done by player </param>
/// <param name="playerRotation"> - Direction player is facing </param>
/// <returns></returns>
void UpdateRifleBurst(std::vector<Bullet>& bulletVector, float playerX, float playerY,
    int playerDamage, float playerRotation);


/// <summary>
/// Check if a weapon is unlocked
/// </summary>
/// <param name="WEAPON_LIST type"> - Checks for type of weapon </param>
/// <returns></returns>
bool IsWeaponUnlocked(WEAPON_LIST type);


/// <summary>
/// Fire all unlocked weapons that are off cooldown
/// </summary>
/// <param name="bulletVector"> - Vector list of bullets to use </param>
/// <param name="playerX"> - Player coordinate on window screen </param>
/// <param name="playerY"> - Player coordinate on window screen </param>
/// <param name="playerDamage"> - Damage done by player </param>
/// <param name="playerRotation"> - Direction player is facing </param>
/// <returns></returns>
void FireAllUnlockedWeapons(std::vector<Bullet>& bulletVector, float playerX, float playerY,
    int playerDamage, float playerRotation);

//} weapon;
#endif // DEBUG
