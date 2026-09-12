/* Start Header ************************************************************************/
/*!
\file		Logo.cpp
\author		Hao Peng
\date		March, 20, 2025
\brief      Implementation file for displaying the game logo.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "Logo.h"
#include "GameStateList.h"
#include "GameStateManager.h"
#include "AEEngine.h"
#include <iostream>
#include "Audio.h"
// Resources for the logo splash screen
static AEGfxTexture* logoTexture = nullptr;
static AEGfxVertexList* logoMesh = nullptr;
static s8 logoFontId = -1;
static float fadeTimer = 0.0f;
static float fadeAlpha = 0.0f;
static const float LOGO_DURATION = 3.5f;       // Total duration
static const float FADE_IN_DURATION = 1.0f;    // Time to fade in
static const float FADE_OUT_DURATION = 1.0f;   // Time to fade out
static const float DISPLAY_DURATION = 1.5f;    // Time at full opacity

enum LogoState { FADING_IN, VISIBLE, FADING_OUT, COMPLETE };
static LogoState logoState = FADING_IN;

void Logo_Load()
{
    //std::cout << "Logo: Load" << std::endl;
    //printf("Audio initialize!\n");
    Audio_Init();
    // Get current working directory to debug path issues
    char currentDir[256];
    GetCurrentDirectory(256, currentDir);
    //std::cout << "Current working directory: " << currentDir << std::endl;

    // Try to load the texture
    const char* logoPath = "Assets/DigiPen_Singapore_WEB_WHITE.png";
    //std::cout << "Attempting to load logo from: " << logoPath << std::endl;

    logoTexture = AEGfxTextureLoad(logoPath);

    //if (!logoTexture) {
    //    //std::cout << "ERROR: Failed to load logo texture!" << std::endl;
    //    // Let's try an absolute path as a test
    //    logoTexture = AEGfxTextureLoad("C:/Path/To/Your/Project/Assets/logo.png");
    //    if (!logoTexture) {
    //        std::cout << "ERROR: Failed with absolute path too!" << std::endl;
    //    }
    //}
    //else {
    //    std::cout << "SUCCESS: Logo texture loaded successfully!" << std::endl;
    //}

    // Create the mesh for displaying the texture
    AEGfxMeshStart();
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    logoMesh = AEGfxMeshEnd();
}

void Logo_Initialize()
{
    //std::cout << "Logo: Initialize" << std::endl;

    // Initialize logo animation parameters
    fadeTimer = 0.0f;
    fadeAlpha = 0.0f;
    logoState = FADING_IN;
}

void Logo_Update()
{
    // Update logo timer
    fadeTimer += (float)AEFrameRateControllerGetFrameTime();

    // Update logo state and alpha based on timer
    switch (logoState) {
    case FADING_IN:
        // During fade-in: 0.0 -> 1.0 over FADE_IN_DURATION
        fadeAlpha = fadeTimer / FADE_IN_DURATION;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            logoState = VISIBLE;
        }
        break;

    case VISIBLE:
        // During visible phase: maintain alpha at 1.0
        fadeAlpha = 1.0f;
        if (fadeTimer >= FADE_IN_DURATION + DISPLAY_DURATION) {
            logoState = FADING_OUT;
        }
        break;

    case FADING_OUT:
        // During fade-out: 1.0 -> 0.0 over FADE_OUT_DURATION
        fadeAlpha = 1.0f - (fadeTimer - FADE_IN_DURATION - DISPLAY_DURATION) / FADE_OUT_DURATION;
        if (fadeAlpha <= 0.0f) {
            fadeAlpha = 0.0f;
            logoState = COMPLETE;
        }
        break;

    case COMPLETE:
        // Once complete, transition to menu
        next = GS_MENU;
        break;
    }

    // If total duration is complete, move to menu state
    if (fadeTimer >= LOGO_DURATION) {
        next = GS_MENU;
    }

    // Also check for any key/click to skip logo screen
    if (AEInputCheckTriggered(AEVK_LBUTTON) || AEInputCheckTriggered(AEVK_SPACE) ||
        AEInputCheckTriggered(AEVK_RETURN) || AEInputCheckTriggered(AEVK_ESCAPE)) {
        next = GS_MENU;
    }
}

void Logo_Draw()
{
    // Clear background
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    // Set up for texture rendering
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    // Set camera position
    AEGfxSetCamPosition(0.0f, 0.0f);

    //std::cout << logoState << std::endl;
    // If no texture was loaded, just show a small debug message
    if (!logoTexture) {
        if (logoFontId >= 0) {
            AEGfxPrint(logoFontId, "Logo texture failed to load", -0.5f, 0.0f, 0.4f, 1.0f, 0.0f, 0.0f, 1.0f);
        }
        return;
    }

    // Replace the blend mode section with:
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, fadeAlpha); // Use alpha here
    AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 0.0f); // Don't use blend color for fading
    AEGfxSetTransparency(1.0f);

    // Create the transform for the logo
    AEMtx33 transform = { 0 };
    AEMtx33Identity(&transform);

    // Scale to an appropriate size
    AEMtx33Scale(&transform, 1200.0f, 800.0f);


    // Set the texture
    AEGfxTextureSet(logoTexture, 0, 0);
    AEGfxSetTransform(transform.m);

    //std::cout << "Logo: Draw" << std::endl;
    // Draw the logo
    AEGfxMeshDraw(logoMesh, AE_GFX_MDM_TRIANGLES);

    // Reset blend mode
    //AEGfxSetBlendMode(AE_GFX_BM_NONE);
}
void Logo_Free()
{
    //std::cout << "Logo: Free" << std::endl;
    // No temporary resources to free
}

void Logo_Unload()
{
    //std::cout << "Logo: Unload" << std::endl;

    // Free the mesh
    if (logoMesh) {
        AEGfxMeshFree(logoMesh);
        logoMesh = nullptr;
    }

    // Free the texture
    if (logoTexture) {
        AEGfxTextureUnload(logoTexture);
        logoTexture = nullptr;
    }

    // Free the font
    if (logoFontId >= 0) {
        AEGfxDestroyFont(logoFontId);
        logoFontId = -1;
    }
}