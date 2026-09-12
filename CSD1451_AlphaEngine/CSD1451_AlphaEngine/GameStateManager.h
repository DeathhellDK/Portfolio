/* Start Header ************************************************************************/
/*!
\file		GameStateManager.h
\author		Hao Peng
\date		March, 20, 2025
\brief		Header file for the game state management system

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef GAMESTATEMAN_H
#define GAMESTATEMAN_H

// Type definition for function pointers that represent state-specific functions
typedef void(*FP)(void);

// Global variables to hold the current, previous, and next game states
extern int current, previous, next;

// Function pointers for managing game state functions
extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

// Initialize the Game State Manager with a starting state
void GSM_Initialize(int startingState);

// Update the GSM (set function pointers based on current state)
void GSM_Update();

#endif // GAMESTATEMANAGER_H
