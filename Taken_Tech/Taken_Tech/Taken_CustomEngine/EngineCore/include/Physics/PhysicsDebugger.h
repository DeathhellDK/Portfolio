/**
* @file PhysicsDebugger.h
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-09-26
*
* @brief Header file for Physics Debugger class.
* This class provides static methods to toggle physics debugging features, handle step-by-step execution of physics updates, and log collision events.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/


#ifndef PHYSICS_DEBUGGER_H /// Include Header Guard
#define PHYSICS_DEBUGGER_H

#include <string>
#include <cstdint>
#include <iostream>
#include <GLFW/glfw3.h>
#include "Input/input.h"


using Entity = std::uint32_t;

/**
* @class PhysicsDebug
* @brief A manager class for handling physics debugging features.
* This class provides static methods to toggle physics debugging, handle step-by-step execution of physics updates, and log collision events.
* It includes methods to check if debugging is active, manage step execution, and print collision information.
*
* Usage:
* - Call `ToggelePhysicsDebug` to enable or disable physics debugging.
* - Use `HandleStepKeyInput` to process input for step execution.
* - Call `BeginFrameAllowStep` at the start of each frame to determine if physics updates should proceed.
* - Use `EndFrameClear` at the end of each frame to reset step state.
* - Call `printTriggerCollsion` to log collision events between entities.
*/
class PhysicsDebug
{
public:
	/**
	* @brief Toggle the physics debugging feature on or off.
	*/
	static void ToggelePhysicsDebug();

	/**
	* @brief Check if physics debugging is currently active.
	*/
	static bool IsDebugActive();

	/**
	* @brief Handle input for step-by-step execution of physics updates.
	*
	* @param in The input manager to check for key presses.
	*/
	static void HandleStepKeyInput(const eng::Input& in);
	/**
	* @brief Handle input for toggling physics debug mode (F11).
	*/
	static void HandleDebugToggle(const eng::Input& in);

	/**
	* @brief Determine if physics updates should proceed for the current frame.
	*/
	static bool BeginFrameAllowStep();

	/**
	* @brief Check if a step execution is active for the current frame.
	*/
	static bool IsStepActiveThisFrame();

	/**
	* @brief Stop the step state at the end of the frame.
	*/
	static void EndFrameStop();

	/**
	* @brief Print collision information between two entities.
	*
	* @param gameObject1 The first entity involved in the collision.
	* @param gameObject2 The second entity involved in the collision.
	* @param checkCollided A boolean indicating whether a collision occurred.
	*/
	static void printTriggerCollsion(const Entity& gameObject1, const Entity& gameObject2, bool checkCollided);

private:
	/**
	* @brief Indicates whether physics debugging is currently active.
	*/
	static bool isDebugTrigger;

	/**
	* @brief Indicates whether a step execution has been requested.
	*/
	static bool s_stepRequested;

	/**
	* @brief Indicates whether a step execution is active for the current frame.
	*/
	static bool s_stepActiveThisFrame;

	/**
	* @brief Previous state of the step key to detect key presses.
	*/
	static bool s_prevStepKeyState;
	/**
	* @brief Previous state of the toggle key (F11) to detect edge presses.
	*/
	static bool s_prevToggleKeyState;
};
#endif // !PHYSICS_DEBUGGER_H
