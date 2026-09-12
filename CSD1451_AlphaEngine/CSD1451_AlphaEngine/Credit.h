/* Start Header ************************************************************************/
/*!
\file		Credit.h
\author		Hao Peng
\date		March, 20, 2025
\brief		Header file for handling and displaying game credits

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef credit_H
#define credit_H

#pragma once
void Credit(int font_id);
// extern float creditY;
extern const char* credit_names[];

// Function to load resources for the Credit state
void Credit_Load();

// Function to initialize the Credit state
void Credit_Initialize();

// Function to update the Credit state
void Credit_Update();

// Function to draw the Credit state
void Credit_Draw();

// Function to free resources for the Credit state
void Credit_Free();

// Function to unload the Credit state
void Credit_Unload();


#endif // CREDIT_H

