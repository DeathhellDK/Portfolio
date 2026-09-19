/**
 * @file      randomiser.cpp
 * @author    Low JianLin
 * @email     jianlin.low
 * @date      2025-11-23
 *
 * @brief	  Implements the non-repeating random selection.
 *
 *  Provides a small utility module that selects a random variant from a
 *  predefined list (e.g., footstep variations, VFX variants, animation cues,
 *  or any other repeated effect). The system avoids repeating the same index
 *  twice in a row when more than one option is available.
 *  Internally, the module owns:
 *    - a shared RNG (std::mt19937)
 *    - the list of variant keys
 *    - a "last used index" tracker
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include "Core/randomiser.h"
#include "Core/resourceManager.h"
#include "World/ai.h"
#include <array>
#include <random>

static constexpr std::array<const char*, 7> kFootstepKeys = {
	"footstep_1",
	"footstep_2",
	"footstep_3",
	"footstep_4",
	"footstep_5",
	"footstep_6",
	"footstep_7"
};

static constexpr std::array<const char*, 7> kAtkKeys = {
	"Slash_1",
	"Slash_2",
	"Slash_3",
	"Slash_4",
	"Slash_5",
	"Slash_6",
	"Slash_7",
};

static constexpr std::array<const char*, 3> kShotKeys = {
	"PlayerShot1",
	"PlayerShot2",
	"PlayerShot3"
};

static constexpr std::array<const char*, 3> kPlayerGruntKeys = {
	"player_grunt1",
	"player_grunt2",
	"player_grunt3"
};

static constexpr std::array<const char*, 2> kKeyCollectedKeys = {
	"key_collected1",
	"key_collected2"
};

static constexpr std::array<const char*, 2> kAbilityAbsorbKeys = {
	"ability_absorb1",
	"ability_absorb2"
};

/**
 * @brief Get a reference to the shared random engine used for footsteps.
 *
 * Lazily constructs and returns a static std::mt19937 instance that is reused
 * for all footstep randomisation, so that the distribution is stable and does
 * not re-seed every call.
 *
 * @return Reference to the internal random number generator.
 */
static std::mt19937& Rng() {
	static std::mt19937 rng{ std::random_device{}() };
	return rng;
}

/**
 * @brief Play a random non-repeating sound from a list of keys.
 *
 * @param keys     Array of audio keys to choose from.
 * @param keyCount Number of keys in the array.
 * @param lastIdx  Reference to previous index for repeat prevention.
 * @param gain     Linear gain multiplier.
 */
static void PlayRandomFromList(
	const char* const* keys,
	size_t keyCount,
	int& lastIdx,
	float gain)
{
	if (!keys || keyCount == 0) return;

	auto& rng = Rng();
	std::uniform_int_distribution<int> dist(0, (int)keyCount - 1);

	int idx = dist(rng);

	if (keyCount > 1 && idx == lastIdx)
		idx = (idx + 1) % (int)keyCount;

	lastIdx = idx;
	if (keys[idx])
		ResourceManager::PlaySfx(keys[idx], gain);
}

/**
 * @brief Selects and plays a spatialized random variant by constructing asset keys dynamically.
 * * This helper avoids hardcoded arrays by combining a base string (e.g., "mob_footstep")
 * with a random index. It applies 2D panning and logarithmic volume attenuation
 * relative to the player's position.
 * * @param app      Reference to the GameApp to retrieve player/listener data.
 * @param baseKey  The prefix of the sound in the JSON (e.g., "range_footstep").
 * @param count    The total number of variants available for this sound.
 * @param lastIdx  Reference to a persistent integer to prevent repeat variations.
 * @param pos      The world-space position where the sound is emitted.
 * @param maxGain  The maximum volume (ceiling) for the sound.
 */
static void PlayDynamicRandomSfx(const GameApp& app, const std::string& baseKey, int count,
	int& lastIdx, glm::vec2 pos, float maxGain)
{
	if (count <= 0) return;

	// Reuse existing RNG logic
	static std::mt19937 rng{ std::random_device{}() };
	std::uniform_int_distribution<int> dist(1, count);

	int idx = dist(rng);
	if (count > 1 && idx == lastIdx) {
		idx = (idx % count) + 1;
	}
	lastIdx = idx;

	// Construct key: "mob_footstep" + "1" = "mob_footstep1"
	std::string finalKey = baseKey + std::to_string(idx);

	// Call the spatialized PlaySfx
	// minRoll: 100.0f, maxRoll: 500.0f
	ResourceManager::PlaySfx(app, finalKey, pos, 100.0f, 500.0f, maxGain);
}

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
void PlayRandomFootstep(float gain)
{
	static int lastFootstep = -1;
	PlayRandomFromList(
		kShotKeys.data(),
		kShotKeys.size(),
		lastFootstep,
		gain
	);
}

/**
 * @brief Play a random sound effect for right mouse button actions.
 *
 * Used for attacks, swings, or any repeated RMB-triggered effect that
 * benefits from audio variation to avoid repetition.
 *
 * @param gain Linear gain multiplier for volume control.
 */
void PlayRandomAtkSfx(float gain)
{
	static int lastRmb = -1;

	PlayRandomFromList(
		kAtkKeys.data(),
		kAtkKeys.size(),
		lastRmb,
		gain
	);
}

void PlayRandomProjectileShotSfx(float gain)
{
	static int lastShot = -1;

	PlayRandomFromList(
		kShotKeys.data(),
		kShotKeys.size(),
		lastShot,
		gain
	);
}

void PlayRandomPlayerGruntSfx(float gain)
{
	static int lastGrunt = -1;

	PlayRandomFromList(
		kPlayerGruntKeys.data(),
		kPlayerGruntKeys.size(),
		lastGrunt,
		gain
	);
}

void PlayRandomKeyCollectedSfx(float gain)
{
	static int lastKey = -1;

	PlayRandomFromList(
		kKeyCollectedKeys.data(),
		kKeyCollectedKeys.size(),
		lastKey,
		gain
	);
}

void PlayRandomAbilityAbsorbSfx(float gain)
{
	static int lastKey = -1;

	PlayRandomFromList(
		kAbilityAbsorbKeys.data(),
		kAbilityAbsorbKeys.size(),
		lastKey,
		gain
	);
}

/**
 * @brief Triggers a spatialized footstep sound based on the enemy's mobility type.
 * * Maps MobType enums to specific JSON asset prefixes. This ensures a Ranged
 * enemy sounds different from a Burrowing or Healing enemy.
 * @param app  Reference to the GameApp for coordinate calculations.
 * @param type The MobType of the enemy (used to determine the sound set).
 * @param pos  The current world position of the enemy entity.
 * @param gain The volume multiplier for the footstep.
 */
void PlayRandomEnemyFootstep(const GameApp& app, MobType type, glm::vec2 pos, float gain) {
	// Separate trackers so different mobs don't interfere with each other's "last played"
	static int lBasic = -1, lRange = -1, lHeal = -1, lBurrow = -1, lFinalBoss = -1;

	switch (type) {
	case MobType::RANGED:
		PlayDynamicRandomSfx(app, "range_footstep", 2, lRange, pos, gain); break;
	case MobType::HEAL:
		PlayDynamicRandomSfx(app, "heal_footstep", 2, lHeal, pos, gain); break;
	case MobType::BURROW:
		PlayDynamicRandomSfx(app, "burrow_footstep", 2, lBurrow, pos, gain); break;
	case MobType::FINALBOSS:
		PlayDynamicRandomSfx(app, "finalboss_footstep", 2, lFinalBoss, pos, gain); break;
	default:
		PlayDynamicRandomSfx(app, "basic_footstep", 2, lBasic, pos, gain); break;
	}
}

/**
 * @brief Plays a randomized, spatialized sound effect from a pool of variations.
 */
void PlayRandomFloorCrack(GameApp& app, const std::string& baseKey, glm::vec2 pos, int variations, float gain) {
	// 1. Randomly select one of the numbered variations
	int choice = (std::rand() % variations) + 1;
	std::string finalKey = baseKey + std::to_string(choice);

	// 2. Add slight pitch/gain variation for even more uniqueness
	float randomGain = gain + ((std::rand() % 20 - 10) / 100.0f); // +/- 0.1 variance

	// 3. Call the engine's spatialized PlaySfx
	// Use a relatively small maxRoll (300.0f) since it's a localized floor sound
	ResourceManager::PlaySfx(app, finalKey, pos, 50.0f, 300.0f, randomGain);
}

void PlayRandomLeverActivate(GameApp& app, const std::string& baseKey, glm::vec2 pos, int variations, float gain) {
	// 1. Randomly select one of the numbered variations
	int choice = (std::rand() % variations) + 1;
	std::string finalKey = baseKey + std::to_string(choice);

	// 2. Add slight pitch/gain variation for even more uniqueness
	float randomGain = gain + ((std::rand() % 20 - 10) / 100.0f); // +/- 0.1 variance

	// 3. Call the engine's spatialized PlaySfx
	ResourceManager::PlaySfx(app, finalKey, pos, 50.0f, 300.0f, randomGain);
}

void PlayRandomEnemyDeath(const GameApp& app, MobType type, bool isBoss, glm::vec2 pos) {
	std::string baseDeathKey = "basic_death";
	float baseVolume = 1.0f;
	int variations = 1;

	// 1. Determine base parameters
	if (isBoss) {
		baseDeathKey = "finalboss_death";
		baseVolume = 5.0f;
		variations = 5;
	}
	else {
		switch (type) {
		case MobType::RANGED:
			baseDeathKey = "range_death";
			baseVolume = 0.7f;
			variations = 6;
			break;
		case MobType::HEAL:
			baseDeathKey = "heal_death";
			baseVolume = 0.4f;
			variations = 7;
			break;
		case MobType::BURROW:
			baseDeathKey = "burrow_death";
			baseVolume = 1.0f;
			variations = 4;
			break;
		default:
			baseDeathKey = "basic_death";
			baseVolume = 0.3f;
			variations = 3;
			break;
		}
	}

	// 2. Asset Randomization
	int assetIndex = (std::rand() % variations) + 1;
	std::string finalDeathKey = baseDeathKey + std::to_string(assetIndex);

	// 3. Volume Randomization (+/- 15% variance)
	float variance = (std::rand() % 31 - 15) / 100.0f;
	float finalVolume = baseVolume * (1.0f + variance);
	if (finalVolume < 0.1f) finalVolume = 0.1f;

	// 4. Play spatialized sound
	ResourceManager::PlaySfx(app, finalDeathKey, pos, 100.0f, 700.0f, finalVolume);
}

void PlayRandomEnemyDamaged(const GameApp& app, MobType type, bool isBoss, glm::vec2 pos) {
	std::string baseHurtKey = "basic_damaged";
	float baseVolume = 1.0f;
	int variations = 1;

	// 1. Determine base parameters for Hurt SFX
	if (isBoss) {
		baseHurtKey = "finalboss_damaged";
		baseVolume = 2.5f;
		variations = 8;
	}
	else {
		switch (type) {
		case MobType::RANGED:
			baseHurtKey = "range_damaged";
			baseVolume = 0.5f;
			variations = 8;
			break;
		case MobType::HEAL:
			baseHurtKey = "heal_damaged";
			baseVolume = 0.2f;
			variations = 7;
			break;
		case MobType::BURROW:
			baseHurtKey = "burrow_damaged";
			baseVolume = 0.6f;
			variations = 4;
			break;
		default:
			baseHurtKey = "basic_damaged";
			baseVolume = 0.25f;
			variations = 5;
			break;
		}
	}

	// 2. Asset Randomization
	int assetIndex = (std::rand() % variations) + 1;
	std::string finalHurtKey = baseHurtKey + std::to_string(assetIndex);

	// 3. Volume Randomization (+/- 15% variance)
	float variance = (std::rand() % 31 - 15) / 100.0f;
	float finalVolume = baseVolume * (1.0f + variance);
	if (finalVolume < 0.1f) finalVolume = 0.1f;

	// 4. Play spatialized sound (Slightly smaller maxRoll so distant enemies aren't too noisy)
	ResourceManager::PlaySfx(app, finalHurtKey, pos, 100.0f, 600.0f, finalVolume);
}

void PlayRandomEnemyAttack(const GameApp& app, MobType type, bool isBoss, glm::vec2 pos, float maxRoll) {
	if (type == MobType::RANGED && !isBoss) {
		return;
	}

	std::string baseAtkKey = "basic_attack";
	float baseVolume = 1.0f;
	int variations = 1;

	// 1. Determine base parameters for Attack SFX
	if (isBoss) {
		baseAtkKey = "finalboss_attack";
		baseVolume = 2.0f; // Needs to be imposing, but not as loud as dying
		variations = 5;
	}
	else {
		switch (type) {
		case MobType::HEAL:
			baseAtkKey = "heal_attack";
			baseVolume = 0.7f;
			variations = 9;
			break;
		case MobType::BURROW:
			baseAtkKey = "burrow_attack";
			baseVolume = 0.9f;
			variations = 6;
			break;
		default:
			baseAtkKey = "basic_attack";
			baseVolume = 0.7f;
			variations = 6;
			break;
		}
	}

	// 2. Asset Randomization
	int assetIndex = (std::rand() % variations) + 1;
	std::string finalAtkKey = baseAtkKey + std::to_string(assetIndex);

	// 3. Volume Randomization (+/- 15% variance)
	float variance = (std::rand() % 31 - 15) / 100.0f;
	float finalVolume = baseVolume * (1.0f + variance);
	if (finalVolume < 0.1f) finalVolume = 0.1f;

	// 4. Play spatialized sound
	ResourceManager::PlaySfx(app, finalAtkKey, pos, 100.0f, maxRoll, finalVolume);
}