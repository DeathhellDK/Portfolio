/**
 * @file    input.cpp
 * @author  Sng Swee Yong Dillon
 * @email    sweeyongdillon.sng
 * @date    2025-09-29
 *
 * @brief   Implements the eng::Input class for keyboard/mouse handling.
 *
 * Provides frame-based tracking of key/button states, cursor position,
 * and scroll offsets. Uses GLFW to query real-time input state and
 * allows systems to detect presses/releases vs. continuous holds.
 *
 * @version 1.0
 */
#include "Input/input.h"
#include <cstring>//memcpy

namespace eng {

	static Input* sInstance = nullptr;// Singleton instance

	/**
		* @brief Constructs the input system and initializes keyboard/mouse state.
		*
		* Sets all key and mouse button state arrays to zero, stores the GLFW window
		* pointer, and registers this instance as the global singleton. This enables
		* systems to access input via Input::Get() without storing additional references.
		*
		* @param win Pointer to the GLFW window associated with this input context.
	*/
	Input::Input(GLFWwindow* win) : win(win)
	{
		// Initialize key and mouse button states
		memset(mKeys, 0, sizeof(mKeys));
		memset(prevKeys, 0, sizeof(prevKeys));
		memset(mouseButtonStates, 0, sizeof(mouseButtonStates));
		memset(prevMouseButtonStates, 0, sizeof(prevMouseButtonStates));
		memset(prevGamepadButtons, 0, sizeof(prevGamepadButtons));
		memset(gamepadButtons, 0, sizeof(gamepadButtons));
		sInstance = this;
		// Set the scroll callback
		//glfwSetScrollCallback(win, scrollCallback);
	}
	bool Input::isKeyDown(int key) const
	{
		return glfwGetKey(win, key) == GLFW_PRESS;
	}
	bool Input::isKeyPressed(int key) const
	{
		return isKeyDown(key) && !prevKeys[key];
	}
	bool Input::isKeyReleased(int key) const
	{
		return !isKeyDown(key) && prevKeys[key];
	}
	bool Input::isMouseButtonDown(int button) const
	{
		return glfwGetMouseButton(win, button) == GLFW_PRESS;
	}
	bool Input::isMouseButtonPressed(int button) const
	{
		return isMouseButtonDown(button) && !prevMouseButtonStates[button];
	}
	bool Input::isMouseButtonReleased(int button) const
	{
		return !isMouseButtonDown(button) && prevMouseButtonStates[button];
	}
	Vector2 Input::getMousePos() const
	{
		double xpos, ypos;
		glfwGetCursorPos(win, &xpos, &ypos);
		return Vector2(static_cast<float>(xpos), static_cast<float>(ypos));
	}
	Vector2 Input::getMouseScroll() const
	{
		return scrollOffset;
	}
	/**
		* @brief Updates input state at the start of each frame.
		*
		* Responsibilities:
		*  - Copies current key/mouse states into "previous" buffers.
		*  - Queries GLFW to refresh current key and mouse states.
		*  - Resets scrollOffset to (0,0).
		*
		* Must be called exactly once per frame to ensure correct pressed/released logic.
	*/
	void Input::beginFrame()
	{
		std::memcpy(prevKeys, mKeys, sizeof(mKeys));
		std::memcpy(prevMouseButtonStates, mouseButtonStates, sizeof(mouseButtonStates));
		std::memcpy(prevGamepadButtons, gamepadButtons, sizeof(gamepadButtons));
		// Update previous key states
		for (int i = 0; i <= GLFW_KEY_LAST; ++i)
			mKeys[i] = (glfwGetKey(win, i) == GLFW_PRESS);
		for (int i = 0; i <= GLFW_MOUSE_BUTTON_LAST; ++i)
			mouseButtonStates[i] = (glfwGetMouseButton(win, i) == GLFW_PRESS);
		// Reset scroll offset after each frame
		scrollOffset = Vector2(0.0f, 0.0f);

		getGamePad();
		if (mGamepadConnected)
		{
			for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i)
				gamepadButtons[i] = static_cast<unsigned char>(mGamepadState.buttons[i]);
		}
		else
		{
			memset(gamepadButtons, 0, sizeof(gamepadButtons));
		}
	}
	/**
		* @brief Global static scroll callback for GLFW.
		*
		* When GLFW scroll events occur, this updates the singleton Input instance's
		* scrollOffset without requiring external systems to manage window-user pointers.
		*
		* @param window (unused)
		* @param xoffset Horizontal scroll amount.
		* @param yoffset Vertical scroll amount.
	*/
	void Input::scrollCallback(GLFWwindow* /*window*/, double xoffset, double yoffset) {
		/*auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
		if (self) {
			self->scrollOffset.x = static_cast<float>(xoffset);
			self->scrollOffset.y = static_cast<float>(yoffset);
		}*/
		if (sInstance) {
			sInstance->scrollOffset.x = static_cast<float>(xoffset);
			sInstance->scrollOffset.y = static_cast<float>(yoffset);
		}
	}

	/// ---- Gamepad Support ----
	/*
	* @brief check if the gamepad is connected
	*/
	void Input::getGamePad() {
		mGamepadConnected = glfwJoystickPresent(GLFW_JOYSTICK_1) &&
			glfwGetGamepadState(GLFW_JOYSTICK_1, &mGamepadState);
		//mGamepadConnected = glfwGetGamepadState(GLFW_JOYSTICK_1, &mGamepadState);
	}

	/*
	* @brief Checks if a gamepad is currently connected.
	*/
	bool Input::isGamepadConnected() const{
		return mGamepadConnected;
	}

	/*
	* @brief Retrieves the current position of the left analog stick, applying a deadzone.
	*/
	Vector2 Input::getGamepadLeftStick() const
	{
		/// check if the gamepad is connected
		if (!mGamepadConnected)
			return Vector2(0.0f, 0.0f);

		/// Read raw stick values from the gamepad state
		float x = mGamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
		float y = mGamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];

		float deadzone = 0.2f;

		/// Apply deadzone to prevent drift when the stick is near the center
		if (fabs(x) < deadzone) x = 0;
		if (fabs(y) < deadzone) y = 0;

		return Vector2(x, y);
	}

	/*
	* @brief Retrieves the current position of the right analog stick,
	* applying a deadzone.
	*/
	Vector2 Input::getGamepadRightStick() const
	{
		/// Check if the gamepad is connected
		if (!mGamepadConnected)
			return Vector2(0.0f, 0.0f);

		/// Read raw stick values from the gamepad state
		float x = mGamepadState.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
		float y = mGamepadState.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];

		float deadzone = 0.2f;

		/// Apply deadzone to prevent drift when the stick is near the center
		if (fabs(x) < deadzone) x = 0;
		if (fabs(y) < deadzone) y = 0;

		return Vector2(x, y);
	}

	/*
	* @brief Checks if a specific gamepad button is currently
	* held down or was just pressed.
	* @param button The GLFW gamepad button index to check.
	*/
	bool Input::isGamepadButtonDown(int button) const
	{
		/// Check if the gamepad is connected
		if (!mGamepadConnected) return false;

		return gamepadButtons[button] == GLFW_PRESS;
	}

	/*
	* @brief Checks if a specific gamepad button was just pressed this frame.
	* @param button The GLFW gamepad button index to check.
	*/
	bool Input::isGamepadButtonPressed(int button) const
	{
		if (!mGamepadConnected) return false;

		bool current = (gamepadButtons[button] == GLFW_PRESS);
		bool prev = (prevGamepadButtons[button] == GLFW_PRESS);
		return current && !prev;
	}

}
