/**
 * @file    window.cpp
 * @author  Jethro Sung
 * @email     w.kyele, sung.h, t.weiliangterril
 * @co-author Woh Kye Le, Tan Wei Liang Terril
 *
 * @brief   Implements the Window class declared in window.h.
 *
 * Provides creation and destruction of a GLFW window, initialization
 * of the OpenGL context via GLAD, handling of framebuffer resize events,
 * and frame-level polling/swap operations.
 *
 * @version 1.0
 */
#include "Core/window.h"
#include <stdexcept>//exceptions
//#include <iostream>//debug 
#include "Input/DebugConsole.hpp"
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

 /**
  * @brief Creates a new window and initializes OpenGL context.
  *
  * Steps performed:
  * - Initializes GLFW and configures OpenGL context hints.
  * - Creates a window with parameters stored in Window::Desc.
  * - Makes the context current and initializes GLAD.
  * - Registers framebuffer resize callback and sets viewport.
  * - Applies 2D-friendly defaults (blending enabled, depth/culling disabled).
  *
  * @return True if creation succeeded, otherwise throws std::runtime_error.
  * @throws std::runtime_error If GLFW or GLAD initialization fails.
  */
bool Window::create() {
	if (!glfwInit()) {
		//DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "Failed to init GLFW\n");
		throw std::runtime_error("Failed to init GLFW");
		//return false;
	}

	//config openGl Context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);//version 3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);//ver 3.3
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);//only modern funcs


	//mWindow = glfwCreateWindow(mDesc.w, mDesc.h, mDesc.title.c_str(), nullptr, nullptr);

	GLFWmonitor* monitor = mDesc.fullscreen ? glfwGetPrimaryMonitor() : nullptr;

	if (monitor) {
		// Fullscreen: use desktop resolution
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		mWindow = glfwCreateWindow(mode->width, mode->height,
			mDesc.title.c_str(), monitor, nullptr);
	}
	else {
		// Windowed: use specified resolution
		mWindow = glfwCreateWindow(mDesc.w, mDesc.h,
			mDesc.title.c_str(), nullptr, nullptr);

		// Optional: make rubric-style windowed mode non-resizable
		glfwSetWindowAttrib(mWindow, GLFW_RESIZABLE, GLFW_FALSE);
	}

	if (!mWindow) {
		glfwTerminate();
		//DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "Failed to create GLFW window\n");
		throw std::runtime_error("Failed to create GLFW window");
		//return false;
	}

	//Make context curr
	glfwMakeContextCurrent(mWindow);
	glfwSwapInterval(mDesc.vsync ? 1 : 0); // Enable vsync if requested

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		glfwDestroyWindow(mWindow);
		glfwTerminate();
		throw std::runtime_error("Failed to init GLAD");
	}

	// Track Framebuffer size changes, and set callback
	glfwSetWindowUserPointer(mWindow, this);
	glfwSetFramebufferSizeCallback(mWindow, framebufferCB);
	int fbw = 0, fbh = 0;
	glfwGetFramebufferSize(mWindow, &fbw, &fbh);
	if (fbw <= 0 || fbh <= 0) {
		fbw = mDesc.w;
		fbh = mDesc.h;
	}

	// Use the callback to update mWidth/mHeight and viewport
	framebufferCB(mWindow, fbw, fbh);
	glViewport(0, 0, fbw, fbh);
	//framebufferCB(mWindow, mDesc.w, mDesc.h); // Set initial viewport size
	//glViewport(0, 0, mDesc.w, mDesc.h);

	// Basic 2D defaults enable blending
	glDisable(GL_DEPTH_TEST);   // no z-buffer for 2D
	glDisable(GL_CULL_FACE);    // don�t cull backfaces
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "Window Created: " + mDesc.title, " Width: " + std::to_string(mDesc.w), " Height: " + std::to_string(mDesc.h), "\n");
	DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "Open GL Ver: " + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION))), "\n");
	return true;
}

/**
 * @brief Destroys the GLFW window and terminates GLFW.
 */
void Window::destroy() {
	if (mWindow) {
		glfwDestroyWindow(mWindow);
		mWindow = nullptr;
	}
	glfwTerminate();
}

/**
 * @brief Polls events from the OS (keyboard, mouse, resize, etc.).
 */
void Window::poll() {
	glfwPollEvents();
}

/**
 * @brief Swaps the front and back buffers.
 *
 * Used once per frame after rendering to present the frame to screen.
 */
void Window::swap() {
	glfwSwapBuffers(mWindow);
}

/**
 * @brief Checks if the user has requested the window to close.
 * @return True if the window should close, false otherwise.
 */
bool Window::ShouldClose() const {
	return glfwWindowShouldClose(mWindow);
}

/**
 * @brief Callback triggered on framebuffer resize.
 *
 * Updates the stored window width/height and resets the OpenGL viewport.
 *
 * @param w Pointer to GLFWwindow.
 * @param fbw New framebuffer width.
 * @param fbh New framebuffer height.
 */

void Window::framebufferCB(GLFWwindow* w, int fbw, int fbh) {
	auto* win = static_cast<Window*>(glfwGetWindowUserPointer(w));
	if (!win)
	{
		return;
	}
	if (win) {
		win->mWidth = fbw;
		win->mHeight = fbh;
		glViewport(0, 0, fbw, fbh);
	}
}

void Window::setFullscreen(bool fullscreen, int windowWidth, int windowHeight) {
	if (!mWindow) return;

	mDesc.fullscreen = fullscreen;

	if (fullscreen) {
		// Switch to fullscreen on primary monitor
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		if (!monitor) return;

		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		if (!mode) return;

		// Go fullscreen at the monitor's native resolution
		glfwSetWindowMonitor(
			mWindow,
			monitor,
			0, 0,
			mode->width,
			mode->height,
			mode->refreshRate
		);
	}
	else {
		// Switch to windowed mode at the given config resolution,
		// centered on the primary monitor.
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		int xpos = 100;
		int ypos = 100;
		if (monitor) {
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			if (mode) {
				xpos = (mode->width - windowWidth) / 2;
				ypos = (mode->height - windowHeight) / 2;
			}
		}

		glfwSetWindowMonitor(
			mWindow,
			nullptr,       // windowed
			xpos, ypos,
			windowWidth,
			windowHeight,
			0             // refresh rate ignored for windowed
		);

		// Match what you do on windowed create
		glfwSetWindowAttrib(mWindow, GLFW_RESIZABLE, GLFW_FALSE);
	}

	// Framebuffer resize callback will update mWidth/mHeight + glViewport,
	// but we can force one immediately in case it doesn't fire:
	int fbw = 0, fbh = 0;
	glfwGetFramebufferSize(mWindow, &fbw, &fbh);
	if (fbw > 0 && fbh > 0) {
		framebufferCB(mWindow, fbw, fbh);
	}
}