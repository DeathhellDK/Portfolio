#pragma once
/**
 * @file      randomiser.h
 * @author    Low JianLin
 * @email     jianlin.low
 * @date      2025-11-23
 *
 * @brief	  Public API for triggering randomised actions or effects.
 *
 * Declares lightweight external functions for requesting a randomised
 * variation of an effect. so gameplay systems do not need to know how
 * random selection, indexing, or bookkeeping is implemented internally
 * Internals such as RNG state or the list of available footstep keys are
 * intentionally hidden in the .cpp file to keep the gameplay API simple
 * and consistent across different controllers or characters.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */

#include <string>
#include <glm/glm.hpp>

 // Forward declaration to avoid circular includes
class GameApp;
enum class MobType : uint8_t;

/**
 * @brief Play a random footstep sound from the configured list.
 *
 * Randomly selects one of the predefined footstep keys (e.g. "footstep_1",
 * "footstep_2", ...), ensuring that the same index is not chosen twice in a
 * row when more than one option exists. The chosen sound is then played on
 * the Sfx bus via ResourceManager::PlaySfx().
 *
 * @param gain Linear gain multiplier applied to this footstep instance.
 */
void PlayRandomFootstep(float gain = 1.0f);

/**
  * @brief Play a random attack sound from the configured list.
  *
  * Randomly selects one of the predefined attack keys (e.g. "Slash_1",
  * "Slash_2", ...), ensuring that the same index is not chosen twice in a
  * row when more than one option exists. The chosen sound is then played on
  * the Sfx bus via ResourceManager::PlaySfx().
  *
  * @param gain Linear gain multiplier applied to this attack instance.
  */
void PlayRandomAtkSfx(float gain = 1.0f);

/**
 * @brief Play a random projectile shot sound.
 * @param gain Linear gain multiplier applied to this sound.
 */
void PlayRandomProjectileShotSfx(float gain = 1.0f);

/**
 * @brief Play a random player grunt sound (e.g. when taking damage).
 * @param gain Linear gain multiplier applied to this sound.
 */
void PlayRandomPlayerGruntSfx(float gain = 1.0f);

/**
 * @brief Play a random key collected sound.
 * @param gain Linear gain multiplier applied to this sound.
 */
void PlayRandomKeyCollectedSfx(float gain = 1.0f);

/**
 * @brief Play a random ability absorbed sound.
 * @param gain Linear gain multiplier applied to this sound.
 */
void PlayRandomAbilityAbsorbSfx(float gain = 1.0f);

/**
 * @brief Play a random enemy footstep sound.
 * @param app Reference to the GameApp instance.
 * @param type The mob type of the enemy.
 * @param pos The world position of the enemy for spatial audio.
 * @param gain Linear gain multiplier applied to this sound.
 */
void PlayRandomEnemyFootstep(const GameApp& app, MobType type, glm::vec2 pos, float gain);

/**
 * @brief Play a random floor cracking sound.
 * @param app Reference to the GameApp instance.
 * @param baseKey The base string key for the audio asset.
 * @param pos The world position of the floor crack.
 * @param variations Number of variations available for this sound.
 * @param gain Linear gain multiplier applied to this sound.
 */
void PlayRandomFloorCrack(GameApp& app, const std::string& baseKey, glm::vec2 pos, int variations, float gain);

/**
 * @brief Play a random lever activation sound.
 * @param app Reference to the GameApp instance.
 * @param baseKey The base string key for the audio asset.
 * @param pos The world position of the lever.
 * @param variations Number of variations available for this sound.
 * @param gain Linear gain multiplier applied to this sound.
 */
void PlayRandomLeverActivate(GameApp& app, const std::string& baseKey, glm::vec2 pos, int variations, float gain);

/**
 * @brief Play a random enemy death sound.
 * @param app Reference to the GameApp instance.
 * @param type The mob type of the enemy.
 * @param isBoss True if the enemy is a boss.
 * @param pos The world position where the enemy died.
 */
void PlayRandomEnemyDeath(const GameApp& app, MobType type, bool isBoss, glm::vec2 pos);

/**
 * @brief Play a random enemy damaged sound.
 * @param app Reference to the GameApp instance.
 * @param type The mob type of the enemy.
 * @param isBoss True if the enemy is a boss.
 * @param pos The world position where the enemy was damaged.
 */
void PlayRandomEnemyDamaged(const GameApp& app, MobType type, bool isBoss, glm::vec2 pos);

/**
 * @brief Play a random enemy attack sound.
 * @param app Reference to the GameApp instance.
 * @param type The mob type of the enemy.
 * @param isBoss True if the enemy is a boss.
 * @param pos The world position where the enemy is attacking.
 * @param maxRoll Maximum roll value used for determining the variation.
 */
void PlayRandomEnemyAttack(const GameApp& app, MobType type, bool isBoss, glm::vec2 pos, float maxRoll);