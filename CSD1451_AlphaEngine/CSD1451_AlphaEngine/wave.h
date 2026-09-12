/* Start Header ************************************************************************/
/*!
\file		wave.h
\author		Zhi Jie 90% (initialize/update/reset_wave_manager), 
            Hao Peng 10% (DrawWaveStatus)
\date		March, 20, 2025
\brief      This file contains the declaration of functions used in wave.cpp.

Components:
- WaveState: Enum defining the current status of wave progression.
- enemystats: Struct holding parameters for individual enemy types.
- Wave: Struct defining the configuration and runtime tracking of a single wave.
- WaveManager: Struct handling the overall wave logic, transitions, and management.

Functions:
- initialise_wave_manager(WaveManager& manager)
- updateWaveManager(WaveManager& manager, float player_pos_x, float player_pos_y, int &WaveScore)
- resetWaveManager(WaveManager& manager)
- DrawWaveStatus(const WaveManager& manager, s8 font_id)

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include <vector>
#include "AEEngine.h"

enum WaveState {
    WAVE_INACTIVE,
    WAVE_STARTING,
    WAVE_ACTIVE,
    WAVE_COMPLETED,
    WAVE_BOSS,
    WAVE_ALL_COMPLETED
};

struct enemystats {
    float radius;
    int hp;
    int exp;
    float movementspd;
    int enemy_type;
};

struct Wave {
    int waveNumber;         // Current wave number
    int enemyCount;         // Total enemies to spawn
    float spawnInterval;    // Time between enemy spawns
    float timeSinceLastSpawn;
    int enemiesSpawned;     // Number of spawned enemies
    enemystats enemyparameters;
};

struct WaveManager {
    WaveState state;  // Current state of the wave
    int currentWave;  // Current wave number
    float timeBetweenWaves; // Delay before next wave
    float waveTimer;  // Timer for wave transitions
    std::vector<Wave> waves; // List of all waves
};

void initialise_wave_manager(WaveManager& manager);

void updateWaveManager(WaveManager& manager, float player_pos_x, float player_pos_y, int &WaveScore);

void resetWaveManager(WaveManager& manager);

void DrawWaveStatus(const WaveManager& manager, s8 font_id);
