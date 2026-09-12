/* Start Header ************************************************************************/
/*!
\file		Options.h
\author		Hao Peng
\date		March, 20, 2025
\brief		Header file for game settings and options menu functionality
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef options_H
#define options_H

#include "AEEngine.h"
#include "Collision.h"  // Add this include to access TriangleCollider struct

// Triangle collider creation functions
void CreateTriangleCollider(TriangleCollider* triangle, float centerX, float centerY, float size);
void CreateDownTriangleCollider(TriangleCollider* triangle, float centerX, float centerY, float size);

// Drawing functions for options UI elements
void Options(s8 font_id);
void SoundUp(AEGfxVertexList* Mesh);
void SoundDown(AEGfxVertexList* Mesh);
void MusicUp(AEGfxVertexList* Mesh);
void MusicDown(AEGfxVertexList* Mesh);

// Function to handle button clicks
void CheckButtonClicks();

// Volume accessors
int GetSoundVolume();
int GetMusicVolume();

// Game state management functions
void Options_Load();
void Options_Initialize();
void Options_Update();
void Options_Draw();
void Options_Free();
void Options_Unload();

#endif // options_H