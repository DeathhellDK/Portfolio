/**
 * @file      PhysicsDebugger.hpp
 * @author	  Tan Wei Liang Terril
 * @email	  t.weiliangterril
 * @date	  04/04/26
 *
 * @brief     Declares the PhysicsDebug helper used to inspect and step through
 *            physics and collision behavior at runtime.
 *
 *            This utility provides toggles for physics debug mode, frame-step
 *            controls, and logging helpers for trigger/collision events to aid
 *            debugging during gameplay and engine testing.
*/
#ifndef PHYSICS_DEBUGGER_HPP
#define PHYSICS_DEBUGGER_HPP

#include <string>
#include <cstdint>
#include <iostream>
#include <GLFW/glfw3.h>
#include "Input/input.h"


using Entity = std::uint32_t;

class PhysicsDebug
{
	public:
		/** @brief Toggles the physics debug mode on or off. */
		static void ToggelePhysicsDebug();
		
		/**
		 * @brief Checks if physics debugging is currently active.
		 * @return True if active.
		 */
		static bool IsDebugActive();

		/**
		 * @brief Handles key input for stepping through physics frames.
		 * @param in The input state.
		 */
		static void HandleStepKeyInput(const eng::Input& in);
		
		/**
		 * @brief Determines if a physics step is allowed this frame.
		 * @return True if a step is allowed.
		 */
		static bool BeginFrameAllowStep();
		
		/**
		 * @brief Checks if a step was executed this frame.
		 * @return True if stepping was active.
		 */
		static bool IsStepActiveThisFrame();
		
		/** @brief Clears step flags at the end of the frame. */
		static void EndFrameClear();
		
		/**
		 * @brief Prints a debug message when a trigger collision occurs.
		 * @param gameObject1 The first entity involved.
		 * @param gameObject2 The second entity involved.
		 * @param checkCollided True if the entities collided.
		 */
		static void printTriggerCollsion(const Entity& gameObject1, const Entity& gameObject2, bool checkCollided);

	private:
		static bool isDebugTrigger;
		static bool s_stepRequested;
		static bool s_stepActiveThisFrame;
		static bool s_prevStepKeyState;
		static constexpr int STEP_KEY = GLFW_KEY_N;

};
#endif // !PHYSICS_DEBUGGER_HPP
