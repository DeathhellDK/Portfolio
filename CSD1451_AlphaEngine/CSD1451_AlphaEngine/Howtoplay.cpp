/* Start Header ************************************************************************/
/*!
\file		Howtoplay.cpp
\author		Hao Peng
\date		March, 20, 2025
\brief      Implementation file for displaying game instructions 
            and controls to players.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include <crtdbg.h> // To check for memory leaks
#include <stdio.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>

#include "Audio.h"
#include "Collision.h"
#include "Credit.h"
#include "enemy.h"
#include "Game_Management.h"
#include "Gameplay.h"
#include "GameStateList.h"
#include "GameStateManager.h"
#include "Howtoplay.h"
#include "Main.h"
#include "Menu.h"
#include "Options.h"
#include "Player.h"
#include "SkillMenu.h"
#include "wave.h"
#include "weapon.h"

void Howtoplay() {
    static s8 Howtoplay_font = 0;
    static bool initialized = false;
    static float scrollOffset = 0.0f;  // Scroll offset for moving text up/down
    const float scrollSpeed = 0.05f;   // Adjust scrolling speed
    const float maxScroll = 1.0f;      // Max scroll limit (adjust as needed)
    const float minScroll = -0.1f;     // Min scroll limit (adjust based on text length)

    std::vector<std::string> instructions; // Store instructions dynamically


    // Initialize font only once
    if (!initialized) {
        const char* Howtoplay_fontPath = "Assets/liberation-mono.ttf";
        int fontSize = 200;
        Howtoplay_font = AEGfxCreateFont(Howtoplay_fontPath, fontSize);
        initialized = true;
    }

    // Read instructions from instruction.txt
    std::ifstream file("Assets/instructions.txt");  // Ensure correct path
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            instructions.push_back(line);  // Store each line
        }
        file.close();
    }
    else {
        instructions.push_back("ERROR: Unable to load instructions!");
    }

    // Handle Scrolling Input
    if (AEInputCheckCurr(AEVK_DOWN)) {
        scrollOffset += scrollSpeed;
        if (scrollOffset > maxScroll) scrollOffset = maxScroll;
        //printf("ypos is: %f\n", scrollOffset + 0.9);
    }
    if (AEInputCheckCurr(AEVK_UP)) {
        scrollOffset -= scrollSpeed;
        if (scrollOffset < minScroll) scrollOffset = minScroll;
        //printf("ypos is: %f\n", scrollOffset+0.9);
    }

    // Display instructions on-screen
    float yPos = 0.9f + scrollOffset;  // Start position for text rendering
    for (const std::string& line : instructions) {
        
        AEGfxPrint(Howtoplay_font, line.c_str(), -0.9f, yPos, 0.15f, 1, 1, 1, 1);
        yPos -= 0.1f;  // Move text down for each new line
    }
    
    AEGfxPrint(Howtoplay_font, "Press B to go back", 0.4f, 0.9f, 0.2f, 1, 1, 1, 1);

}


// Original Howtoplay function (from your code)
extern void Howtoplay();

// Global variables for the Howtoplay state
static s8 Howtoplay_font = -1;
static bool initialized = false;
static float scrollOffset = 0.0f;
static std::vector<std::string> instructions;
static Mouse HowtoplayMouse;

void Howtoplay_Load()
{

    // Create font if not already created
    if (!initialized) {
        const char* Howtoplay_fontPath = "Assets/liberation-mono.ttf";
        int fontSize = 200;
        Howtoplay_font = AEGfxCreateFont(Howtoplay_fontPath, fontSize);
        initialized = true;
    }

    // Read instructions from file
    instructions.clear();
    std::ifstream file("Assets/instructions.txt");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            instructions.push_back(line);
        }
        file.close();
    }
    else {
        instructions.push_back("ERROR: Unable to load instructions!");
    }
}

void Howtoplay_Initialize()
{
    // Initialize mouse and scrolling position
    HowtoplayMouse.mouseX = 0;
    HowtoplayMouse.mouseY = 0;
    scrollOffset = 0.0f;  // Reset scroll position when entering the state
}

void Howtoplay_Update()
{
    // Set background color
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Handle Scrolling Input
    const float scrollSpeed = 0.05f;
    const float maxScroll = 1.0f;
    const float minScroll = -0.1f;

    if (AEInputCheckCurr(AEVK_DOWN)) {
        scrollOffset += scrollSpeed;
        if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    }
    if (AEInputCheckCurr(AEVK_UP)) {
        scrollOffset -= scrollSpeed;
        if (scrollOffset < minScroll) scrollOffset = minScroll;
    }

    // Get mouse position
    AEInputGetCursorPosition(&HowtoplayMouse.mouseX, &HowtoplayMouse.mouseY);
    HowtoplayMouse.mouseX -= (AEGfxGetWindowWidth() / 2);
    HowtoplayMouse.mouseY = (AEGfxGetWindowHeight() / 2) - HowtoplayMouse.mouseY;
    HowtoplayMouse.vec_Mouse = { static_cast<f32>(HowtoplayMouse.mouseX), static_cast<f32>(HowtoplayMouse.mouseY) };

    // Check for back button press (B key or Escape)
    if (AEInputCheckTriggered(AEVK_B) || AEInputCheckTriggered(AEVK_ESCAPE)) {
        next = GS_MENU;
    }
}

void Howtoplay_Draw()
{
    // Draw instructions text
    float yPos = 0.8f + scrollOffset;  // Start position for text rendering
    for (const std::string& line : instructions) {
        AEGfxPrint(Howtoplay_font, line.c_str(), -0.9f, yPos, 0.15f, 1, 1, 1, 1);
        yPos -= 0.1f;  // Move text down for each new line
    }

    // Draw back button text
    AEGfxPrint(Howtoplay_font, "Press B to go back", 0.4f, 0.9f, 0.2f, 1, 1, 1, 1);
}

void Howtoplay_Free()
{
    // Free any temporary resources
}

void Howtoplay_Unload()
{

    // Free font and instructions if needed
    if (Howtoplay_font >= 0) {
        AEGfxDestroyFont(Howtoplay_font);
        Howtoplay_font = -1;
        initialized = false;
    }

    instructions.clear();
}
