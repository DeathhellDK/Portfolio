#pragma once
/**
 * @file    window.h
 * @author  Jethro
 * @email     w.kyele, sung.h
 * @co-author Woh Kye Le
 * @date    2025-09-29
 *
 * @brief   Declares the Window class, a wrapper around GLFW.
 *
 * Provides creation, destruction, frame management, and queries
 * for an OpenGL-capable window using GLFW. Encapsulates width,
 * height, vsync, and title settings.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */


#include <string>
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>//lib for wind

 /**
  * @class Window
  * @brief Encapsulates a GLFW window for rendering and input.
  *
  * Provides methods for creating and destroying the window,
  * handling frame updates, and querying dimensions and close state.
  */
class Window {
	public:
		struct Desc {
			int w = 1280;
			int h = 720;
			std::string title = "2D Window";
			bool vsync = true;
			bool fullscreen = false;
		};

		/**
		* @brief Constructs a Window object with given description.
		* @param d Window description (size, title, vsync).
		*/
		explicit Window(const Desc& d) : mDesc(d), mWindow(nullptr), mWidth(d.w), mHeight(d.h) {}
		/*Window(int width, int height, const std::string& title);
		~Window();*/
		bool create();//create wind
		void destroy();//destroy wind

		// frame loop
		// Polls events from the OS (input, resize, etc.).
		void poll();
		// Swaps front and back buffers (double buffering).
		void swap();

		// queries
		bool ShouldClose() const;//if window close, return true
		// Returns raw GLFWwindow handle for advanced operations.
		GLFWwindow* handle() const { return mWindow; }

		//void PollEvents() const;//check for input events
		//void SwapBuffers() const;//dbl buffering

		int getWidth() const { return mWidth; }
		int getHeight() const { return mHeight; }
		//GLFWwindow* GetNativeHandle() const {
		//	return mWindow;
		//} //dir access to raw Wind

		bool isFullscreen() const { return mDesc.fullscreen; }
		void setFullscreen(bool fullscreen, int windowWidth, int windowHeight);

//		void Terminate();//destroy window
	private:
		GLFWwindow* mWindow; //point to window obj
		int mWidth, mHeight;//wind dim
		static void framebufferCB(GLFWwindow* w, int fbw, int fbh);
		Desc mDesc;//init wind description
};