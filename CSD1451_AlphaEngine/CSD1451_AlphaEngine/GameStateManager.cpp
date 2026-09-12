/* Start Header ************************************************************************/
/*!
\file		GameStateManager.cpp
\author		Hao Peng
\date		March, 20, 2025
\brief      Implementation of the game state system for 
            managing transitions between different game modes.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include <crtdbg.h> // To check for memory leaks
#include "GameStateManager.h"
#include "GameStateList.h"
#include "Howtoplay.h"
#include "Options.h"
#include "Logo.h"
#include "Menu.h"
#include "Gameplay.h"
#include "Credit.h"

//#include <stdio.h>
//#include <chrono>
//#include <thread>
//#include <iostream>
//#include <fstream>
//
//#include "Audio.h"
//#include "Collision.h"
//#include "Credit.h"
//#include "enemy.h"
//#include "Game_Management.h"
//#include "Gameplay.h"
//#include "GameStateList.h"
//#include "GameStateManager.h"
//#include "Howtoplay.h"
//#include "Logo.h"
//#include "Main.h"
//#include "Menu.h"
//#include "Options.h"
//#include "Player.h"
//#include "SkillMenu.h"
//#include "wave.h"
//#include "weapon.h"

// Global state tracking variables
int current = 0, previous = 0, next = 0;

// Function pointers for state management
FP fpLoad = nullptr, fpInitialize = nullptr, fpUpdate = nullptr, fpDraw = nullptr, fpFree = nullptr, fpUnload = nullptr;

void GSM_Initialize(int startingState)
{
    // Set the current, previous, and next state to the starting state
    current = previous = next = startingState;
}

void GSM_Update()
{
    // Switch based on the current game state to set the correct function pointers
    switch (current)
    {
    case GS_LOGO:
        fpLoad = Logo_Load;
        fpInitialize = Logo_Initialize;
        fpUpdate = Logo_Update;
        fpDraw = Logo_Draw;
        fpFree = Logo_Free;
        fpUnload = Logo_Unload;
        break;

    case GS_MENU:
        fpLoad = Menu_Load;
        fpInitialize = Menu_Initialize;
        fpUpdate = Menu_Update;
        fpDraw = Menu_Draw;
        fpFree = Menu_Free;
        fpUnload = Menu_Unload;
        break;

    case GS_GAMEPLAY:
        fpLoad = Gameplay_Load;
        fpInitialize = Gameplay_Initialize;
        fpUpdate = Gameplay_Update;
        fpDraw = Gameplay_Draw;
        fpFree = Gameplay_Free;
        fpUnload = Gameplay_Unload;
        break;

    case GS_OPTIONS:
        fpLoad = Options_Load;
        fpInitialize = Options_Initialize;
        fpUpdate = Options_Update;
        fpDraw = Options_Draw;
        fpFree = Options_Free;
        fpUnload = Options_Unload;
        break;

    case GS_HOWTOPLAY:
        fpLoad = Howtoplay_Load;
        fpInitialize = Howtoplay_Initialize;
        fpUpdate = Howtoplay_Update;
        fpDraw = Howtoplay_Draw;
        fpFree = Howtoplay_Free;
        fpUnload = Howtoplay_Unload;
        break;

    case GS_CREDIT:
        fpLoad = Credit_Load;
        fpInitialize = Credit_Initialize;
        fpUpdate = Credit_Update;
        fpDraw = Credit_Draw;
        fpFree = Credit_Free;
        fpUnload = Credit_Unload;
        break;
    }
}