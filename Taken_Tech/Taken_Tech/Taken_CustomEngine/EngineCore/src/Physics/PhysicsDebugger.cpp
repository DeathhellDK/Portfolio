/**
* @file PhysicsDebugger.cpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-09-26
*
* @brief Physics Debugger class implementation.
* This class provides static methods to toggle physics debugging features, handle step-by-step execution of physics updates, and log collision events.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Physics/PhysicsDebugger.h"
#include "Input/DebugConsole.hpp"
#include <iostream>

/// Indicates whether physics debugging is currently active
bool PhysicsDebug::isDebugTrigger = false;

/// Indicates whether a step execution has been requested
bool PhysicsDebug::s_stepRequested = false;

/// Indicates whether a step execution is active for the current frame
bool PhysicsDebug::s_stepActiveThisFrame = false;

/// Previous state of the step key to detect key presses
bool PhysicsDebug::s_prevStepKeyState = false;
bool PhysicsDebug::s_prevToggleKeyState = false;

/**
* @brief Toggle the physics debugging feature on or off.
*/
void PhysicsDebug::ToggelePhysicsDebug() {

	/// Toggle the debug state
	isDebugTrigger = !isDebugTrigger;

	/// Log the current state of the physics debugger
	DebugConsole::Get().AddFormattedMessage(
		LogLevel::Info, "[Physics Debug]", "Physics Debugger is " + std::string(isDebugTrigger ? "ON" : "OFF") + "\n"
	);
}


/**
* @brief Check if physics debugging is currently active.
*/
bool PhysicsDebug::IsDebugActive() {

	// Return the current state of the physics debugger
	return isDebugTrigger;
}

/**
* @brief Handle input for step-by-step execution of physics updates.
*
* @param in The input manager to check for key presses.
*/
void PhysicsDebug::HandleStepKeyInput(const eng::Input& in) {

	/// Only process input if debugging is active
	bool key = in.isKeyDown(GLFW_KEY_W) ||
		in.isKeyDown(GLFW_KEY_A) ||
		in.isKeyDown(GLFW_KEY_S) ||
		in.isKeyDown(GLFW_KEY_D);

	/// If the step key is pressed and was not pressed in the previous frame, request a step execution
	if (key && !s_prevStepKeyState) {
		DebugConsole::Get().Info("[Physics Debug] Step exectuted\n");
		s_stepRequested = true;
	}

	/// Update the previous key state for the next frame
	s_prevStepKeyState = key;
}

void PhysicsDebug::HandleDebugToggle(const eng::Input& in) {
	bool toggleDown = in.isKeyDown(GLFW_KEY_F11);
	if (toggleDown && !s_prevToggleKeyState) {
		ToggelePhysicsDebug();
	}
	s_prevToggleKeyState = toggleDown;
}
/**
* @brief Determine if physics updates should proceed for the current frame.
*/
bool PhysicsDebug::BeginFrameAllowStep() {

	/// If debugging is not active, allow physics updates to proceed
	if (!isDebugTrigger) {
		s_stepActiveThisFrame = true;
		return true;
	}

	/// If a step execution has been requested, allow physics updates to proceed for this frame
	if (s_stepRequested) {
		s_stepRequested = false;
		s_stepActiveThisFrame = true;
		return true;
	}
	/// If no step execution is requested, do not allow physics updates
	s_stepActiveThisFrame = false;
	return false;
}

/**
* @brief Reset the step execution state at the end of the frame.
*/
bool PhysicsDebug::IsStepActiveThisFrame() {

	/// Return whether a step execution is active for this frame
	return s_stepActiveThisFrame;
}

/**
* @brief Stop the step state at the end of the frame.
*/
void PhysicsDebug::EndFrameStop() {
	/// Reset the step execution state for the next frame
	s_stepActiveThisFrame = false;
}

/**
* @brief Print collision information between two entities.
*
* @param gameObject1 The first entity involved in the collision.
* @param gameObject2 The second entity involved in the collision.
* @param checkCollided A boolean indicating whether a collision occurred.
*/
void PhysicsDebug::printTriggerCollsion(const Entity& gameObject1, const Entity& gameObject2, bool checkCollided) {

	/// Only log collision events if debugging is active
	if (!isDebugTrigger) return;

	/// If a collision occurred, log the entities involved
	if (checkCollided == true) {
		DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Collision] Entity ", gameObject1, " overlapped with ", gameObject2, "\n");
		checkCollided = false;
	}
}
