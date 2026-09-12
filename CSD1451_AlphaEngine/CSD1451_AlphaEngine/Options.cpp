/* Start Header ************************************************************************/
/*!
\file		Options.cpp
\author		Hao Peng
\date		March, 20, 2025
\brief      Implementation of the options menu allowing
            players to adjust game settings.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include <crtdbg.h> // To check for memory leaks
#include <stdio.h>

#include "GameStateList.h"
#include "GameStateManager.h"
#include "Main.h"
#include "Menu.h"
#include "Options.h"
#include "Collision.h" // Added for triangle collision functions
#include "Audio.h"
// Global variables to track sound and music values
static int musicValue = 50;
static int soundValue = 50;

// Triangle button positions and colliders
static TriangleCollider musicUpButton;
static TriangleCollider musicDownButton;
static TriangleCollider soundUpButton;
static TriangleCollider soundDownButton;

// Function to create a triangle collider from a centered position and size
void CreateTriangleCollider(TriangleCollider* triangle, float centerX, float centerY, float size) {
    // For upward-pointing triangle
    triangle->v1.x = centerX;
    triangle->v1.y = centerY + size / 2;
    triangle->v2.x = centerX - size / 2;
    triangle->v2.y = centerY - size / 2;
    triangle->v3.x = centerX + size / 2;
    triangle->v3.y = centerY - size / 2;
    triangle->type = TRIANGLE_POINT;
}

// Function to create a downward-pointing triangle collider
void CreateDownTriangleCollider(TriangleCollider* triangle, float centerX, float centerY, float size) {
    // For downward-pointing triangle
    triangle->v1.x = centerX;
    triangle->v1.y = centerY - size / 2;
    triangle->v2.x = centerX - size / 2;
    triangle->v2.y = centerY + size / 2;
    triangle->v3.x = centerX + size / 2;
    triangle->v3.y = centerY + size / 2;
    triangle->type = TRIANGLE_POINT;
}

void Options(s8 font_id) {
    // Display the text labels
    AEGfxPrint(font_id, "Music", -0.5f, 0.5f, 0.2f, 1, 1, 1, 1);
    AEGfxPrint(font_id, "Sound", -0.5f, -0.5f, 0.2f, 1, 1, 1, 1);

    // Create strings for the values
    char musicValueStr[10];
    char soundValueStr[10];
    sprintf_s(musicValueStr, "%d", musicValue);
    sprintf_s(soundValueStr, "%d", soundValue);

    // Display the current values
    AEGfxPrint(font_id, musicValueStr, 0.23f, 0.5f, 0.2f, 1, 1, 1, 1);
    AEGfxPrint(font_id, soundValueStr, 0.23f, -0.5f, 0.2f, 1, 1, 1, 1);
    AEGfxPrint(font_id, "Press B to quit", 0.4f, 0.9f, 0.2f, 1, 1, 1, 1);
}

void SoundUp(AEGfxVertexList* Mesh) {
    // Set blend color
    AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
    AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 0.0f);

    // Transformation Matrices
    AEMtx33 SoundUp_transform;
    AEMtx33Identity(&SoundUp_transform);

    AEMtx33 SoundUp_spin;
    AEMtx33RotDeg(&SoundUp_spin, 0.0f);

    AEMtx33 SoundUp_scale;
    AEMtx33Scale(&SoundUp_scale, 100.0f, 100.0f);  // Uniform scaling

    AEMtx33 SoundUp_translate;
    AEMtx33Trans(&SoundUp_translate, 400.0f, -225.0f);  // Adjust position

    AEMtx33Concat(&SoundUp_transform, &SoundUp_spin, &SoundUp_scale);
    AEMtx33Concat(&SoundUp_transform, &SoundUp_translate, &SoundUp_transform);

    AEGfxSetTransform(SoundUp_transform.m);

    // Draw triangle mesh
    AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);

    // Update the collider for this triangle button
    CreateTriangleCollider(&soundUpButton, 400.0f, -225.0f, 100.0f);
}

void SoundDown(AEGfxVertexList* Mesh) {
    // Set blend color
    AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
    AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 0.0f);

    // Transformation Matrices
    AEMtx33 SoundDown_transform;
    AEMtx33Identity(&SoundDown_transform);

    AEMtx33 SoundDown_spin;
    AEMtx33RotDeg(&SoundDown_spin, 180.0f); // Rotated to point down

    AEMtx33 SoundDown_scale;
    AEMtx33Scale(&SoundDown_scale, 100.0f, 100.0f);  // Uniform scaling

    AEMtx33 SoundDown_translate;
    AEMtx33Trans(&SoundDown_translate, 0.0f, -225.0f);  // Adjust position

    AEMtx33Concat(&SoundDown_transform, &SoundDown_spin, &SoundDown_scale);
    AEMtx33Concat(&SoundDown_transform, &SoundDown_translate, &SoundDown_transform);

    AEGfxSetTransform(SoundDown_transform.m);

    // Draw triangle mesh
    AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);

    // Update the collider for this triangle button
    CreateDownTriangleCollider(&soundDownButton, 0.0f, -225.0f, 100.0f);
}

void MusicDown(AEGfxVertexList* Mesh) {
    // Set blend color
    AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
    AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 0.0f);

    // Transformation Matrices
    AEMtx33 MusicDown_transform;
    AEMtx33Identity(&MusicDown_transform);

    AEMtx33 MusicDown_spin;
    AEMtx33RotDeg(&MusicDown_spin, 180.0f); // Rotated to point down

    AEMtx33 MusicDown_scale;
    AEMtx33Scale(&MusicDown_scale, 100.0f, 100.0f);  // Uniform scaling

    AEMtx33 MusicDown_translate;
    AEMtx33Trans(&MusicDown_translate, 0.0f, 225.0f);  // Adjust position

    AEMtx33Concat(&MusicDown_transform, &MusicDown_spin, &MusicDown_scale);
    AEMtx33Concat(&MusicDown_transform, &MusicDown_translate, &MusicDown_transform);

    AEGfxSetTransform(MusicDown_transform.m);

    // Draw triangle mesh
    AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);

    // Update the collider for this triangle button
    CreateDownTriangleCollider(&musicDownButton, 0.0f, 225.0f, 100.0f);
}

void MusicUp(AEGfxVertexList* Mesh) {
    // Set blend color
    AEGfxSetBlendColor(0.0f, 0.0f, 0.0f, 1.0f);
    AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 0.0f);

    // Transformation Matrices
    AEMtx33 MusicUp_transform;
    AEMtx33Identity(&MusicUp_transform);

    AEMtx33 MusicUp_spin;
    AEMtx33RotDeg(&MusicUp_spin, 0.0f); // Upward pointing triangle

    AEMtx33 MusicUp_scale;
    AEMtx33Scale(&MusicUp_scale, 100.0f, 100.0f);  // Uniform scaling

    AEMtx33 MusicUp_translate;
    AEMtx33Trans(&MusicUp_translate, 400.0f, 225.0f);  // Adjust position

    AEMtx33Concat(&MusicUp_transform, &MusicUp_spin, &MusicUp_scale);
    AEMtx33Concat(&MusicUp_transform, &MusicUp_translate, &MusicUp_transform);

    AEGfxSetTransform(MusicUp_transform.m);

    // Draw triangle mesh
    AEGfxMeshDraw(Mesh, AE_GFX_MDM_TRIANGLES);

    // Update the collider for this triangle button
    CreateTriangleCollider(&musicUpButton, 400.0f, 225.0f, 100.0f);
}

static AEGfxVertexList* OptionsMesh = nullptr;
static AEGfxVertexList* TriangleMesh = nullptr;
static s8 OptionsFontID = -1;
static Mouse OptionsMouse;

void Options_Load()
{
    // Create rectangle mesh
    AEGfxMeshStart();
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    OptionsMesh = AEGfxMeshEnd();

    // Create triangle mesh
    AEGfxMeshStart();
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.0f, 0.5f, 0xFFFFFFFF, 0.5f, 0.0f);
    TriangleMesh = AEGfxMeshEnd();

    // Load font
    OptionsFontID = AEGfxCreateFont("Assets/liberation-mono.ttf", 250);

    // Initialize the triangle colliders for the buttons
    CreateTriangleCollider(&musicUpButton, 400.0f, 225.0f, 100.0f);
    CreateDownTriangleCollider(&musicDownButton, 0.0f, 225.0f, 100.0f);
    CreateTriangleCollider(&soundUpButton, 400.0f, -225.0f, 100.0f);
    CreateDownTriangleCollider(&soundDownButton, 0.0f, -225.0f, 100.0f);
}

void Options_Initialize()
{
    // Initialize mouse position
    OptionsMouse.mouseX = 0;
    OptionsMouse.mouseY = 0;

    // Initialize music and sound values
    musicValue = 50;
    soundValue = 50;
}

void CheckButtonClicks() {
    // Only process clicks when the left mouse button is pressed
    if (AEInputCheckTriggered(AEVK_LBUTTON)) {
        AEVec2 mousePos = OptionsMouse.vec_Mouse;

        // Check music up button
        if (CheckTrianglePointCollision(&musicUpButton, &mousePos)) {
            musicValue += 5;
            if (musicValue > 100) musicValue = 100; // Cap at 100
            PlayGameSound(CLICK);
        }

        // Check music down button
        if (CheckTrianglePointCollision(&musicDownButton, &mousePos)) {
            musicValue -= 5;
            if (musicValue < 0) musicValue = 0; // Minimum 0
            PlayGameSound(CLICK);
        }

        // Check sound up button
        if (CheckTrianglePointCollision(&soundUpButton, &mousePos)) {
            soundValue += 5;
            if (soundValue > 100) soundValue = 100; // Cap at 100
            PlayGameSound(CLICK);
        }

        // Check sound down button
        if (CheckTrianglePointCollision(&soundDownButton, &mousePos)) {
            soundValue -= 5;
            if (soundValue < 0) soundValue = 0; // Minimum 0
            PlayGameSound(CLICK);
        }
    }
}

void Options_Update()
{
    // Set background color
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Set render mode
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // Set blend color
    AEGfxSetBlendColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Get mouse position
    AEInputGetCursorPosition(&OptionsMouse.mouseX, &OptionsMouse.mouseY);
    OptionsMouse.mouseX -= (AEGfxGetWindowWidth() / 2);
    OptionsMouse.mouseY = (AEGfxGetWindowHeight() / 2) - OptionsMouse.mouseY;
    OptionsMouse.vec_Mouse = { static_cast<f32>(OptionsMouse.mouseX), static_cast<f32>(OptionsMouse.mouseY) };

    // Check for button clicks
    CheckButtonClicks();

    UpdateAudioVolumes();

    // Check for back button press (B key)
    if (AEInputCheckTriggered(AEVK_B) || AEInputCheckTriggered(AEVK_ESCAPE))
    {
        next = GS_MENU;
    }
}

void Options_Draw()
{
    // Use existing functions to draw the UI elements
    SoundUp(TriangleMesh);
    SoundDown(TriangleMesh);
    MusicUp(TriangleMesh);
    MusicDown(TriangleMesh);

    // Call the original Options function with the font ID
    Options(OptionsFontID);
}

int GetSoundVolume() {
    return soundValue;
}
int GetMusicVolume() {
    return musicValue;
}

void Options_Free()
{
    // Free temporary resources for the current session
}

void Options_Unload()
{
    // Free meshes
    if (OptionsMesh) {
        AEGfxMeshFree(OptionsMesh);
        OptionsMesh = nullptr;
    }

    if (TriangleMesh) {
        AEGfxMeshFree(TriangleMesh);
        TriangleMesh = nullptr;
    }

    // Free font
    if (OptionsFontID >= 0) {
        AEGfxDestroyFont(OptionsFontID);
        OptionsFontID = -1;
    }
}