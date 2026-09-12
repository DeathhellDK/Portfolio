/* Start Header ************************************************************************/
/*!
\file		Menu.cpp
\author		Hao Peng
\date		March, 20, 2025
\brief      Implementation of the main menu interface and 
            button interaction functionality.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "Menu.h"
#include "Main.h"
#include <fstream>
#include <iostream>
#include "GameStateList.h"
#include "GameStateManager.h"
#include "Audio.h"
#pragma region START GAME
	void startgame(AEGfxVertexList* Mesh) {
		AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
		AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 0.0f);

		AEMtx33 START_transform;
		AEMtx33Identity(&START_transform);

		AEMtx33 START_spin;
		AEMtx33Rot(&START_spin, 0.0f);

		AEMtx33 START_scale;
		AEMtx33Scale(&START_scale, 300.0f, 100.0f);

		AEMtx33 START_translate;
		AEMtx33Trans(&START_translate, 0.0f , 300.0f);

		AEMtx33Concat(&START_transform, &START_spin, &START_scale);
		AEMtx33Concat(&START_transform, &START_translate, &START_transform);

		AEGfxSetTransform(START_transform.m);

		AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
	}

#pragma endregion

#pragma region HOW TO PLAY
	void howtoplay(AEGfxVertexList* Mesh) {
		AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
		AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

		AEMtx33 HOW_transform;
		AEMtx33Identity(&HOW_transform);

		AEMtx33 HOW_spin;
		AEMtx33Rot(&HOW_spin, 0.0f);

		AEMtx33 HOW_scale;
		AEMtx33Scale(&HOW_scale, 300.0f, 100.0f);

		AEMtx33 HOW_translate;
		AEMtx33Trans(&HOW_translate, 0.0f, 150.0f);

		AEMtx33Concat(&HOW_transform, &HOW_spin, &HOW_scale);
		AEMtx33Concat(&HOW_transform, &HOW_translate, &HOW_transform);

		AEGfxSetTransform(HOW_transform.m);

		AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
	}
#pragma endregion


#pragma region OPTIONS
	void options(AEGfxVertexList* Mesh) {
		AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
		AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

		AEMtx33 OPTIONS_transform;
		AEMtx33Identity(&OPTIONS_transform);

		AEMtx33 OPTIONS_spin;
		AEMtx33Rot(&OPTIONS_spin, 0.0f);

		AEMtx33 OPTIONS_scale;
		AEMtx33Scale(&OPTIONS_scale, 300.0f, 100.0f);

		AEMtx33 OPTIONS_translate;
		AEMtx33Trans(&OPTIONS_translate, 0.0f, 0.0f);

		AEMtx33Concat(&OPTIONS_transform, &OPTIONS_spin, &OPTIONS_scale);
		AEMtx33Concat(&OPTIONS_transform, &OPTIONS_translate, &OPTIONS_transform);

		AEGfxSetTransform(OPTIONS_transform.m);

		AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
	}
#pragma endregion


#pragma region CREDITS
	void credits(AEGfxVertexList* Mesh) {
		AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
		AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

		AEMtx33 CREDITS_transform;
		AEMtx33Identity(&CREDITS_transform);

		AEMtx33 CREDITS_spin;
		AEMtx33Rot(&CREDITS_spin, 0.0f);

		AEMtx33 CREDITS_scale;
		AEMtx33Scale(&CREDITS_scale, 300.0f, 100.0f);

		AEMtx33 CREDITS_translate;
		AEMtx33Trans(&CREDITS_translate, 0.0f, -150.0f);

		AEMtx33Concat(&CREDITS_transform, &CREDITS_spin, &CREDITS_scale);
		AEMtx33Concat(&CREDITS_transform, &CREDITS_translate, &CREDITS_transform);

		AEGfxSetTransform(CREDITS_transform.m);

		AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
	}

#pragma endregion


#pragma region QUIT
	void quitbutton(AEGfxVertexList* Mesh) {
		AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
		AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 1.0f);

		AEMtx33 QUIT_transform;
		AEMtx33Identity(&QUIT_transform);

		AEMtx33 QUIT_spin;
		AEMtx33Rot(&QUIT_spin, 0.0f);

		AEMtx33 QUIT_scale;
		AEMtx33Scale(&QUIT_scale, 300.0f, 100.0f);

		AEMtx33 QUIT_translate;
		AEMtx33Trans(&QUIT_translate, 0.0f, -300.0f);

		AEMtx33Concat(&QUIT_transform, &QUIT_spin, &QUIT_scale);
		AEMtx33Concat(&QUIT_transform, &QUIT_translate, &QUIT_transform);

		AEGfxSetTransform(QUIT_transform.m);

		AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);
	}

#pragma endregion

    // Global resources for the menu
    static AEGfxVertexList* MenuMesh = nullptr;
    static s8 MenuFontID = -1;
    static Mouse MenuMouse;

    void Menu_Load()
    {
        // Load the menu font once
        const char* fontPath = "Assets/liberation-mono.ttf";
        MenuFontID = AEGfxCreateFont(fontPath, 100);

        // Create menu mesh (rectangle)
        AEGfxMeshStart();
        AEGfxTriAdd(
            -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
            0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
            -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        AEGfxTriAdd(
            0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
            0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
            -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        MenuMesh = AEGfxMeshEnd();
    }

    void Menu_Initialize()
    {
        // Initialize mouse position
        MenuMouse.mouseX = 0;
        MenuMouse.mouseY = 0;
        PlayGameSound(MENU);
    }

    void Menu_Update()
    {
        AEGfxSetCamPosition(0, 0);

        // Set the game background 
        AEGfxSetBackgroundColor(0.6f, 0.6f, 0.6f);

        // Set the render for the game
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
       

        // Get mouse position
        AEInputGetCursorPosition(&MenuMouse.mouseX, &MenuMouse.mouseY);
        MenuMouse.mouseX -= (AEGfxGetWindowWidth() / 2);
        MenuMouse.mouseY = (AEGfxGetWindowHeight() / 2) - MenuMouse.mouseY;
        MenuMouse.vec_Mouse = { static_cast<f32>(MenuMouse.mouseX), static_cast<f32>(MenuMouse.mouseY) };

        // Check for button clicks
        // START GAME
        AEVec2 startgame_pos = { 0.0f, 300.0f };
        if (AETestPointToRect(&MenuMouse.vec_Mouse, &startgame_pos, 300.0f, 100.0f) && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            PlayGameSound(CLICK);
            next = GS_GAMEPLAY;
        }

        // HOW TO PLAY (part of options in this implementation)
        AEVec2 howtoplay_pos = { 0.0f, 150.0f };
        if (AETestPointToRect(&MenuMouse.vec_Mouse, &howtoplay_pos, 300.0f, 100.0f) && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            PlayGameSound(CLICK);
            next = GS_HOWTOPLAY;
        }

        // OPTIONS
        AEVec2 options_pos = { 0.0f, 0.0f };
        if (AETestPointToRect(&MenuMouse.vec_Mouse, &options_pos, 300.0f, 100.0f) && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            PlayGameSound(CLICK);
            next = GS_OPTIONS;
        }

        // CREDITS
        AEVec2 credit_pos = { 0.0f, -150.0f };
        if (AETestPointToRect(&MenuMouse.vec_Mouse, &credit_pos, 300.0f, 100.0f) && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            PlayGameSound(CLICK);
            next = GS_CREDIT;
        }

        // QUIT
        AEVec2 quit_pos = { 0.0f, -300.0f };
        if (AETestPointToRect(&MenuMouse.vec_Mouse, &quit_pos, 300.0f, 100.0f) && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            PlayGameSound(CLICK);
            next = GS_QUIT;
        }
    }

    void Menu_Draw()
    {
        // Draw all menu buttons
        startgame(MenuMesh);
        AEGfxPrint(MenuFontID, "START GAME", -0.11f, 0.65f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);

        howtoplay(MenuMesh);
        AEGfxPrint(MenuFontID, "HOW TO PLAY", -0.12f, 0.3f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);

        options(MenuMesh);
        AEGfxPrint(MenuFontID, "OPTIONS", -0.08f, -0.02f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);

        credits(MenuMesh);
        AEGfxPrint(MenuFontID, "CREDITS", -0.08f, -0.35f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);

        quitbutton(MenuMesh);
        AEGfxPrint(MenuFontID, "QUIT", -0.05f, -0.70f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    void Menu_Free()
    {
        // Free temporary resources for the current menu session
    }

    void Menu_Unload()
    {
        // Free the mesh when unloading menu resources
        if (MenuMesh)
        {
            AEGfxMeshFree(MenuMesh);
            MenuMesh = nullptr;
        }

        // Free the font
        if (MenuFontID >= 0)
        {
            AEGfxDestroyFont(MenuFontID);
            MenuFontID = -1;
        }
        if (next == GS_QUIT)  // 
        {
            Audio_Shutdown();
            //printf("Audio shutwdown!\n");
        }
    }


