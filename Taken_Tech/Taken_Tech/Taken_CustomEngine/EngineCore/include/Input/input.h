#pragma once
/**
 * @file    input.h
 * @author  Sng Swee Yong Dillon
 * @email   sweeyongdillon.sng
 * @date    2025-09-29
 *
 * @brief   Declares the eng::Input class for handling keyboard and mouse input.
 *
 * This class wraps GLFW input polling into an object-oriented interface.
 * It tracks both current and previous states of keys and mouse buttons,
 * allowing detection of "pressed", "released", and "held" events. It also
 * provides mouse position and scroll wheel values.
 *
 * @version 1.0
 */
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "Math/vect2.h"

namespace eng
{
	/**
	 * @class Input
	 * @brief Handles real-time keyboard and mouse input using GLFW.
	 *
	 * Provides:
	 * - Key and mouse button state queries (`isDown`, `isPressed`, `isReleased`).
	 * - Mouse cursor position.
	 * - Scroll wheel offsets.
	 *
	 * Usage:
	 * @code
	 * eng::Input input(window);
	 * while (running) {
	 *     input.beginFrame(); // must be called once per frame
	 *     if (input.isKeyPressed(GLFW_KEY_SPACE)) { ... }
	 *     Vector2 pos = input.getMousePos();
	 * }
	 * @endcode
	 */
	class Input {
		public:
			/**
			 * @brief Constructs an Input handler bound to a GLFW window.
			 * @param win Pointer to GLFWwindow to poll input from.
			 */
			explicit Input(GLFWwindow* win);
			//keyboard check
			bool isKeyDown(int key) const;
			bool isKeyPressed(int key) const;
			bool isKeyReleased(int key) const;

			//mouse button check
			bool isMouseButtonDown(int button) const;
			bool isMouseButtonPressed(int button) const;
			bool isMouseButtonReleased(int button) const;

			//mouse pos
			Vector2 getMousePos() const;
			Vector2 getMouseScroll() const;
			/**
			 * @brief Updates input states at start of frame.
			 *
			 * Copies current states into previous arrays, polls GLFW for
			 * latest key and button values, and resets scroll offsets.
			 */
			void beginFrame(); // Call at the start of each frame to update states

			/**
			 * @brief GLFW callback for scroll events.
			 * @param window Pointer to GLFW window.
			 * @param xoffset Horizontal scroll offset.
			 * @param yoffset Vertical scroll offset.
			 */
			static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

			/*
			* @brief check if the gamepad is connected.
			*/
			void getGamePad();

			/*
			* @brief Checks if a gamepad is currently connected.
			*/
			bool isGamepadConnected() const;

			/*
			* @brief Retrieves the current position of the left analog stick, applying a deadzone.
			*/
			Vector2 getGamepadLeftStick() const;

			/*
			* @brief Retrieves the current position of the right analog stick, 
			* applying a deadzone.
			*/
			Vector2 getGamepadRightStick() const;

			/*
			* @brief Checks if a specific gamepad button is currently 
			* held down or was just pressed.
			* @param button The GLFW gamepad button index to check.
			*/
			bool isGamepadButtonDown(int button) const;

			/*
			* @brief Checks if a specific gamepad button was just pressed this frame.
			* @param button The GLFW gamepad button index to check.
			*/
			bool isGamepadButtonPressed(int button) const;

		private:
			GLFWwindow* win;//Window associated with this input handler.
			// Store previous states to detect presses and releases
			bool mKeys[GLFW_KEY_LAST+1]{};// Array to store key states
			bool prevKeys[GLFW_KEY_LAST + 1]{};// Array to store previous key states
			bool mouseButtonStates[GLFW_MOUSE_BUTTON_LAST + 1]{}; // Array to store mouse button states
			bool prevMouseButtonStates[GLFW_MOUSE_BUTTON_LAST + 1]{}; // Array to store previous mouse button states

			// Scroll offsets
			Vector2 scrollOffset{ 0.0f, 0.0f };

			/// State for gamepad input
			GLFWgamepadstate mGamepadState{};

			/// Check if gamepad is connected
			bool mGamepadConnected = false;

			/// Store previou gamepad button states for press/release detection
			unsigned char prevGamepadButtons[GLFW_GAMEPAD_BUTTON_LAST + 1]{};

			/// Store current gamepad button states
			unsigned char gamepadButtons[GLFW_GAMEPAD_BUTTON_LAST + 1]{};
	};
}
