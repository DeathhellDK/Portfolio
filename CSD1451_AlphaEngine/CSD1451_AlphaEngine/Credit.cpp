/* Start Header ************************************************************************/
/*!
\file		Credit.cpp
\author		Hao Peng
\date		March, 20, 2025
\brief      Implementation file for displaying scrolling credits 
            and contributor information.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include <crtdbg.h> // To check for memory leaks
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "Audio.h"
#include "Credit.h"
#include "GameStateList.h"
#include "GameStateManager.h"
#include "Main.h"
#include "Menu.h"

// Characters to display
const char* credit_names[] = {
    "FINAL STAND",
    "",
    "",
    "by Pew Pew",
    "",
    "",
    "Members:",
    "Terril",
    "Zhi Jie",
    "Ting Kai",
    "Jia Hao",
    "Hao Peng",
    "",
    "",
    "Instructors:",
    "Cheng Ding Xiang",
    "Gerald Wong",
    "Soroor",
    "",
    "",
    "",
    "President:",
    "Claude Comair",
    "",
    "",
    "",
    "",
    "Thank you!"
};

float creditX = -0.3f;          // X position for text alignment
float creditY = 0.7f;           // Start slightly off-screen
float scrollSpeed = 0.1f;       // Initial scroll speed
float normalScrollSpeed = 0.1f; // Normal scroll speed
float fastScrollSpeed = 1.0f;   // Fast scroll speed when holding left mouse button
float spacing = 0.15f;          // Space between lines

void Credit(int font_id) {

    static bool firstEntry = true;

    // Reset position when entering credits page for the first time or after leaving
    if (firstEntry) {
        creditY = -1.2f;  // Start from bottom of screen
        firstEntry = false;
    }

    // Check if left mouse button is currently being held down
    if (AEInputCheckCurr(AEVK_LBUTTON)) {
        // Increase scroll speed to fast speed
        scrollSpeed = fastScrollSpeed;
    }

    // Check if left mouse button was released
    if (AEInputCheckReleased(AEVK_LBUTTON)) {
        // Reset scroll speed to normal speed
        scrollSpeed = normalScrollSpeed;
    }

    // Get frame time for smooth scrolling
    float dt = (float)AEFrameRateControllerGetFrameTime();

    // Update position for scrolling
    creditY += scrollSpeed * dt;

    // Calculate total height of credits
    const int num_credits = sizeof(credit_names) / sizeof(credit_names[0]);
    float totalHeight = num_credits * spacing;

    // Reset creditY position if all credits have scrolled off the screen
    if (creditY - totalHeight > 1.2f) {
        creditY = -1.2f;
    }

    // Set dark background
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Render each line of credits
    for (int i = 0; i < num_credits; i++) {
        float currentY = creditY - (i * spacing);
        // Only render credits that are visible on screen
        if (currentY <= 1.2f && currentY >= -1.2f) {
            AEGfxPrint(static_cast<s8>(font_id), credit_names[i], creditX, currentY, 0.2f, 1, 1, 1, 1);
        }
    }

    AEGfxPrint(static_cast<s8>(font_id), "Press B to go back", 0.4f, 0.9f, 0.2f, 1, 1, 1, 1);
}

// Resources for Credit screen
static s8 CreditFontID = -1;
static Mouse CreditMouse;

void Credit_Load()
{
    // Load font
    CreditFontID = AEGfxCreateFont("Assets/liberation-mono.ttf", 100);
}

void Credit_Initialize()
{
    // Initialize mouse position
    CreditMouse.mouseX = 0;
    CreditMouse.mouseY = 0;
}

void Credit_Update()
{
    // Get mouse position
    AEInputGetCursorPosition(&CreditMouse.mouseX, &CreditMouse.mouseY);

    // Check for back button press (B key)
    if (AEInputCheckTriggered(AEVK_B) || AEInputCheckTriggered(AEVK_ESCAPE))
    {
        next = GS_MENU;
    }
}

void Credit_Draw()
{
    // Call the original Credit function with the font ID
    Credit(CreditFontID);
}

void Credit_Free()
{
    // Free temporary resources
}

void Credit_Unload()
{
    // Free font
    if (CreditFontID >= 0) {
        AEGfxDestroyFont(CreditFontID);
        CreditFontID = -1;
    }
}