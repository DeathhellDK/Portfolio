/* Start Header ************************************************************************/
/*!
\file		Logo.h
\author		Hao Peng
\date		March, 20, 2025
\brief		Header file for displaying and managing the game logo.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef LOGO_STATE_H
#define LOGO_STATE_H

// Function to load resources for the Logo state
void Logo_Load();

// Function to initialize the Logo state
void Logo_Initialize();

// Function to update the Logo state
void Logo_Update();

// Function to draw the Logo state
void Logo_Draw();

// Function to free resources for the Logo state
void Logo_Free();

// Function to unload the Logo state
void Logo_Unload();

#endif // LOGO_H