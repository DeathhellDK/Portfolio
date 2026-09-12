/* Start Header ************************************************************************/
/*!
\file		Gameplay.cpp
\author		Tan Wei Liang Terril, Zhi Jie, Tingkai, Hao Peng, Jia Hao
\date		March, 20, 2025
\brief      Implementation of core gameplay mechanics,
            player controls, and enemy interactions.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include <crtdbg.h> // To check for memory leaks
#include <stdio.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>

#include "Audio.h"
#include "Collision.h"
#include "Credit.h"
#include "enemy.h"
#include "Game_Management.h"
#include "Gameplay.h"
#include "GameStateList.h"
#include "GameStateManager.h"
#include "Howtoplay.h"
#include "Main.h"
#include "Menu.h"
#include "Options.h"
#include "Player.h"
#include "SkillMenu.h"
#include "wave.h"
#include "weapon.h"

// Global variables needed for gameplay
static Player* player;
static ScoreBoard* playerScore;
static AEGfxVertexList* recMesh = nullptr, * circleMesh = nullptr, * triMesh = nullptr;
static s8 fontType;
static Mouse* user_mouse;
static float rotateX, rotateY;
static float bossrotateX, bossrotateY;
static float enemyrotation;
static shape* option1, * option2, * option3;
static shape _optionBtn1{}, _optionBtn2{}, _optionBtn3{};
static std::vector<Bullet> playerBullet;
static std::vector<Bullet> enemyBullet;
static std::vector<Weapon_Skill_Item> itemDB;
static std::vector<std::string> skillWeapon_txt;
static std::vector<const char*> displayskillWeapon_txt;
static Collider* playerCollider = nullptr;
static std::vector<Collider> enemyColliders;
static AEVec2 playerPos;
static Bullet player_defaultBullet{ 0, 450, 10, 0, 0, BULLET_FIRED_BY::PLAYER, 0 };
static Bullet boss_defaultBullet{ 0, 450, 6, 0, 0, BULLET_FIRED_BY::ENEMY, 0 };
static float enemyfirespeed = 1.0f;
static WaveManager waveManager{};
static std::chrono::steady_clock::time_point startTimer;
static std::chrono::steady_clock::time_point currenttimer;
static int min = 0, second = 0;
static bool timetest = false;
static f64 newTime = 0;
static f64 currentTime = 0;
static bool showScoreBoard = false;
static bool showPastScores = false;
static bool showVictoryScreen = false;
static Player _player{};
static ScoreBoard _scoreBoard{};
static Mouse mouse{};

// Initial player stats
static int player_damage = 10;
static int player_currentexp = 0;
static int player_currentmaxexp = 20;
static int player_currentSpeed = 250;
static float player_currentfirecd = 16.0f;
static float player_currenthp = 100.0f;
static float player_currentmaxhp = 100.0f;
static float player_tempfire_cd = player_currentfirecd;
static int enemyBulletDamage = 5;
static int playerBulletDamage = 0;

// Boundary constraints
static const float BOUNDARY_LEFT = -800.0f;
static const float BOUNDARY_RIGHT = 800.0f;
static const float BOUNDARY_TOP = 450.0f;
static const float BOUNDARY_BOTTOM = -450.0f;

void Gameplay_Load() {

    /// Create the mesh scale
    AEGfxMeshStart();

    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    /// set the above mesh scale into the rectangle mesh
    recMesh = AEGfxMeshEnd();

    /// For circle
    float radius = 0.5f;
    int numofTri_segments = 36;
    unsigned int defaultcolor = 0xFFFFFFFF;
    float center_posX = 0.0f;
    float center_posY = 0.0f;

    /// Create a circle using the segments
    AEGfxMeshStart();
    for (int i = 0; i < numofTri_segments; ++i)
    {
        /// Calculate the angle for the current and next segment
        float angle1 = (2.0f * 3.14f * i) / numofTri_segments;
        float angle2 = (2.0f * 3.14f * (i + 1)) / numofTri_segments;

        /// vertices for that particulare triangle
        float x1 = center_posX + radius * cosf(angle1);
        float y1 = center_posY + radius * sinf(angle1);
        float x2 = center_posX + radius * cosf(angle2);
        float y2 = center_posY + radius * sinf(angle2);

        AEGfxTriAdd(
            center_posX, center_posY, defaultcolor, 0.5f, 0.5f,  // Center vertex
            x1, y1, defaultcolor, 0.5f + x1, 0.5f + y1, // First outer vertex
            x2, y2, defaultcolor, 0.5f + x2, 0.5f + y2  // Second outer vertex
        );
    }

    /// set the above mesh scale to the circle mesh
    circleMesh = AEGfxMeshEnd();

    /// Triangle Mesh
    AEGfxMeshStart();

    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.0f, 0.5f, 0xFFFFFFFF, 0.5f, 0.0f);

    triMesh = AEGfxMeshEnd();

    /// Set the font 
    fontType = AEGfxCreateFont("Assets/liberation-mono.ttf", 100);

    /// Initialize weapons
    InitializeWeapons();

    /// Initialize wave manager
    initialise_wave_manager(waveManager);

    /// Initialize audio
    //Audio_Init();
}

void Gameplay_Initialize() {

    // Initialize timer
    timetest = false;

    /// set the pointer to the player struct
    player = &_player;
    playerScore = &_scoreBoard;

    /// set the pointer to the mouse struct
    user_mouse = &mouse;
    user_mouse->mouseX = user_mouse->mouseY = 0;

    option1 = &_optionBtn1;
    option2 = &_optionBtn2;
    option3 = &_optionBtn3;

    OptionBtn_Init(option1, static_cast<float>(AEGfxGetWindowWidth() - AEGfxGetWindowWidth()), AEGfxGetWindowHeight() / 4.5f);
    OptionBtn_Init(option2, static_cast<float>(AEGfxGetWindowWidth() - AEGfxGetWindowWidth()), static_cast<float>(AEGfxGetWindowHeight() - AEGfxGetWindowHeight()));
    OptionBtn_Init(option3, static_cast<float>(AEGfxGetWindowWidth() - AEGfxGetWindowWidth()), AEGfxGetWindowHeight() / (-4.5f));

    option1->vec_pos = { option1->pos_x + (AEGfxGetWindowWidth() / 2.0f), (AEGfxGetWindowHeight() / 2.0f) - option1->pos_y };
    option2->vec_pos = { option2->pos_x + (AEGfxGetWindowWidth() / 2.0f), (AEGfxGetWindowHeight() / 2.0f) - option2->pos_y };
    option3->vec_pos = { option3->pos_x + (AEGfxGetWindowWidth() / 2.0f), (AEGfxGetWindowHeight() / 2.0f) - option3->pos_y };

    /// spawn the player with its parameter when the game start
    player_setspawn(player, player_currenthp, player_currentmaxhp, player_damage,
        player_currentfirecd, player_currentexp, player_currentmaxexp, player_currentSpeed);

    /// Initialize score
    scoreBoardInit(playerScore);

    ///Creating collider for player
    AEVec2 playerPosition = { player->player_shape.pos_x, player->player_shape.pos_y };
    playerCollider = new Collider;
    *playerCollider = CreateCollider(player->player_shape.scale / 2.0f, &playerPosition, CIRCLE);

    // Clear vectors
    playerBullet.clear();
    enemyBullet.clear();
    enemyColliders.clear();
    wave.clear();

    // Reset flags
    player->scoreSaved = false;
    showScoreBoard = false;
    showPastScores = false;
    showVictoryScreen = false;

    // Start timer
    startTimer = std::chrono::steady_clock::now();
    PlayGameSound(BATTLE);
}

void Gameplay_Update() {
    currenttimer = std::chrono::steady_clock::now();

    //size_t enemyCount = wave.size();
    /// check if the player mouse is clicking on the skill menu btn
    AEInputGetCursorPosition(&user_mouse->mouseX, &user_mouse->mouseY);
    user_mouse->vec_Mouse = { static_cast<f32>(user_mouse->mouseX), static_cast<f32>(user_mouse->mouseY) };

    ///update player collider position in game loop
    ///since the player position is not created by AEVec2 but my update collider function is, I create these 2 to keep updating player position
    playerPos.x = player->player_shape.pos_x;
    playerPos.y = player->player_shape.pos_y;

    // Ensures the score board's wave cleared count is always in sync with what the wave manager knows
    playerScore->WaveCleared = waveManager.currentWave;

    //call updateCollider 
    UpdateCollider(playerCollider, playerPos, CIRCLE);
    //printf("Player collider updated to (%.2f, %.2f)\n", playerCollider->position->x, playerCollider->position->y);

    /// Timer since game started
    while (!timetest) {
        AEGetTime(&currentTime);
        timetest = true;
    }

    AEGetTime(&newTime);

    rotateX = user_mouse->mouseX - (AEGfxGetWindowWidth() / 2.0f);
    rotateY = -(user_mouse->mouseY - (AEGfxGetWindowHeight() / 2.0f));
    // std::cout << "X: " << rotateX << " Y: " << rotateY << std::endl;
    //std::cout << player->player_shape.pos_x << " " << player->player_shape.pos_y << std::endl;
    player->player_shape.Rotation = atan2f(rotateY, rotateX) - 1.5708f;

    // Restrict player movement inside the boundaries
    if (player->player_shape.pos_x < BOUNDARY_LEFT) {
        player->player_shape.pos_x = BOUNDARY_LEFT;
    }
    else if (player->player_shape.pos_x > BOUNDARY_RIGHT) {
        player->player_shape.pos_x = BOUNDARY_RIGHT;
    }

    if (player->player_shape.pos_y < BOUNDARY_BOTTOM) {
        player->player_shape.pos_y = BOUNDARY_BOTTOM;
    }
    else if (player->player_shape.pos_y > BOUNDARY_TOP) {
        player->player_shape.pos_y = BOUNDARY_TOP;
    }

    if (player->lvUp == false && player->hp > 0)
    {
        player_Input_Movement(player);

        if (player->hp > 0 && player->firing_cd > 0.0f) {
            //std::cout << "Temp: " << *player_tempfire_cd << std::endl;
            //std::cout << "player: " << player->firing_cd << std::endl;
            enemyfirespeed -= 0.05f;
            player->firing_cd -= 0.1f;
            if (player->firing_cd < 0.0f)
            {
                BulletInit(playerBullet, player_defaultBullet, player->player_shape.pos_x, player->player_shape.pos_y, player->damage_output,
                    MAX_AMMO_SIZE, BULLET_FIRED_BY::PLAYER, player->player_shape.Rotation);
                PlayGameSound(WEAPON1_SHOOT);
                player->firing_cd = player_tempfire_cd;
            }

            // Update weapon cooldowns
            UpdateWeaponCooldowns();

            // Update rifle delay between shots in bursts
            UpdateRifleBurst(playerBullet, player->player_shape.pos_x, player->player_shape.pos_y,
                player->damage_output, player->player_shape.Rotation);

            // Fire all unlocked weapons
            FireAllUnlockedWeapons(playerBullet, player->player_shape.pos_x, player->player_shape.pos_y,
                player->damage_output, player->player_shape.Rotation);

            for (int i = int(wave.size() - 1); i >= 0; i--) {
                if (enemyfirespeed < 0) {
                    if (wave[i].type == 1) {
                        enemyfirespeed = 1.5f;
                        bossrotateX = wave[i].posx - player->player_shape.pos_x;
                        bossrotateY = wave[i].posy - player->player_shape.pos_y;
                        enemyrotation = atan2f(bossrotateY, bossrotateX) - 1.5708f;

                        for (int j = 0; j < 8; ++j) {
                            float newangle = enemyrotation + (j - 4) * (PI / 4);
                            BulletInit(enemyBullet, boss_defaultBullet, wave[i].posx, wave[i].posy, enemyBulletDamage, MAX_AMMO_SIZE, BULLET_FIRED_BY::ENEMY, newangle);
                        }
                    }
                }
            }
        }
        updateWaveManager(waveManager, player->player_shape.pos_x, player->player_shape.pos_y, playerScore->WaveCleared);
        enemyai(player->player_shape.pos_x, player->player_shape.pos_y);

        if (enemyColliders.empty()) {
            enemyColliders.clear();
        }
        ///Creating Collider for enemy
        enemyColliders.resize(wave.size());
        for (int i = 0; i < wave.size(); i++) {
            AEVec2 enemyPos = { wave[i].posx, wave[i].posy };
            enemyColliders[i] = CreateCollider(wave[i].radius / 2.0f, &enemyPos, CIRCLE);
        }

        // collision check against enemy
        AEVec2 playerpos[]{ f32(player->player_shape.pos_x), f32(player->player_shape.pos_y) };
        if (player->lvUp == false)
        {
            for (int i = 0; i < wave.size(); i++) {
                ///update enemy colliders
                AEVec2 enemyPos = { wave[i].posx, wave[i].posy };
                UpdateCollider(&enemyColliders[i], enemyPos, CIRCLE);
                // printf("Enemy %d collider updated to (%.2f, %.2f)\n", i, enemyColliders[i].position->x, enemyColliders[i].position->y);

                for (auto bullet_iter = enemyBullet.begin(); bullet_iter != enemyBullet.end(); ) {
                    Bullet& bullet = *bullet_iter;

                    AEVec2 bulletPos = { bullet.bulletShape.pos_x, bullet.bulletShape.pos_y };
                    UpdateCollider(&bullet.collider, bulletPos, PROJECTILE);
                    bool bulletRemoved = false;

                    if (Collide(bullet.collider, *playerCollider)) {
                        PlayGameSound(PLAYER_HIT);
                        //printf("Bullet hit player %d at (%.2f, %.2f)!\n", i, playerCollider->position.x, playerCollider->position.y);
                        player->hp -= bullet.damage;
                        bullet_iter = enemyBullet.erase(bullet_iter); // Remove bullet
                        bulletRemoved = true;
                        break;
                    }

                    if (!bulletRemoved) {
                        ++bullet_iter; // Move to next bullet if it wasn't removed
                    }
                }

                for (auto bullet_iter = playerBullet.begin(); bullet_iter != playerBullet.end(); ) {
                    Bullet& bullet = *bullet_iter;

                    // Update bullet collider position
                    AEVec2 bulletPos = { bullet.bulletShape.pos_x, bullet.bulletShape.pos_y };
                    UpdateCollider(&bullet.collider, bulletPos, PROJECTILE);
                    //printf("bullet %d collider updated to (%.2f, %.2f)\n", i, bullet.collider.position->x, bullet.collider.position->y);
                    bool bulletRemoved = false;

                    if (Collide(bullet.collider, enemyColliders[i])) {
                        PlayGameSound(SHOOTING_HIT);
                        wave[i].hp -= bullet.damage;
                        bullet_iter = playerBullet.erase(bullet_iter); // Remove bullet
                        bulletRemoved = true;
                        break; // Bullet has hit something, exit enemy loop
                    }

                    if (!bulletRemoved) {
                        ++bullet_iter; // Move to next bullet if it wasn't removed
                    }
                }

                if (wave[i].hp <= 0) { // Remove enemy if HP reaches zero
                    player->exp += wave[i].exp;
                    playerScore->EnemyKilled++;
                    removeenemy(i);
                }

                if (Collide(*playerCollider, enemyColliders[i]) && player->hp > 0) {
                    player->hp -= 0.1f;
                }
            }
        }

        Bulletupdate(playerBullet);
        Bulletupdate(enemyBullet);
    }

    if (player->hp <= 0)
    {
        player->default_speed = 0;
        playerScore->surviveDuration.first = min;
        playerScore->surviveDuration.second = second;

        // Make sure WaveCleared is updated before calculating score
        playerScore->WaveCleared = waveManager.currentWave;

        // Calculate final score
        scoreTabulation(player->score, playerScore);

        // Save score to file when player dies (using a non-static flag or ensure it's reset)
        if (!player->scoreSaved) {  // Add this boolean to the Player struct
            SaveScoreToFile(playerScore, player);
            player->scoreSaved = true;
        }

        // Update mouse position
        AEInputGetCursorPosition(&user_mouse->mouseX, &user_mouse->mouseY);
        user_mouse->mouseX -= (AEGfxGetWindowWidth() / 2);
        user_mouse->mouseY = (AEGfxGetWindowHeight() / 2) - user_mouse->mouseY;
        user_mouse->vec_Mouse = { static_cast<f32>(user_mouse->mouseX), static_cast<f32>(user_mouse->mouseY) };

        if (!showScoreBoard && !showPastScores) {
            // Check for menu button click
            AEVec2 menu = { 0.0f, -175.0f };
            if (AETestPointToRect(&user_mouse->vec_Mouse, &menu, 300.0f, 100.0f)
                && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                next = GS_MENU;
                player->hp = 100;
                player->exp = 0;
                newTime = 0;
                player->scoreSaved = false; // Reset the flag when returning to menu
            }

            // Check for score button click
            AEVec2 scoreBtn = { 0.0f, -350.0f };
            if (AETestPointToRect(&user_mouse->vec_Mouse, &scoreBtn, 400.0f, 100.0f)
                && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                showScoreBoard = true; // Toggle to show score board
            }
        }
        else if (showScoreBoard && !showPastScores) {
            // Check for back button click
            AEVec2 backBtn = { 200.0f, -350.0f };
            if (AETestPointToRect(&user_mouse->vec_Mouse, &backBtn, 400.0f, 100.0f)
                && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                showScoreBoard = false; // Toggle back to death screen
            }

            // Handle past score button click event
            AEVec2 pastScoreBtn = { -200.0f, -350.0f };
            if (AETestPointToRect(&user_mouse->vec_Mouse, &pastScoreBtn, 400.0f, 100.0f)
                && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                showPastScores = true;
                showScoreBoard = false;
            }
        }
        else if (showPastScores) {
            // Check for back button click
            AEVec2 backBtn = { 0.0f, -350.0f };  // Centered back button
            if (AETestPointToRect(&user_mouse->vec_Mouse, &backBtn, 400.0f, 100.0f)
                && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                showPastScores = false;
                showScoreBoard = true;  // Go back to the score board
            }
        }
    }

    // If all waves are cleared, display victory screen
    if (waveManager.state == WAVE_ALL_COMPLETED && !showVictoryScreen) {
        // Victory achieved
        showVictoryScreen = true;

        // Make sure WaveCleared is updated before calculating score
        playerScore->WaveCleared = static_cast<int>(waveManager.waves.size());

        // Record player survival time for scoring
        playerScore->surviveDuration.first = min;
        playerScore->surviveDuration.second = second;

        // Calculate final score
        scoreTabulation(player->score, playerScore);

        // Save score to file (using a non-static flag or ensure it's reset)
        if (!player->scoreSaved) {  // Add this boolean to the Player struct
            SaveScoreToFile(playerScore, player);
            player->scoreSaved = true;
        }
    }

    // Handle victory screen interactions - add this where you handle other UI interactions
    if (showVictoryScreen) {
        // Update mouse position
        AEInputGetCursorPosition(&user_mouse->mouseX, &user_mouse->mouseY);
        user_mouse->mouseX -= (AEGfxGetWindowWidth() / 2);
        user_mouse->mouseY = (AEGfxGetWindowHeight() / 2) - user_mouse->mouseY;
        user_mouse->vec_Mouse = { static_cast<f32>(user_mouse->mouseX), static_cast<f32>(user_mouse->mouseY) };

        if (!showScoreBoard && !showPastScores) {
            // Check for menu button click
            AEVec2 menu = { 0.0f, -175.0f };
            if (AETestPointToRect(&user_mouse->vec_Mouse, &menu, 300.0f, 100.0f)
                && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                next = GS_MENU;
                player->hp = 100;
                player->exp = 0;
                newTime = 0;
                showVictoryScreen = false;  // Reset the flag when returning to menu
            }

            // Check for score button click
            AEVec2 scoreBtn = { 0.0f, -350.0f };
            if (AETestPointToRect(&user_mouse->vec_Mouse, &scoreBtn, 400.0f, 100.0f)
                && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                showScoreBoard = true; // Toggle to show score board
            }
        }
        else if (showScoreBoard && !showPastScores) {
            // Check for back button click
            AEVec2 backBtn = { 200.0f, -350.0f };
            if (AETestPointToRect(&user_mouse->vec_Mouse, &backBtn, 400.0f, 100.0f)
                && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                showScoreBoard = false; // Toggle back to victory screen
            }

            // Handle past score button click event
            AEVec2 pastScoreBtn = { -200.0f, -350.0f };
            if (AETestPointToRect(&user_mouse->vec_Mouse, &pastScoreBtn, 400.0f, 100.0f)
                && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                showPastScores = true;
                showScoreBoard = false;
            }
        }
        else if (showPastScores) {
            // Check for back button click
            AEVec2 backBtn = { 0.0f, -350.0f };  // Centered back button
            if (AETestPointToRect(&user_mouse->vec_Mouse, &backBtn, 400.0f, 100.0f)
                && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                showPastScores = false;
                showScoreBoard = true;  // Go back to the score board
            }
        }
    }

    /// Once the player gain enough exp to lv up
    if (player->exp >= player->max_exp && !player->lvUp) {
        /*player->movement_speed = 0; */
        RandomSkillWeaponGenerator(itemDB, skillWeapon_txt, displayskillWeapon_txt);
        player->lvUp = true;
        PlayGameSound(LEVELUP);
    }

    if (player->lvUp == true)
    {
        if (AETestPointToRect(&user_mouse->vec_Mouse, &option1->vec_pos, AEGfxGetWindowWidth() / 2.2f, AEGfxGetWindowHeight() / 10.0f)
            && AEInputCheckTriggered(AEVK_LBUTTON)) {
            PlayerSelectionOption(itemDB, player, player_tempfire_cd, 0);
        }

        if (AETestPointToRect(&user_mouse->vec_Mouse, &option2->vec_pos, AEGfxGetWindowWidth() / 2.2f, AEGfxGetWindowHeight() / 10.0f)
            && AEInputCheckTriggered(AEVK_LBUTTON)) {
            PlayerSelectionOption(itemDB, player, player_tempfire_cd, 1);
        }

        if (AETestPointToRect(&user_mouse->vec_Mouse, &option3->vec_pos, AEGfxGetWindowWidth() / 2.2f, AEGfxGetWindowHeight() / 10.0f)
            && AEInputCheckTriggered(AEVK_LBUTTON)) {
            PlayerSelectionOption(itemDB, player, player_tempfire_cd, 2);
        }
    }

    // mouse coords
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    AEGfxSetCamPosition(player->player_shape.pos_x, player->player_shape.pos_y);

#pragma region Cheat code
    if (player->hp < player->max_hp) { if (AEInputCheckCurr(AEVK_Z)) player->hp += 2; }
    if (player->hp > player->max_hp || player->hp > 0) { if (AEInputCheckCurr(AEVK_X)) player->hp -= 7; }
    if (player->exp != player->max_exp) { if (AEInputCheckCurr(AEVK_C)) player->exp += 1; }
    if (AEInputCheckTriggered(AEVK_B))
    {
        player->exp -= player->max_exp;
        player->max_exp += 15;
        player->lvUp = false;
    }
#pragma endregion

    // Check for escape key to exit
    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        next = GS_MENU;
    }
}

void Gameplay_Draw() {
    // First set camera to follow player for game elements
    AEGfxSetCamPosition(player->player_shape.pos_x, player->player_shape.pos_y);

    /// Set the game background 
    AEGfxSetBackgroundColor(0.6f, 0.6f, 0.6f);

    /// set the render for the game
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    /// Set the blend color for the game
    AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Draw boundary walls
    Left_Boundary(recMesh);
    Right_Boundary(recMesh);
    Top_Boundary(recMesh);
    Bottom_Boundary(recMesh);

    // spawn enemy
    enemyspawn();

    /// Spawn the player at the intialise position
    draw_player(player, circleMesh);
    draw_player_aimIcon(player, triMesh);

    // Draw bullets
    DrawBullet(playerBullet, player, circleMesh);
    DrawBullet(enemyBullet, player, circleMesh);

    // Now reset camera for UI elements
    //AEGfxSetCamPosition(0, 0);

    /// Draw the exp bar
    draw_exp(player, recMesh);

    draw_hp(player, recMesh, fontType);

    char hp_text[10];
    sprintf_s(hp_text, "%d / %d", static_cast<int>(player->hp), static_cast<int>(player->max_hp));
    AEGfxPrint(fontType, hp_text, -0.8f, 0.865f, 0.4f, 1.0f, 1.0f, 1.0f, 1.0f);

    DrawWaveStatus(waveManager, fontType);

    char timer[100];
    // timer
    if (player->hp > 0)
    {
        long long timeElasped = (currenttimer - startTimer).count();
        int elapsed_seconds = static_cast<int>(timeElasped / 1000000000);
        min = elapsed_seconds / 60;
        second = elapsed_seconds % 60;

        sprintf_s(timer, "Timer: %i:%02i", min, second);
        AEGfxPrint(fontType, timer, 0.725f, 0.88f, 0.3f, 1.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        sprintf_s(timer, "Timer: --:--");
        AEGfxPrint(fontType, timer, 0.725f, 0.88f, 0.3f, 1.0f, 0.0f, 0.0f, 1.0f);
    }

    // Draw death screen and score displays with fixed camera
    if (player->hp <= 0) {
        if (!showScoreBoard && !showPastScores) {
            // Display death screen
            Death_ScreenDisplay(player, recMesh, fontType);
        }
        else if (showScoreBoard && !showPastScores) {
            Death_ScreenDisplay(player, recMesh, fontType);
            ScoreBoard_Display(player, playerScore, recMesh, fontType);
        }
        else if (showPastScores) {
            Death_ScreenDisplay(player, recMesh, fontType);
            PastScore_Display(player, recMesh, fontType);
        }
    }

    // Draw victory screen with fixed camera if all waves are completed
    if (showVictoryScreen) {
        if (!showScoreBoard && !showPastScores) {
            // Display victory screen
            Victory_ScreenDisplay(player, recMesh, fontType);
        }
        else if (showScoreBoard && !showPastScores) {
            Victory_ScreenDisplay(player, recMesh, fontType);
            ScoreBoard_Display(player, playerScore, recMesh, fontType);
        }
        else if (showPastScores) {
            Victory_ScreenDisplay(player, recMesh, fontType);
            PastScore_Display(player, recMesh, fontType);
        }
    }

    // Draw skill menu with fixed camera
    if (player->lvUp == true)
    {
        skillweaponmenu_Display(player, recMesh, fontType, displayskillWeapon_txt);
    }

    // Reset camera to follow player at the end
    AEGfxSetCamPosition(player->player_shape.pos_x, player->player_shape.pos_y);
}

void Gameplay_Free() {
    // Free temporary resources for the current game session
    enemyColliders.clear();
}

void Gameplay_Unload() {

    // Clean up all resources
    showVictoryScreen = false;
    delete playerCollider;
    playerCollider = nullptr;

    enemyColliders.clear();
    playerBullet.clear();
    enemyBullet.clear();
    itemDB.clear();
    skillWeapon_txt.clear();
    displayskillWeapon_txt.clear();
    wave.clear();

    resetWaveManager(waveManager);

    if (recMesh) {
        AEGfxMeshFree(recMesh);
        recMesh = nullptr;
    }

    if (circleMesh) {
        AEGfxMeshFree(circleMesh);
        circleMesh = nullptr;
    }

    if (triMesh) {
        AEGfxMeshFree(triMesh);
        triMesh = nullptr;
    }

    if (fontType >= 0) {
        AEGfxDestroyFont(fontType);
        fontType = -1;
    }
}