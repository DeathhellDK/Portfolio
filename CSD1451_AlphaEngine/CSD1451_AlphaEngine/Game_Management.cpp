/* Start Header ************************************************************************/
/*!
\file		Game_Management.cpp
\author		Tan Wei Liang Terril (85%), Zhi Jie, Tingkai, Hao Peng
\date		March, 20, 2025
\brief		File contains the implementation of functions to handle the following:
			- Initialise the scoreboard
			- Tabulation the player current gameplay score
			- Handle how the player experience (exp) will be drawn
			- Handle how the player health (hp) will be drawn
			- Handle how the player move
			- Add bullet into the bullet vector
			- Update the bullet on it position and rotation
			- Display game over when player life is drop to 0
			- Function to display past scores
			- Function to save the current score to file
			- Function to load past scores (vector) from file
			
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "Game_Management.h"
#include "weapon.h"
#include "collision.h"
#include "enemy.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>

/// <summary>
///  Initialise the scoreboard value to a default value
/// </summary>
/// <param name="score"> Retrieve score data </param>
/// <returns></returns>
ScoreBoard *scoreBoardInit(ScoreBoard* score) {
	score->EnemyKilled = 0; // set as the dafault value
	score->surviveDuration.first = 0; // set as the dafault value
	score->surviveDuration.second = 0; // set as the dafault value
	score->WaveCleared = 0; // set as the dafault value
	score->survivalBonus = 0; // set as the dafault value

	return score; // return the score value
}

/// <summary>
/// Tabulation the player current gameplay score
/// </summary>
/// <param name="playerScore"> save into the player score which will use to display the respecitve score component </param>
/// <param name="score"> Retrieve the score struct data </param>
/// <returns></returns>
int scoreTabulation(int &playerScore, ScoreBoard* score) {
	// Check if the min is even, set the survial bonus score base off the even duration value
	if (score->surviveDuration.first % 2 == 0) { score->survivalBonus = (score->surviveDuration.first / 2) * 2; }

	// tabulate the total score base on the enemy kill, how many wave they clear and the survival bonus
	playerScore = (score->EnemyKilled * score->WaveCleared) + score->survivalBonus; 

	return playerScore; // return the player score
}

/// <summary>
/// Handle how the player experience (exp) will be drawn
/// </summary>
/// <param name="player"> Retreive the player data </param>
/// <param name="mesh"> Retreive the mesh data that will be use to draw the exp </param>
void draw_exp(Player* player, AEGfxVertexList* mesh)
{
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

	/// Exp Bar: Background of the exp 
	#pragma region ExpBar
	AEMtx33 Expbar_transform, Expbar_angle, Expbar_scale, Expbar_translate;
	AEMtx33Identity(&Expbar_transform);
	AEMtx33Rot(&Expbar_angle, 0.0f);
	AEMtx33Scale(&Expbar_scale, (float)AEGfxGetWindowWidth(), AEGfxGetWindowHeight() / 60.0f);
	AEMtx33Trans(&Expbar_translate, player->player_shape.pos_x, player->player_shape.pos_y + (AEGfxGetWindowHeight() / 2.04f));

	AEMtx33Concat(&Expbar_transform, &Expbar_angle, &Expbar_scale);
	AEMtx33Concat(&Expbar_transform, &Expbar_translate, &Expbar_transform);
	AEGfxSetTransform(Expbar_transform.m);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	#pragma endregion

	/// Exp Bar: player exp gain from killing enemy
	#pragma region ExpBar(Player)

	// The maximum exp that the player can currently obtain from killing the enemy
	float obtain = (float)(player->exp - 0) / (player->max_exp - 0);

	// Color of the exp bar changes upon reaching a cetrain threshold
	if (obtain >= 0 && obtain < 0.5) AEGfxSetColorToAdd(0.0f, 1.0f, 0.8f, 1.0f);
	else if (obtain >= 0.5 && obtain <= 0.7) AEGfxSetColorToAdd(0.0f, 0.9f, 1.0f, 1.0f);
	else AEGfxSetColorToAdd(0.0f, 0.0f, 1.0f, 1.0f);

	AEMtx33 exp_transform, exp_angle, exp_scale, exp_translate;

	AEMtx33Identity(&exp_transform);
	AEMtx33Rot(&exp_angle, 0.0f);
	AEMtx33Scale(&exp_scale, (AEGfxGetWindowWidth()) * obtain, AEGfxGetWindowHeight() / 60.0f);

	float levelingBar = AEGfxGetWindowWidth() * (1.0f + obtain) * 0.5f; // a bar represent the amount of exp that the player cuurently have
	AEMtx33Trans(&exp_translate, player->player_shape.pos_x + ((AEGfxGetWindowWidth() * -1.0f) + levelingBar), player->player_shape.pos_y 
		+ (AEGfxGetWindowHeight() / 2.04f)); // Draw the current exp of the player

	AEMtx33Concat(&exp_transform, &exp_angle, &exp_scale);
	AEMtx33Concat(&exp_transform, &exp_translate, &exp_transform);
	AEGfxSetTransform(exp_transform.m);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	#pragma endregion
}

/// <summary>
/// Handle how the player health (hp) will be drawn
/// </summary>
/// <param name="player"> Retreive the player data </param>
/// <param name="mesh"> Retreive the mesh data that will be use to draw the hp</param>
/// <param name="font_id"> Retrieve the font id that will be use for the text </param>
void draw_hp(Player* player, AEGfxVertexList* mesh, s8 font_id)
{
	// Display the hp text
	AEGfxPrint(font_id, "HP", -0.98f, 0.86f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);

	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

	//Hp Bar: Background of the hp
	#pragma region Hpbar
	AEMtx33 healthbarmeter_transform, healthbarmeter_angle, healthbarmeter_scale, healthbarmeter_translate;
	AEMtx33Identity(&healthbarmeter_transform);
	AEMtx33Rot(&healthbarmeter_angle, 0.0f);
	AEMtx33Scale(&healthbarmeter_scale, AEGfxGetWindowWidth() / 5.0f, AEGfxGetWindowHeight() / 20.0f);
	AEMtx33Trans(&healthbarmeter_translate, player->player_shape.pos_x + ((AEGfxGetWindowWidth() * -1.0f) / 3.0f),
		player->player_shape.pos_y + (AEGfxGetWindowHeight() / 2.25f));

	AEMtx33Concat(&healthbarmeter_transform, &healthbarmeter_angle, &healthbarmeter_scale);
	AEMtx33Concat(&healthbarmeter_transform, &healthbarmeter_translate, &healthbarmeter_transform);
	AEGfxSetTransform(healthbarmeter_transform.m);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	#pragma endregion

	//Hp Bar: Player hp bar
	#pragma region Hpbar(player)
	float healthRemain = (float)(player->hp - 0) / (player->max_hp - 0); // The amount of health the player left

	// Color of the health bar changes upon reaching a cetrain threshold
	if (healthRemain > 0.6) AEGfxSetColorToAdd(0.02f, 0.67f, 0.22f, 1.0f);
	else if (healthRemain > 0.4 && healthRemain <= 0.6) AEGfxSetColorToAdd(0.7f, 0.7f, 0.04f, 1.0f);
	else AEGfxSetColorToAdd(1.0f, 0.0f, 0.0f, 1.0f);

	AEMtx33 healthblood_transform, healthblood_angle, healthblood_scale, healthblood_translate;
	AEMtx33Identity(&healthblood_transform);
	AEMtx33Rot(&healthblood_angle, 0.0f);
	AEMtx33Scale(&healthblood_scale, (AEGfxGetWindowWidth() / 5.0f) * healthRemain, AEGfxGetWindowHeight() / 20.0f);

	float healthloss = (AEGfxGetWindowWidth() / 5.0f) * (1.0f - healthRemain) * 0.5f; // offset base on how much health the player have left
	AEMtx33Trans(&healthblood_translate, player->player_shape.pos_x + (((AEGfxGetWindowWidth() * -1.0f) / 3.0f) - healthloss),
		player->player_shape.pos_y + (AEGfxGetWindowHeight() / 2.25f)); // Draw the remaining health of the player

	AEMtx33Concat(&healthblood_transform, &healthblood_angle, &healthblood_scale);
	AEMtx33Concat(&healthblood_transform, &healthblood_translate, &healthblood_transform);
	AEGfxSetTransform(healthblood_transform.m);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
#pragma endregion
}

/// <summary>
/// Handle how the player move 
/// </summary>
/// <param name="player"> Retreive the player struct </param>
void player_Input_Movement(Player *player) {
	float move_x = 0.0f;
	float move_y = 0.0f;

	// Check input and accumulate movement
	if (AEInputCheckCurr(AEVK_W)) move_y += player->movement_speed; // move forward
	if (AEInputCheckCurr(AEVK_S)) move_y -= player->movement_speed; // move backward
	if (AEInputCheckCurr(AEVK_A)) move_x -= player->movement_speed; // move left
	if (AEInputCheckCurr(AEVK_D)) move_x += player->movement_speed; // move right

	// Normalize the movement vector
	float length = sqrtf(move_x * move_x + move_y * move_y);
	if (length > 0.0f) {
		move_x /= length;
		move_y /= length;
	}

	// Apply movement speed
	player->player_shape.pos_x += move_x * player->movement_speed;
	player->player_shape.pos_y += move_y * player->movement_speed;

	//if (AEInputCheckCurr(AEVK_W)) player->player_shape.pos_y += player->movement_speed; // move forward
	//if (AEInputCheckCurr(AEVK_S)) player->player_shape.pos_y -= player->movement_speed; // move backward
	//if (AEInputCheckCurr(AEVK_A)) player->player_shape.pos_x -= player->movement_speed; // move right
	//if (AEInputCheckCurr(AEVK_D)) player->player_shape.pos_x += player->movement_speed; // move left
}

/// <summary>
/// Add bullet into the bullet 
/// </summary>
/// <param name="BulletVectorID"> Retreive the bullet vector array </param>
/// <param name="bullet"> Retrieve the bullet data which will be store into a constructor </param>
/// <param name="posX"> the current position of x where the bullet will fire from </param>
/// <param name="posY"> the current position of y where the bullet will fire from </param>
/// <param name="MAX_AMMO"> the maximum threshold that the bullet can store until 
/// (use to maintain the size of the vector array so that it does not grow too big) </param>
/// <param name="id"> The bullet is fired by which object </param>
/// <param name="userRoation"> Retrieve the rotation of which object is using that particluar bullet </param>
void BulletInit(std::vector<Bullet>& BulletVectorID, Bullet bullet, float posX, float posY, int& damageOutput,const size_t MAX_AMMO, BULLET_FIRED_BY id, float userRoation) {

	float bulletRotation = userRoation; // set a tempo rotation base on the user rotation for each bullet
	if (id == BULLET_FIRED_BY::PLAYER) { // check if the bullet is fired by the player
		bullet.damage = damageOutput; // apply player damage to the bullet instead
	}
	else if (id == BULLET_FIRED_BY::ENEMY) {
		bullet.damage = damageOutput;
	}

	// checks if the current bullet vector array is more than the limit threshold (exitsing bullet could be still in the vector array),
	// if it is more or equal than the threshold, the vector will start to remove exitising bullet in its vector array.
	// This is to manage the vector array so that it does not go out of control as this is a dynamic array.
	if (BulletVectorID.size() >= MAX_AMMO) BulletVectorID.erase(BulletVectorID.begin()); 
	BulletVectorID.push_back(Bullet(bullet.damage,bullet.speed, bullet.lifespan, posX, posY, id, bulletRotation)); // add the new bullet into the vector array

	// ADDITIONAL CODE: Assign collider after the bullet is added
	Bullet& newBullet = BulletVectorID.back(); // Get reference to the last bullet added
	float bulletRadius = newBullet.bulletShape.scale / 2.0f;
	// Create a circle collider for the bullet
	AEVec2 bulletPos = { posX, posY };
	newBullet.collider = CreateCollider(bulletRadius, &bulletPos, PROJECTILE); 
}

/// <summary>
/// Update the bullet on it position and rotation
/// </summary>
/// <param name="BulletVectorID"> Retreive the bullet vector array </param>
/// <param name="enemyColliders"> Retreive the enemy collider</param>
/// <param name="enemyCount"> Retreive the amount of enemy </param>
/// <param name="playerCollider"> Retreive the player collidr </param>
/// <param name="isPlayerBullet"> Retreive the mesh data that will be use to draw the hp </param>
void Bulletupdate(std::vector<Bullet>& BulletVectorID) {

	std::vector<std::vector<Bullet>::iterator> removeBulletvec; //Declare a iterator which pointing to each bullet object in bullet vector

	// Loop through the bulletvector array until it reach the end of it
	for (auto bullet_vec = BulletVectorID.begin(); bullet_vec != BulletVectorID.end();++bullet_vec)
	{
		Bullet& bullet = *bullet_vec; // dereference the iteratator to get a reference to the bullet object

		if (bullet.id == BULLET_FIRED_BY::PLAYER) { // if the bullet is belong to the player
			float bullet_directionX = cosf(bullet.bulletShape.Rotation + 1.5709f); // calculate the x component of the bullet direction base on it rotation
			float bullet_directionY = sinf(bullet.bulletShape.Rotation + 1.5709f); // calculate the y component of the bullet direction base on it rotation 

			// update x position of the bullet base on its speed and direction
			bullet.bulletShape.pos_x += bullet_directionX * bullet.speed * static_cast<float>(AEFrameRateControllerGetFrameTime());

			// update y position of the bullet base on its speed and direction
			bullet.bulletShape.pos_y += bullet_directionY * bullet.speed * static_cast<float>(AEFrameRateControllerGetFrameTime());
			AEVec2 bulletPos = { bullet.bulletShape.pos_x, bullet.bulletShape.pos_y };
			UpdateCollider(&bullet.collider, bulletPos, PROJECTILE);
		}
		else if (bullet.id == BULLET_FIRED_BY::ENEMY) { // if the bullet is belong to the enemy
			// put the code on how you want the bullet to move when enemy using it
			float enemy_bullet_directionX = cosf(bullet.bulletShape.Rotation);
			float enemy_bullet_directionY = sinf(bullet.bulletShape.Rotation);

			bullet.bulletShape.pos_x +=   enemy_bullet_directionX * bullet.speed * static_cast<float>(AEFrameRateControllerGetFrameTime());
			bullet.bulletShape.pos_y +=  enemy_bullet_directionY * bullet.speed * static_cast<float>(AEFrameRateControllerGetFrameTime());

			AEVec2 bulletPos = { bullet.bulletShape.pos_x, bullet.bulletShape.pos_y };
			UpdateCollider(&bullet.collider, bulletPos, PROJECTILE);
		}

		bullet.lifespan-=0.1f; // decrease the bullet lifespan upon fire 
		// bullet will be remove when its life reach 0, remove the bullet from the vector
		if (bullet.lifespan <= 0) { removeBulletvec.push_back(bullet_vec); }
	}

	// Remove the bullet from the intial vector
	for (auto bulletRemover = removeBulletvec.rbegin(); bulletRemover != removeBulletvec.rend(); ++bulletRemover) { BulletVectorID.erase(*bulletRemover); }
}

/// <summary>
/// how the will be drawn base on its translate, rotation and scale
/// </summary>
/// <param name="BulletVectorID"> Retreive the bullet vector array </param>
/// <param name="player"> Retreive the player data </param>
/// <param name="mesh"></param>
void DrawBullet(std::vector<Bullet>& BulletVectorID, Player* player, AEGfxVertexList* mesh)
{
	// check if the bullet is empty, if so, exit the function
	if (BulletVectorID.empty()) { return; }

	// lopp through each of the bullet vector array
	for (auto& bullet : BulletVectorID) {
		if (bullet.id == BULLET_FIRED_BY::PLAYER)
		{
			AEGfxSetColorToAdd(1.0f, 0.9f, 0.4f, 1.0f);
		}
		if (bullet.id == BULLET_FIRED_BY::ENEMY)
		{
			AEGfxSetColorToAdd(1.0f, 0.9f, 0.4f, 1.0f);
		}
		
		AEMtx33 bullet_transform, bullet_roatate, bullet_scale, bullet_translate;

		AEMtx33Trans(&bullet_translate, bullet.bulletShape.pos_x, bullet.bulletShape.pos_y);
		AEMtx33Rot(&bullet_roatate, bullet.bulletShape.Rotation);
		AEMtx33Scale(&bullet_scale, player->player_shape.scale / 2.0f, player->player_shape.scale / 2.0f);

		AEMtx33Concat(&bullet_transform, &bullet_scale, &bullet_roatate);
		AEMtx33Concat(&bullet_transform, &bullet_translate, &bullet_transform);

		AEGfxSetTransform(bullet_transform.m);
		AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	}
}

/// <summary>
/// Display game over when player life is drop to 0 
/// </summary>
/// <param name="player"> Retrieve the player data </param>
/// <param name="score"> Retrieve the the score data </param>
/// <param name="mesh"> Retrieve the mesh data </param>
/// <param name="font_id"> Retrieve the font id  data </param>
void Death_ScreenDisplay(Player* player, AEGfxVertexList* mesh, s8 font_id) {
	
	AEGfxSetColorToAdd(1.0f, 0.0f, 0.0f, 1.0f);

	AEMtx33 Gameover_transform, Gameover_angle, Gameover_scale, Gameover_translate;

	#pragma region GameOver display
	AEMtx33Identity(&Gameover_transform);
	AEMtx33Rot(&Gameover_angle, 0.0f);
	AEMtx33Scale(&Gameover_scale, (f32)AEGfxGetWindowWidth(), (f32)AEGfxGetWindowHeight());
	AEMtx33Trans(&Gameover_translate, player->player_shape.pos_x, player->player_shape.pos_y);

	AEMtx33Concat(&Gameover_transform, &Gameover_angle, &Gameover_scale);
	AEMtx33Concat(&Gameover_transform, &Gameover_translate, &Gameover_transform);

	AEGfxSetTransform(Gameover_transform.m);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

	AEGfxPrint(font_id, "GAME OVER", -1.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	#pragma endregion

	#pragma region MENU
	AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
	AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

	AEMtx33 MENU_transform;
	AEMtx33Identity(&MENU_transform);

	AEMtx33 MENU_spin;
	AEMtx33Rot(&MENU_spin, 0.0f);

	AEMtx33 MENU_scale;
	AEMtx33Scale(&MENU_scale, 300.0f, 100.0f);

	AEMtx33 MENU_translate;
	AEMtx33Trans(&MENU_translate, player->player_shape.pos_x, (player->player_shape.pos_y) - 175.0f);

	AEMtx33Concat(&MENU_transform, &MENU_spin, &MENU_scale);
	AEMtx33Concat(&MENU_transform, &MENU_translate, &MENU_transform);

	AEGfxSetTransform(MENU_transform.m);

	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

	AEGfxPrint(font_id, "MENU", -0.045f, -0.40f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);
	#pragma endregion

	#pragma region score
	//AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
	//AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

	AEMtx33 Score_transform;
	AEMtx33Identity(&Score_transform);

	AEMtx33 score_spin;
	AEMtx33Rot(&score_spin, 0.0f);

	AEMtx33 score_scale;
	AEMtx33Scale(&score_scale, 300.0f, 100.0f);

	AEMtx33 score_translate;
	AEMtx33Trans(&score_translate, player->player_shape.pos_x, (player->player_shape.pos_y) - 350.0f);

	AEMtx33Concat(&Score_transform, &score_spin, &score_scale);
	AEMtx33Concat(&Score_transform, &score_translate, &Score_transform);

	AEGfxSetTransform(Score_transform.m);

	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

	AEGfxPrint(font_id, "SCORE", -0.045f, -0.80f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);

}

void ScoreBoard_Display(Player* player, ScoreBoard* score, AEGfxVertexList* Mesh, s8 font_id)
{
#pragma region ScoreBoard_Display
	AEMtx33 ScoreMenu_transform, ScoreMenu_angle, ScoreMenu_scale, ScoreMenu_translate;

	// Display background panel
	AEGfxSetColorToAdd(0.5f, 0.5f, 0.8f, 1.0f);
	AEMtx33Identity(&ScoreMenu_transform);
	AEMtx33Rot(&ScoreMenu_angle, 0.0f);
	AEMtx33Scale(&ScoreMenu_scale, AEGfxGetWindowWidth() / 2.0f, AEGfxGetWindowHeight() / 1.5f);
	AEMtx33Trans(&ScoreMenu_translate, player->player_shape.pos_x, player->player_shape.pos_y);
	AEMtx33Concat(&ScoreMenu_transform, &ScoreMenu_angle, &ScoreMenu_scale);
	AEMtx33Concat(&ScoreMenu_transform, &ScoreMenu_translate, &ScoreMenu_transform);
	AEGfxSetTransform(ScoreMenu_transform.m);
	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);

	// Score title
	AEGfxPrint(font_id, "GAME STATISTICS", -0.25f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f);

	// Display score components
	char enemiesKilled[50], wavesCleared[50], survivalTime[50], survivalBonus[50], totalScore[50];

	sprintf_s(enemiesKilled, "Enemies Killed: %d", score->EnemyKilled);
	sprintf_s(wavesCleared, "Waves Cleared: %d", score->WaveCleared);
	sprintf_s(survivalTime, "Survival Time: %d min %d sec", score->surviveDuration.first, score->surviveDuration.second);
	sprintf_s(survivalBonus, "Survival Bonus: %d", score->survivalBonus);
	sprintf_s(totalScore, "TOTAL SCORE: %d", player->score);

	AEGfxPrint(font_id, enemiesKilled, -0.4f, 0.3f, 0.35f, 1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxPrint(font_id, wavesCleared, -0.4f, 0.1f, 0.35f, 1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxPrint(font_id, survivalTime, -0.4f, -0.1f, 0.35f, 1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxPrint(font_id, survivalBonus, -0.4f, -0.3f, 0.35f, 1.0f, 1.0f, 1.0f, 1.0f);

	// Total score (highlighted)
	AEGfxPrint(font_id, totalScore, -0.3f, -0.5f, 0.5f, 1.0f, 0.8f, 0.0f, 1.0f);

	// Back button
	AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);
	AEMtx33 BackBtn_transform, BackBtn_angle, BackBtn_scale, BackBtn_translate;
	AEMtx33Identity(&BackBtn_transform);
	AEMtx33Rot(&BackBtn_angle, 0.0f);
	AEMtx33Scale(&BackBtn_scale, 400.0f, 100.0f);
	AEMtx33Trans(&BackBtn_translate, player->player_shape.pos_x + 200.0f, player->player_shape.pos_y - 350.0f);
	AEMtx33Concat(&BackBtn_transform, &BackBtn_angle, &BackBtn_scale);
	AEMtx33Concat(&BackBtn_transform, &BackBtn_translate, &BackBtn_transform);
	AEGfxSetTransform(BackBtn_transform.m);
	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
	AEGfxPrint(font_id, "BACK", 0.20f, -0.80f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);

	// Past Score button
	AEGfxSetColorToAdd(0.2f, 0.8f, 1.0f, 1.0f);
	AEMtx33 PastScoreBtn_transform, PastScoreBtn_angle, PastScoreBtn_scale, PastScoreBtn_translate;
	AEMtx33Identity(&PastScoreBtn_transform);
	AEMtx33Rot(&PastScoreBtn_angle, 0.0f);
	AEMtx33Scale(&PastScoreBtn_scale, 400.0f, 100.0f);
	AEMtx33Trans(&PastScoreBtn_translate, player->player_shape.pos_x - 200.0f, player->player_shape.pos_y - 350.0f);
	AEMtx33Concat(&PastScoreBtn_transform, &PastScoreBtn_angle, &PastScoreBtn_scale);
	AEMtx33Concat(&PastScoreBtn_transform, &PastScoreBtn_translate, &PastScoreBtn_transform);
	AEGfxSetTransform(PastScoreBtn_transform.m);
	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
	AEGfxPrint(font_id, "PAST SCORE", -0.35f, -0.80f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);
#pragma endregion
}

// Function to escape special characters in JSON strings
std::string EscapeJSONString(const std::string& input) {
	std::ostringstream ss;
	for (auto c : input) {
		switch (c) {
		case '\"': ss << "\\\""; break;
		case '\\': ss << "\\\\"; break;
		case '\b': ss << "\\b"; break;
		case '\f': ss << "\\f"; break;
		case '\n': ss << "\\n"; break;
		case '\r': ss << "\\r"; break;
		case '\t': ss << "\\t"; break;
		default: ss << c;
		}
	}
	return ss.str();
}

void SaveScoreToFile(ScoreBoard* score, Player* player) {
	// Create a string for the new score entry
	std::ostringstream newScore;

	// Get current time with safer localtime_s
	time_t now = time(nullptr);
	struct tm timeinfo;
	localtime_s(&timeinfo, &now);
	char timestamp[20];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M", &timeinfo);

	// Load existing scores
	std::vector<std::string> existingScores;
	std::ifstream inFile("past_scores.json");

	// Read existing scores if the file exists
	if (inFile.is_open()) {
		std::string line;
		while (std::getline(inFile, line)) {
			// Skip the array brackets and any whitespace-only lines
			line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
			if (line != "[" && line != "]" && line != "," && !line.empty()) {
				// Remove trailing commas if present
				if (line.back() == ',') {
					line.pop_back();
				}
				existingScores.push_back(line);
			}
		}
		inFile.close();
	}

	// Add the new score as a JSON object
	newScore << "{";
	newScore << "\"timestamp\":\"" << timestamp << "\",";
	newScore << "\"score\":" << player->score << ",";
	newScore << "\"kills\":" << score->EnemyKilled << ",";
	newScore << "\"waves\":" << score->WaveCleared << ",";
	newScore << "\"duration\":\"" << score->surviveDuration.first << ":" << score->surviveDuration.second << "\",";
	newScore << "\"bonus\":" << score->survivalBonus;
	newScore << "}";

	// Insert the new score at the beginning (most recent)
	existingScores.insert(existingScores.begin(), newScore.str());

	// Keep only MAX_PAST_SCORES
	while (existingScores.size() > MAX_PAST_SCORES) {
		existingScores.pop_back();
	}

	// Write all scores back to file in JSON format
	std::ofstream outFile("past_scores.json", std::ios::out | std::ios::trunc);

	if (outFile.is_open()) {
		outFile << "[" << std::endl;

		for (size_t i = 0; i < existingScores.size(); ++i) {
			outFile << "  " << existingScores[i];

			// Add comma after all but the last entry
			if (i < existingScores.size() - 1) {
				outFile << ",";
			}

			outFile << std::endl;
		}

		outFile << "]" << std::endl;
		outFile.close();

		// Debug message - you can comment this out in final version
		printf("Score saved successfully!\n");
	}
	else {
		printf("Error: Could not open file for writing.\n");
	}
}

std::vector<std::string> LoadPastScores() {
	std::vector<std::string> formattedScores;
	std::ifstream inFile("past_scores.json");

	if (!inFile.is_open()) {
		printf("No past scores file found or could not be opened.\n");
		return formattedScores;
	}

	// Read the JSON file line by line
	std::vector<std::string> jsonObjects;
	std::string line;
	while (std::getline(inFile, line)) {
		// Skip array brackets and commas
		line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
		if (line != "[" && line != "]" && line != "," && !line.empty()) {
			// Remove trailing commas
			if (line.back() == ',') {
				line.pop_back();
			}
			jsonObjects.push_back(line);
		}
	}
	inFile.close();

	// Process each JSON object
	for (const auto& jsonObj : jsonObjects) {
		// Simple JSON parsing - extract each field
		std::string timestamp, score, kills, waves, duration, bonus;

		// Extract timestamp
		size_t timestampPos = jsonObj.find("\"timestamp\"");
		if (timestampPos != std::string::npos) {
			size_t start = jsonObj.find(":", timestampPos) + 1;
			size_t end = jsonObj.find(",", start);
			if (end == std::string::npos) end = jsonObj.find("}", start);

			timestamp = jsonObj.substr(start, end - start);
			// Remove quotes
			timestamp.erase(std::remove(timestamp.begin(), timestamp.end(), '\"'), timestamp.end());
		}

		// Extract score
		size_t scorePos = jsonObj.find("\"score\"");
		if (scorePos != std::string::npos) {
			size_t start = jsonObj.find(":", scorePos) + 1;
			size_t end = jsonObj.find(",", start);
			if (end == std::string::npos) end = jsonObj.find("}", start);

			score = jsonObj.substr(start, end - start);
		}

		// Extract kills
		size_t killsPos = jsonObj.find("\"kills\"");
		if (killsPos != std::string::npos) {
			size_t start = jsonObj.find(":", killsPos) + 1;
			size_t end = jsonObj.find(",", start);
			if (end == std::string::npos) end = jsonObj.find("}", start);

			kills = jsonObj.substr(start, end - start);
		}

		// Extract waves
		size_t wavesPos = jsonObj.find("\"waves\"");
		if (wavesPos != std::string::npos) {
			size_t start = jsonObj.find(":", wavesPos) + 1;
			size_t end = jsonObj.find(",", start);
			if (end == std::string::npos) end = jsonObj.find("}", start);

			waves = jsonObj.substr(start, end - start);
		}

		// Extract duration
		size_t durationPos = jsonObj.find("\"duration\"");
		if (durationPos != std::string::npos) {
			size_t start = jsonObj.find(":", durationPos) + 1;
			size_t end = jsonObj.find(",", start);
			if (end == std::string::npos) end = jsonObj.find("}", start);

			duration = jsonObj.substr(start, end - start);
			// Remove quotes
			duration.erase(std::remove(duration.begin(), duration.end(), '\"'), duration.end());
		}

		// Extract bonus
		size_t bonusPos = jsonObj.find("\"bonus\"");
		if (bonusPos != std::string::npos) {
			size_t start = jsonObj.find(":", bonusPos) + 1;
			size_t end = jsonObj.find(",", start);
			if (end == std::string::npos) end = jsonObj.find("}", start);

			bonus = jsonObj.substr(start, end - start);
		}

		// Format the data for display (pipe-delimited)
		std::string formattedScore = timestamp + "|" + score + "|" + kills + "|" + waves + "|" + duration + "|" + bonus;
		formattedScores.push_back(formattedScore);
	}

	// Debug message - you can comment this out in final version
	printf("Loaded %d scores\n", (int)formattedScores.size());

	return formattedScores;
}

// Helper function to check if file exists
bool fileExists(const std::string& filename) {
	std::ifstream file(filename);
	return file.good();
}

// Display past scores
void PastScore_Display(Player* player, AEGfxVertexList* Mesh, s8 font_id) {
	// Display background panel
	AEMtx33 PastScorePanel_transform, PastScorePanel_angle, PastScorePanel_scale, PastScorePanel_translate;

	// Display background panel with a slightly different color
	AEGfxSetColorToAdd(0.3f, 0.5f, 0.9f, 1.0f);
	AEMtx33Identity(&PastScorePanel_transform);
	AEMtx33Rot(&PastScorePanel_angle, 0.0f);
	AEMtx33Scale(&PastScorePanel_scale, AEGfxGetWindowWidth() / 2.0f, AEGfxGetWindowHeight() / 1.5f);
	AEMtx33Trans(&PastScorePanel_translate, player->player_shape.pos_x, player->player_shape.pos_y);
	AEMtx33Concat(&PastScorePanel_transform, &PastScorePanel_angle, &PastScorePanel_scale);
	AEMtx33Concat(&PastScorePanel_transform, &PastScorePanel_translate, &PastScorePanel_transform);
	AEGfxSetTransform(PastScorePanel_transform.m);
	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);

	// Past scores title
	AEGfxPrint(font_id, "PAST SCORES", -0.20f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f);

	// Display header
	AEGfxPrint(font_id, "Score  Kills WaveCleared Bonus Duration", -0.45f, 0.3f, 0.3f, 0.7f, 0.7f, 1.0f, 1.0f);
	AEGfxPrint(font_id, "=======================================", -0.45f, 0.25f, 0.3f, 0.7f, 0.7f, 1.0f, 1.0f);

	// Load past scores
	std::vector<std::string> pastScores = LoadPastScores();

	if (pastScores.empty()) {
		// No past scores to display
		AEGfxPrint(font_id, "No past scores available", -0.25f, 0.0f, 0.4f, 1.0f, 1.0f, 1.0f, 1.0f);
	}
	else {
		// Display past scores
		float y_pos = 0.2f;
		for (const auto& scoreEntry : pastScores) {
			// Parse the score entry
			std::istringstream ss(scoreEntry);
			std::string token;
			std::vector<std::string> tokens;

			// Split by '|' delimiter
			while (std::getline(ss, token, '|')) {
				tokens.push_back(token);
			}

			if (tokens.size() >= 6) {
				// Format: date|total_score|kills|waves|duration|bonus
				char scoreDisplay[100];
				sprintf_s(scoreDisplay, "%-5s  %-5s  %-11s  %-5s  %-8s",
					tokens[1].c_str(),         // total score
					tokens[2].c_str(),         // kills
					tokens[3].c_str(),         // waves cleared
					tokens[5].c_str(),         // bonus
					tokens[4].c_str()          // duration
				);

				AEGfxPrint(font_id, scoreDisplay, -0.45f, y_pos, 0.3f, 1.0f, 1.0f, 1.0f, 1.0f);
				y_pos -= 0.1f;
			}
		}
	}

	// Back button (single centered button)
	AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);
	AEMtx33 BackBtn_transform, BackBtn_angle, BackBtn_scale, BackBtn_translate;
	AEMtx33Identity(&BackBtn_transform);
	AEMtx33Rot(&BackBtn_angle, 0.0f);
	AEMtx33Scale(&BackBtn_scale, 400.0f, 100.0f);
	AEMtx33Trans(&BackBtn_translate, player->player_shape.pos_x, player->player_shape.pos_y - 350.0f);
	AEMtx33Concat(&BackBtn_transform, &BackBtn_angle, &BackBtn_scale);
	AEMtx33Concat(&BackBtn_transform, &BackBtn_translate, &BackBtn_transform);
	AEGfxSetTransform(BackBtn_transform.m);
	AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
	AEGfxPrint(font_id, "BACK", -0.05f, -0.80f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);
}


void Victory_ScreenDisplay(Player* player, AEGfxVertexList* mesh, s8 font_id) {

	AEGfxSetColorToAdd(0.0f, 0.8f, 0.0f, 1.0f);  // Green tint for victory

	AEMtx33 Victory_transform, Victory_angle, Victory_scale, Victory_translate;

#pragma region VictoryDisplay
	AEMtx33Identity(&Victory_transform);
	AEMtx33Rot(&Victory_angle, 0.0f);
	AEMtx33Scale(&Victory_scale, (f32)AEGfxGetWindowWidth(), (f32)AEGfxGetWindowHeight());
	AEMtx33Trans(&Victory_translate, player->player_shape.pos_x, player->player_shape.pos_y);

	AEMtx33Concat(&Victory_transform, &Victory_angle, &Victory_scale);
	AEMtx33Concat(&Victory_transform, &Victory_translate, &Victory_transform);

	AEGfxSetTransform(Victory_transform.m);
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

	AEGfxPrint(font_id, "VICTORY!", -0.8f, 0.0f, 3.0f, 0.8f, 1.0f, 0.0f, 1.0f);
#pragma endregion

#pragma region MENU
	AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
	AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

	AEMtx33 MENU_transform;
	AEMtx33Identity(&MENU_transform);

	AEMtx33 MENU_spin;
	AEMtx33Rot(&MENU_spin, 0.0f);

	AEMtx33 MENU_scale;
	AEMtx33Scale(&MENU_scale, 300.0f, 100.0f);

	AEMtx33 MENU_translate;
	AEMtx33Trans(&MENU_translate, player->player_shape.pos_x, (player->player_shape.pos_y) - 175.0f);

	AEMtx33Concat(&MENU_transform, &MENU_spin, &MENU_scale);
	AEMtx33Concat(&MENU_transform, &MENU_translate, &MENU_transform);

	AEGfxSetTransform(MENU_transform.m);

	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

	AEGfxPrint(font_id, "MENU", -0.045f, -0.40f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);
#pragma endregion

#pragma region score
	AEMtx33 Score_transform;
	AEMtx33Identity(&Score_transform);

	AEMtx33 score_spin;
	AEMtx33Rot(&score_spin, 0.0f);

	AEMtx33 score_scale;
	AEMtx33Scale(&score_scale, 300.0f, 100.0f);

	AEMtx33 score_translate;
	AEMtx33Trans(&score_translate, player->player_shape.pos_x, (player->player_shape.pos_y) - 350.0f);

	AEMtx33Concat(&Score_transform, &score_spin, &score_scale);
	AEMtx33Concat(&Score_transform, &score_translate, &Score_transform);

	AEGfxSetTransform(Score_transform.m);

	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

	AEGfxPrint(font_id, "SCORE", -0.045f, -0.80f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);
#pragma endregion
}