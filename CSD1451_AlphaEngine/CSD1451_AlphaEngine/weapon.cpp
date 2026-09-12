/* Start Header ************************************************************************/
/*!
\file		weapon.cpp
\author		Ye Tingkai
\date		March, 20, 2025
\brief		This file contains the implementation of definitions from weapon.h
            The functions below implements:
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

#include "weapon.h"
#include "Main.h"
#include "AEEngine.h"
#include "Player.h"
#include <vector>
#include "audio.h"
#include "Game_Management.h"


// =========================================================
// Struct to track each weapon's state
// 
// - weapon's unlocked state will be used by skill menu
// - cooldown to fire alongside player starter weapom
// - bullet counter (weapons like shotgun fire more bullets)
// - burst counter/delay/timer (Rifle specific only)
//      - shots interval in one burst
// 
// =========================================================
struct WeaponState {
    WEAPON_LIST type;
    bool unlocked;
    float current_cooldown;
    float max_cooldown;
    int damage;
    float range;  // Represented as bullet lifespan
    int bullet_count;  // For shotgun spread or rifle burst

    // Rifle burst specific
    int burst_counter;  // How many shots have been fired in current burst
    float burst_delay;  // Delay between burst shots
    float burst_timer;  // Timer for next burst shot

    WeaponState(WEAPON_LIST t, bool u, float cd, int dmg, float r, int bc) :
        type(t), unlocked(u), current_cooldown(0), max_cooldown(cd),
        damage(dmg), range(r), bullet_count(bc), burst_counter(0), burst_delay(2.0f), burst_timer(0) {
    }
};


// =========================================================
// Global vector to store the state of all weapons
// =========================================================
std::vector<WeaponState> player_weapons;

// =========================================================
// Initialize weapon states (call once at game start)
// 
// - Contains: (New weapons can be added to here)
// - Pistol
// - Rifle
// - Shotgun
// =========================================================
void InitializeWeapons() {
    player_weapons.clear();
    // Pistol: faster firing than starter weapon
    player_weapons.push_back(WeaponState(WEAPON_LIST::PISTOL, false, 8.0f, 2, 10.0f, 1));
    // Rifle: 3 round burst, longer range
    player_weapons.push_back(WeaponState(WEAPON_LIST::RIFLE, false, 12.0f, 1, 20.0f, 3));
    // Shotgun: 5 bullet spread, medium range
    player_weapons.push_back(WeaponState(WEAPON_LIST::SHOTGUN, false, 15.0f, 4, 14.0f, 5));
}


// =========================================================
// Unlock a weapon when selected from the skill menu
// 
// - checks weapon type
// - calls function to get weapon unlocked state
// - unlocks if not unlocked
//      - return true if it is just unlocked
//      - return false when already unlocked
// =========================================================
bool UnlockWeapon(WEAPON_LIST type) {
    for (auto& weapon : player_weapons) {
        if (weapon.type == type) {
            if (!weapon.unlocked) {
                weapon.unlocked = true;
                return true;  // Successfully unlocked
            }
            return false;  // Already unlocked
        }
    }
    return false;  // Weapon not found
}

// =========================================================
// Update all weapon cooldowns
// - Decrements cooldown for all unlocked weapons
// =========================================================
void UpdateWeaponCooldowns() {
    for (auto& weapon : player_weapons) {
        if (weapon.unlocked && weapon.current_cooldown > 0) {
            weapon.current_cooldown -= 0.1f;  // Same decrement as in the game loop
        }
    }
}


// =========================================================
// Fire a weapon if it's unlocked and off cooldown
// 
// - checks weapon type that is unlocked
// - checks if weapon cooldown is depleted
//      - reset cooldown to full
// 
// - New weapons can be added under the switch statement
// 
// - Pistol
//      - A longer range version of the player starter weapon
// 
// - Rifle
//      - 3 round burst, longer range
// 
// - Shotgun
//      - 5 rounds, spreadout in a cone
//      - range between Rifle and Pistol
// =========================================================
void FireWeapon(WEAPON_LIST type, std::vector<Bullet>& bulletVector, float playerX, float playerY,
    int playerDamage, float playerRotation) {

    for (auto& weapon : player_weapons) {
        if (weapon.type == type && weapon.unlocked && weapon.current_cooldown <= 0) {
            // Set the weapon on cooldown
            weapon.current_cooldown = weapon.max_cooldown;

            // Weapon-specific firing behavior
            switch (type) {
            case WEAPON_LIST::PISTOL: {
                // Single shot pistol
                Bullet pistolBullet(weapon.damage, 550, weapon.range, playerX, playerY, BULLET_FIRED_BY::PLAYER, playerRotation);
                BulletInit(bulletVector, pistolBullet, playerX, playerY, playerDamage, MAX_AMMO_SIZE, BULLET_FIRED_BY::PLAYER, playerRotation);
                PlayGameSound(WEAPON1_SHOOT);
                break;
            }

            case WEAPON_LIST::RIFLE: {
                // 3-round burst rifle
                // For rifle, only start a new burst if not already in one
                if (weapon.burst_counter == 0) {
                    // Fire first shot of burst
                    Bullet rifleBullet(weapon.damage, 650, weapon.range, playerX, playerY, BULLET_FIRED_BY::PLAYER, playerRotation);
                    BulletInit(bulletVector, rifleBullet, playerX, playerY, playerDamage, MAX_AMMO_SIZE, BULLET_FIRED_BY::PLAYER, playerRotation);
                    PlayGameSound(WEAPON2_SHOOT);

                    // Start burst sequence
                    weapon.burst_counter = 1;  // First shot fired
                    weapon.burst_timer = weapon.burst_delay;  // Set timer for next shot
                }
                break;
            }

            case WEAPON_LIST::SHOTGUN: {
                // Shotgun spread (5 bullets in a cone)
                float spreadAngle = 0.2f;  // Spread in radians
                for (int i = 0; i < weapon.bullet_count; ++i) {
                    // Calculate spread angle for this bullet
                    float angle = playerRotation + spreadAngle * (i - (weapon.bullet_count - 1) / 2.0f);
                    Bullet shotgunBullet(weapon.damage, 500, weapon.range, playerX, playerY, BULLET_FIRED_BY::PLAYER, angle);
                    BulletInit(bulletVector, shotgunBullet, playerX, playerY, playerDamage, MAX_AMMO_SIZE, BULLET_FIRED_BY::PLAYER, angle);
                }
                PlayGameSound(WEAPON3_SHOOT);
                break;
            }

            default:
                break;
            }
            break;  // Exit the loop after firing the weapon
        }
    }
}


// =========================================================
// Burst rifle specific function
// 
// - Checks for only if the weapon is a rifle
//     - Checks using weapon enum
// 
// - Burst timer (Time until next burst shot, not main weapon cooldown)
// - Burst counter (Number of Bullets fired)
// - Burst delay (Delay until next shot within burst)
// =========================================================
void UpdateRifleBurst(std::vector<Bullet>& bulletVector, float playerX, float playerY,
    int playerDamage, float playerRotation) {
    for (auto& weapon : player_weapons) {
        if (weapon.type == WEAPON_LIST::RIFLE && weapon.unlocked && weapon.burst_counter > 0) {
            // In the middle of a burst
            weapon.burst_timer -= 0.1f;  // Same decrement as other timers

            if (weapon.burst_timer <= 0) {
                // Time to fire next shot in burst
                Bullet rifleBullet(weapon.damage, 650, weapon.range, playerX, playerY, BULLET_FIRED_BY::PLAYER, playerRotation);
                BulletInit(bulletVector, rifleBullet, playerX, playerY, playerDamage, MAX_AMMO_SIZE, BULLET_FIRED_BY::PLAYER, playerRotation);
                PlayGameSound(WEAPON3_SHOOT);

                weapon.burst_counter++;  // Increment counter

                if (weapon.burst_counter >= weapon.bullet_count) {
                    // Burst complete
                    weapon.burst_counter = 0;
                    weapon.current_cooldown = weapon.max_cooldown;  // Start cooldown after burst completes
                }
                else {
                    // Set timer for next burst shot
                    weapon.burst_timer = weapon.burst_delay;
                }
            }
        }
    }
}


// =========================================================
// Check if a weapon is unlocked
// - Checks using weapon enum
// - From the checked weapon type, get unlock state
// =========================================================
bool IsWeaponUnlocked(WEAPON_LIST type) {
    for (const auto& weapon : player_weapons) {
        if (weapon.type == type) {
            return weapon.unlocked;
        }
    }
    return false;
}


// =========================================================
// Fire all unlocked weapons if they're off cooldown
// 
// - Uses the bullet vector used globally in game_management.cpp
// 
// - Gets player position and rotation
//      - Fires weapon based on the player current parameters
//      - Fires in direction of player rotation
// 
// - Calls Fireweapon() to fire unlocked weapons based on cooldown
// =========================================================
void FireAllUnlockedWeapons(std::vector<Bullet>& bulletVector, float playerX, float playerY,
    int playerDamage, float playerRotation) {
    for (auto& weapon : player_weapons) {
        if (weapon.unlocked && weapon.current_cooldown <= 0) {
            FireWeapon(weapon.type, bulletVector, playerX, playerY, playerDamage, playerRotation);
        }
    }
}