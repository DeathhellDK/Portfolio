#pragma once
/**
 * @file     engine.hpp
 * @author   Jethro Sung
 * @enail    sung.h
 * @co-author Woh Kye Le, Lim Zhi Jie
 * @email     w.kyele, zhijie.lim
 * @date     2025-09-11
 *
 * @brief    Minimal engine namespace providing windowing, timing,
 *           and input utilities on top of GLFW + OpenGL.
 *
 * This header declares core engine functions used by gameplay code,
 * including initialization, per-frame updates, delta-time access,
 * input accessors, and window handle retrieval. It wraps GLFW in a
 * simplified API suitable for use in the GameApp class and systems.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include <string>
#include <functional>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>   // make ::GLFWwindow visible

namespace eng {

	struct Config { int width = 1280, height = 720; const char* title = "2D"; bool vsync = true; bool fullscreen = false; };

bool   init(const Config& cfg);  // create window + GL context
void   shutdown();               // destroy window/GLFW

void   beginFrame();             // poll events, update input/time
void   endFrame();               // swap buffers
bool   shouldClose();            // window close request?

double deltaTime();              // seconds since last frame

// accessors for gameplay files

class  Input;
GLFWwindow* windowHandle();      
Input&      input();             // engine's input wrapper

void framebufferSize(int& w, int& h);

// fixed Dt simulation 
void   setFixedDelta(double seconds);                                // set target time step
double fixedDelta();                                                 // returns current fixed step
void   setMaxFixedStepsPerFrame(int n);                              // max number of fix update per frame 
void   forEachFixedStep(const std::function<void(double)>& stepFn);  // core fixed step loop 
double interpolationAlpha();                                         // returns time left over after the fixed update has been processed
																     // (between 1 and 0)
void toggleFullscreen(int windowWidth, int windowHeight);
bool isFullscreen();
 
bool   isPaused();
void   _setPaused(bool paused);
}