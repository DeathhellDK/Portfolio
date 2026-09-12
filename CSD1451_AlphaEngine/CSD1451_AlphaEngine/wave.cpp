/* Start Header ************************************************************************/
/*!
\file		wave.cpp
\author		Zhi Jie 90% (initialize/update/reset_wave_manager), 
            Hao Peng 10% (Draw Wave Status)
\date		March, 20, 2025
\brief      The Wave Manager system controls the spawning and progression of enemy waves 
in the game. It initializes wave parameters such as enemy count, spawn interval, and enemy 
attributes. The initialise_wave_manager function sets up predefined waves, including standard 
enemies and boss encounters. The updateWaveManager function handles wave progression, spawning 
enemies at set intervals, and transitioning between wave states, including inactive, active, 
boss, and completed states. It ensures smooth wave transitions, adjusting delays for boss 
fights and special waves. The resetWaveManager function resets the wave system, allowing 
for a fresh game session by clearing and reinitializing waves. The DrawWaveStatus function 
provides real-time wave and enemy count information on-screen, displaying the current wave 
number, total spawned enemies, and remaining enemies. The system dynamically manages enemy 
waves, balancing difficulty progression while ensuring engaging gameplay.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "wave.h"
#include "enemy.h"
#include "iostream"

void initialise_wave_manager(WaveManager& manager) {
    manager.state = WAVE_INACTIVE;
    manager.currentWave = 0;
    manager.timeBetweenWaves = 25.0f;
    manager.waveTimer = 0.0f;

    // Define waves (wave number, enemy count, spawn interval)
    // Enemy stats (radius, hp, exp, movementspd)

    manager.waves.push_back({ 1, 10, 2.0f, 0.0f, 0, enemystats { 50.0f, 5, 5, 1.0f, 0 } });
    
    manager.waves.push_back({ 2, 15, 0.5f, 0.0f, 0, enemystats { 40.0f, 5, 7, 3.0f, 0 } });
    
    manager.waves.push_back({ 3, 8, 0.0f, 0.0f, 0, enemystats { 50.0f, 5, 10, 0.0f, 2 } });

    manager.waves.push_back({ 4, 20, 0.6f, 0.0f, 0, enemystats { 35.0f, 5, 12, 3.0f, 0 } });

    // boss wave
    manager.waves.push_back({ 5, 1, 2.0f, 0.0f, 0, enemystats { 100.0f, 100, 50, 1.0f, 1 } });

    manager.waves.push_back({ 6, 20, 0.2f, 0.0f, 0, enemystats { 50.0f, 25, 15, 2.5f, 0 } });

    manager.waves.push_back({ 7, 20, 0.4f, 0.0f, 0, enemystats { 35.0f, 20, 15, 3.8f, 0 } });

    manager.waves.push_back({ 8, 8, 0.0f, 0.0f, 0, enemystats { 25.0f, 30, 20, 0.0f, 2 } });

    manager.waves.push_back({ 9, 20, 0.6f, 0.0f, 0, enemystats { 25.0, 30, 25, 5.5f, 0 } });

    // boss
    manager.waves.push_back({ 10, 1, 2.0f, 0.0f, 0, enemystats { 125.0f, 150, 75, 1.0f, 1 } });
}


void updateWaveManager(WaveManager& manager, float player_pos_x, float player_pos_y,int &WaveScore) {
    bool firstwavespawned = false;
    static double lastWaveStartTime = AEGetTime(&lastWaveStartTime);  
    static double lastEnemySpawnTime = AEGetTime(&lastEnemySpawnTime); 

    double currentTime = AEGetTime(&currentTime); 

    switch (manager.state) {
    case WAVE_INACTIVE:
        //std::cout << "inactive\n";
        if (!firstwavespawned) {
            manager.state = WAVE_STARTING;  // Start the first wave
            firstwavespawned = true;
            lastWaveStartTime = currentTime;  // Initialize wave start time
        }
        break;

    case WAVE_STARTING:
        //std::cout << "start\n";
        if (manager.currentWave < manager.waves.size()) {
            manager.state = WAVE_ACTIVE;
        }
        break;

    case WAVE_ACTIVE:
        //std::cout << "active\n";
    case WAVE_BOSS:
        //std::cout << "boss\n";
        if (manager.currentWave < manager.waves.size()) {
            Wave& currentWave = manager.waves[manager.currentWave];

            if (currentWave.enemiesSpawned < currentWave.enemyCount &&
                currentTime - lastEnemySpawnTime >= currentWave.spawnInterval) {

                addenemy(currentWave.enemyparameters.radius,
                    currentWave.enemyparameters.hp,
                    currentWave.enemyparameters.exp,
                    currentWave.enemyparameters.movementspd,
                    currentWave.enemyparameters.enemy_type,
                    player_pos_x, player_pos_y);

                currentWave.enemiesSpawned++;
                lastEnemySpawnTime = currentTime;  // Reset enemy spawn timer
            }

            // If all enemies have been spawned, move to next state
            if (currentWave.enemiesSpawned >= currentWave.enemyCount) {
                manager.state = WAVE_COMPLETED;
                WaveScore += 1;
                lastWaveStartTime = currentTime;  // Reset wave timer
            }
        }
        break;

    case WAVE_COMPLETED:
        if (all_enemies_killed(wave) || ((currentTime - lastWaveStartTime) >= manager.timeBetweenWaves)) {
            manager.currentWave++;

            if (manager.currentWave >= manager.waves.size()) {
                manager.state = WAVE_ALL_COMPLETED;  // Set to victory state instead of inactive
            }
            else {
                // Check if the next wave is a boss wave
                if ((manager.currentWave + 1) % 5 == 0) {
                    manager.state = WAVE_BOSS;
                    manager.timeBetweenWaves = 45.0f;  // Boss waves have a longer delay
                }
                else if ((manager.currentWave + 1) % 5 == 3) {
                    manager.state = WAVE_STARTING;
                    manager.timeBetweenWaves = 0.0f;
                }
                else {
                    manager.state = WAVE_STARTING;
                    manager.timeBetweenWaves = 25.0f;  // Normal waves have a shorter delay
                }

                lastWaveStartTime = currentTime;  // Reset wave timer
                lastEnemySpawnTime = currentTime; // Reset enemy spawn timer
            }
        }
        break;
    }
}

void resetWaveManager(WaveManager& manager) {
    manager.state = WAVE_INACTIVE;  // Set state to inactive
    manager.currentWave = 0;        // Reset to the first wave
    manager.timeBetweenWaves = 25.0f;  // Set the default delay
    manager.waveTimer = 0.0f;       // Reset wave timer
    manager.waves.clear();          // Clear any defined waves
}

void DrawWaveStatus(const WaveManager& manager, s8 font_id) {
    int totalEnemies = static_cast<int>(wave.size());  // Total spawned in this wave
    int aliveEnemies = get_current_enemy_count(wave);
    int killedEnemies = 0;
    killedEnemies = totalEnemies - aliveEnemies;

    // Wave Counter
    char waveText[50];
    snprintf(waveText, sizeof(waveText), "Wave: %d / %d", manager.currentWave + 1, (int)manager.waves.size());
    AEGfxPrint(font_id, waveText, -0.3f, 0.865f, 0.4f, 1.0f, 1.0f, 1.0f, 1.0f);

    // Enemy Counter (Killed / Total)
    char enemyText[50];
    snprintf(enemyText, sizeof(enemyText), "Enemies: %d", totalEnemies);
    AEGfxPrint(font_id, enemyText, 0.2f, 0.865f, 0.4f, 1.0f, 1.0f, 1.0f, 1.0f);

}