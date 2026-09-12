/* Start Header ************************************************************************/
/*!
\file		Game_Management.h
\author		Tan Wei Liang Terril (85%), Zhi Jie, Tingkai, Hao Peng
\date		March, 20, 2025
\brief		File contains the function to set the game management
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
#ifndef GAME_MANAGEMENT_H
#define GAME_MANAGEMENT_H

#include "Player.h"

/// <summary>
///  Initialise the scoreboard value to a default value
/// </summary>
/// <param name="score"> Retrieve score data </param>
/// <returns></returns>
ScoreBoard *scoreBoardInit(ScoreBoard* score);

/// <summary>
/// Tabulation the player current gameplay score
/// </summary>
/// <param name="playerScore"> save into the player score which will use to display the respecitve score component </param>
/// <param name="score"> Retrieve the score struct data </param>
/// <returns></returns>
int scoreTabulation(int& playerScore, ScoreBoard* score);

/// <summary>
/// Handle how the player experience (exp) will be drawn
/// </summary>
/// <param name="player"> Retreive the player data </param>
/// <param name="mesh"> Retreive the mesh data that will be use to draw the exp </param>
void draw_exp(Player* player, AEGfxVertexList *mesh);

/// <summary>
/// Handle how the player health (hp) will be drawn
/// </summary>
/// <param name="player"> Retreive the player data </param>
/// <param name="mesh"> Retreive the mesh data that will be use to draw the hp</param>
/// <param name="font_id"> Retrieve the font id that will be use for the text </param>
void draw_hp(Player* player, AEGfxVertexList* mesh, s8 font_id);

/// <summary>
/// Handle how the player move 
/// </summary>
/// <param name="player"> Retreive the player struct </param>
void player_Input_Movement(Player* player);

/*
	Add the bullet into the respecitve vector:
	- BulletVectorID: Whos vector it belong to? (example: Player, Weapon, enemy etc...)
	- Bullet: Whos struct it belong to? (example: Player, Weapon, enemy etc...)
	- posX, PosY: Where you want it to spawn at? (example: Player position, enemy position etc...)
	- MAX_AMMO: Mainly to 'control' how big the vector can become before remove the first entry.

	Note: I have already decalre a constexpr of the MAX_AMMO under the main.h, all you have to do is to
	call MAX_AMMO_SIZE into the function paramater and the function will handle it.
*/

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
void BulletInit(std::vector<Bullet>& BulletVectorID, Bullet bullet, float posX, float posY, int &damageOutput,const size_t MAX_AMMO, BULLET_FIRED_BY id, float userRoation);

/// <summary>
/// Update the bullet on it position and rotation
/// </summary>
/// <param name="BulletVectorID"> Retreive the bullet vector array </param>
/// <param name="enemyColliders"> Retreive the enemy collider</param>
/// <param name="enemyCount"> Retreive the amount of enemy </param>
/// <param name="playerCollider"> Retreive the player collidr </param>
/// <param name="isPlayerBullet"> Retreive the mesh data that will be use to draw the hp </param>
void Bulletupdate(std::vector<Bullet>& BulletVectorID);

/// <summary>
/// how the will be drawn base on its translate, rotation and scale
/// </summary>
/// <param name="BulletVectorID"> Retreive the bullet vector array </param>
/// <param name="player"> Retreive the player data </param>
/// <param name="mesh"></param>
void DrawBullet(std::vector<Bullet>& BulletVectorID, Player *player, AEGfxVertexList* mesh);

/// <summary>
/// Display game over when player life is drop to 0 
/// </summary>
/// <param name="player"> Retrieve the player data </param>
/// <param name="score"> Retrieve the the score data </param>
/// <param name="mesh"> Retrieve the mesh data </param>
/// <param name="font_id"> Retrieve the font id  data </param>
void Death_ScreenDisplay(Player* player, AEGfxVertexList* mesh, s8 font_id);

//void menu(Player* player, AEGfxVertexList* mesh);

//template<typename T> void Free_vector(std::vector<T> &free_vec);

//void score(Player* player, AEGfxVertexList* mesh);

/// <summary>
/// Function to display past scores
/// </summary>
/// <param name="player"> - Retreive the player struct data </param>
/// <param name="score"> - Retrieve the score struct data </param>
/// <param name="Mesh"> - Get created mesh to draw display board </param>
/// <param name="font_id"> - Font used to display </param>
void ScoreBoard_Display(Player* player, ScoreBoard* score, AEGfxVertexList* Mesh, s8 font_id);


/// <summary>
/// Function to display past scores
/// </summary>
/// <param name="player"> - Retreive the player struct data </param>
/// <param name="Mesh"> - Get created mesh to draw display board </param>
/// <param name="font_id"> - Font used to display </param>
void PastScore_Display(Player* player, AEGfxVertexList* Mesh, s8 font_id);


/// <summary>
/// Function to save the current score to file
/// </summary>
/// <param name="score"> - Retrieve the score struct data </param>
/// <param name="player"> - Retreive the player struct data </param>
void SaveScoreToFile(ScoreBoard* score, Player* player);


/// <summary>
/// Function to load past scores (vector) from file 
/// </summary>
std::vector<std::string> LoadPastScores();

/// <summary>
/// Display victory screen when player clears all waves
/// </summary>
/// <param name="player"> Retrieve the player data </param>
/// <param name="score"> Retrieve the the score data </param>
/// <param name="mesh"> Retrieve the mesh data </param>
/// <param name="font_id"> Retrieve the font id  data </param>
void Victory_ScreenDisplay(Player* player, AEGfxVertexList* mesh, s8 font_id);

#endif // !GAME_MANAGEMENT_H

