/* Start Header ************************************************************************/
/*!
\file		GameStateList.h
\author		Hao Peng
\date		March, 20, 2025
\brief      Header file defining the game state enumeration and state identifiers

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef GAMESTATELIST_H
#define GAMESTATELIST_H

// Enum to represent the different game states
enum GS_STATES
{

    GS_LOGO = 0,   // Start with logo screen

    // Main menu state
    GS_MENU,

    // Gameplay state - the main game loop
    GS_GAMEPLAY,

    // How to play state
    GS_HOWTOPLAY,

    // Options menu state
    GS_OPTIONS,

    // Credits screen state
    GS_CREDIT,

    // Quit state - the game should terminate
    GS_QUIT
};

#endif // GAMESTATELIST_H