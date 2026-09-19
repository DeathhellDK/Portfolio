/**
* @file		  PlayStopManager.cpp
* @author     Woh Kye Le
* @email      w.kyele
* @date		  2026 - 01 - 25
*
* @brief      
* Implementation of editor Play / Pause / Stop UI helpers.
*
* This file contains the ImGui rendering logic and state-application
* functions used by the editor play control bar.
*
* @version 1.0
* @copyright
* Copyright (C) 2026 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/
#ifndef ENABLE_EDITOR
#define ENABLE_EDITOR 0
#endif

#if ENABLE_EDITOR

#include "Editor/PlayStopManager.h"
#include "imgui.h" 
#include "imgui_internal.h"


/**
 * @brief Draws the editor play control buttons.
 *
 * The UI layout adapts based on whether the editor is currently:
 *  - Stopped
 *  - Playing
 *  - Paused
 *
 * This function only reports intent; it does not change state.
 */
PlayControlsUIResult PlayStopSystem::DrawPlayControls(EditorPlayControlsState& s, const EditorPlayControlsCallbacks& /*cb*/, float btnW, float btnH, float gap) {

    PlayControlsUIResult r{};

    // Center the 3 buttons
    float fullW = ImGui::GetContentRegionAvail().x;
    float totalW = btnW * 3.0f + gap * 2.0f;
    if (fullW > totalW)
        ImGui::SetCursorPosX((fullW - totalW) * 0.5f);

    const char* pauseLabel = s.isPaused ? "Resume" : "Pause";

    // STOPPED
    if (!s.isPlaying) {
        if (ImGui::Button("Play", ImVec2(btnW, btnH))) {
            //if (cb.onPlay) cb.onPlay();
            r.action = PlayControlAction::Play;
            r.pressed = true;
        }
        ImGui::SameLine(0.0f, gap);

        ImGui::BeginDisabled(true);
        ImGui::Button(pauseLabel, ImVec2(btnW, btnH));
        ImGui::SameLine(0.0f, gap);
        ImGui::Button("Stop", ImVec2(btnW, btnH));
        ImGui::EndDisabled();
    }
    else { // PLAYING
        ImGui::BeginDisabled(true);
        ImGui::Button("Play", ImVec2(btnW, btnH));
        ImGui::EndDisabled();
        ImGui::SameLine(0.0f, gap);

        if (ImGui::Button(pauseLabel, ImVec2(btnW, btnH))) {
            //if (cb.onPause) cb.onPause();
            r.action = PlayControlAction::TogglePause;
            r.pressed = true;
        }
        ImGui::SameLine(0.0f, gap);

        if (ImGui::Button("Stop", ImVec2(btnW, btnH))) {
           // if (cb.onStop) cb.onStop();
            r.action = PlayControlAction::Stop;
            r.pressed = true;
        }
    }

    return r;
}

/**
 * @brief Applies a play control action and invokes callbacks.
 *
 * This function mutates editor state and forwards the action
 * to the owning system (e.g. SceneManager).
 */
bool PlayStopSystem::UpdatePlayControls(
    EditorPlayControlsState& s,
    const EditorPlayControlsCallbacks& cb,
    PlayControlAction action){

    switch (action)
    {
    case PlayControlAction::Play:
    {
        s.isPlaying = true;
        s.isPaused = false;
        if (cb.onPlay) cb.onPlay();
        return true;
    }

    case PlayControlAction::TogglePause:
    {
        // Only meaningful if currently playing
        if (!s.isPlaying) return false;

        s.isPaused = !s.isPaused;
        if (cb.onPause) cb.onPause(); 
        return true;
    }

    case PlayControlAction::Stop:
    {
        s.isPlaying = false;
        s.isPaused = false;
        if (cb.onStop) cb.onStop();
        return true;
    }

    case PlayControlAction::None:
    default:
        return false;
    }
}
#endif