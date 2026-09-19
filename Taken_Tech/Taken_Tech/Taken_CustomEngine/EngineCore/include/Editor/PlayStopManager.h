#pragma once 
/**
* @file		  PlayStopManager.h
* @author     Woh Kye Le
* @email      w.kyele
* @date		  2026 - 01 - 25
*
* @brief - Editor play / pause / stop control helpers.
*
* This module provides:
*  - Lightweight state structs for editor simulation state
*  - Callback hooks for Play / Pause / Stop actions
*  - UI helpers to draw ImGui play controls
*  - A state-application function to execute the selected action
* @version 1.0
* @copyright
* Copyright (C) 2026 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include <functional>

/**
 * @struct EditorPlayControlsState
 * @brief Represents the current editor simulation state.
 *
 * This struct mirrors the high-level editor play state
 * and is used purely for UI presentation and logic gating.
 */
struct EditorPlayControlsState {
    bool isPlaying = false;
    bool isPaused = false;
};

/**
 * @struct EditorPlayControlsCallbacks
 * @brief Callback hooks invoked when play control actions occur.
 *
 * These callbacks are typically bound to SceneManager or GameApp
 * functions that enqueue editor commands or change simulation state.
 */
struct EditorPlayControlsCallbacks {
    std::function<void()> onPlay;
    std::function<void()> onPause;
    std::function<void()> onStop;
};

/**
 * @enum PlayControlAction
 * @brief Logical play control actions produced by the UI.
 *
 * This enum decouples UI interaction from execution logic.
 */
enum class PlayControlAction {
    None,
    Play,
    TogglePause,
    Stop
};

/**
 * @struct PlayControlsUIResult
 * @brief Result returned from drawing the play controls UI.
 *
 * Indicates whether a button was pressed and which action
 * was requested by the user.
 */
struct PlayControlsUIResult {
    PlayControlAction action = PlayControlAction::None;
    bool pressed = false;
};


namespace PlayStopSystem {
    
    /**
     * @brief Draws the Play / Pause / Stop buttons using ImGui.
     *
     * This function:
     *  - Renders the appropriate button states based on `s`
     *  - Detects user interaction
     *  - Does not modify state or invoke callbacks
     *
     * @param[in]  s     Current editor play state (read-only usage)
     * @param[in]  cb    Callback set (not invoked here)
     * @param[in]  btnW  Button width in pixels
     * @param[in]  btnH  Button height in pixels
     * @param[in]  gap   Horizontal spacing between buttons
     *
     * @return PlayControlsUIResult describing the requested action
     */
    PlayControlsUIResult  DrawPlayControls( EditorPlayControlsState& s, const EditorPlayControlsCallbacks& cb, float btnW = 110.0f, float btnH = 28.0f, float gap = 10.0f);


    /**
     * @brief Applies a play control action to the editor state and fires callbacks.
     *
     * This function:
     *  - Mutates the editor play state
     *  - Invokes the appropriate callback (Play / Pause / Stop)
     *  - Performs no UI rendering
     *
     * Intended to be called AFTER DrawPlayControls(),
     * using the returned PlayControlsUIResult.
     *
     * @param[in,out] s      Editor play state to update
     * @param[in]     cb     Callback hooks to execute
     * @param[in]     action Action to apply
     *
     * @return true if the action caused a state change, false otherwise
     */
    bool UpdatePlayControls( EditorPlayControlsState& s, const EditorPlayControlsCallbacks& cb, PlayControlAction action);
}