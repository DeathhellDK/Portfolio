/* Start Header ************************************************************************/
/*!
\file		Gameplay.h
\author		Hao Peng
\date		March, 20, 2025
\brief		Header file containing core gameplay functions and declarations

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef GAMEPLAY_STATE_H
#define GAMEPLAY_STATE_H

// Function to load resources for the Gameplay state
void Gameplay_Load();

// Function to initialize the Gameplay state
void Gameplay_Initialize();

// Function to update the Gameplay state
void Gameplay_Update();

// Function to draw the Gameplay state
void Gameplay_Draw();

// Function to free resources for the Gameplay state
void Gameplay_Free();

// Function to unload the Gameplay state
void Gameplay_Unload();

#endif // GAMEPLAY_H