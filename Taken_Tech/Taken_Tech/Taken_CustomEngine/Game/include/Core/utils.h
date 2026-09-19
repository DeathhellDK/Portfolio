/**
* @file     utils.h
* @author   Woh Kye Le
* @email    w.kyele
* @co-author
* @date     2025-09-29
*
* @brief
* This header file contains all helper function declaration
*  -> mesh related function: buildquad, buildtriangle, buildline, buildpoints
*  -> Framebuffer
*  -> World
*  -> GUI layer
*  -> Editor scene texture 
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#pragma once 

#include "Graphics/mesh2d.h"
#include "Graphics/renderer.h"
#include "Graphics/camera2d.h"
class SystemManager;
class MinimapHUD;
class StressTestManager;
class EditorOverlay;
class RoomEditor;
class UiEditor;
class GuiSystem;
class GameApp;
class SceneManager;
class MainMenu;
class PauseMenu;

namespace Utility {

	/*---------------------- building different type of mesh ---------------------------*/
	Mesh2D BuildQuad       (const std::vector<Vector2>& positions, const std::vector<Vector3>& colors);
	Mesh2D BuildTri        (const std::vector<Vector2>& positions, const std::vector<Vector3>& colors);
	Mesh2D BuildLine       (const std::vector<Vector2>& positions, const std::vector<Vector3>& colors, float lineWidth = 2.0f);
	Mesh2D BuildPoint      (const std::vector<Vector2>& positions, const std::vector<Vector3>& colors, float pointSize = 5.0f);
	Mesh2D BuildCircle     (int segments, const Vector3& colors, bool filled = true);
	Mesh2D BuildRectOutline(); // 1x1 rectangle outline from (0,0) to (1,1), drawn with GL_LINES

	/*-------------------------- Framebuffer / scene pass ------------------------------*/
	void BeginGameScene(Renderer& r, bool usingEditor, int fbw, int fbh);
	void EndGameScene  (Renderer& r, bool usingEditor, int fbw, int fbh);

	/*------------------------------- World composite ----------------------------------*/
	void DrawWorld(Renderer& r, SystemManager& systems, MinimapHUD& minimap, StressTestManager& stress, GameApp& app);

	/*------------------------------------ GUI  ----------------------------------------*/
	void DrawGuiLayer(Renderer& r, GuiSystem& gui, bool visible);

	/*------------------------------------ Editor  -------------------------------------*/
	#if ENABLE_EDITOR
	void ProvideEditorSceneTexture(EditorOverlay* overlay, Renderer& r, bool usingEditor);
	void ProvideEditorSceneTexture(RoomEditor* editor, Renderer& r, bool usingEditor);
	void ProvideEditorSceneTexture(UiEditor* editor, Renderer& r, bool usingEditor);
	void LoadTileGridFromTxt(const std::string& txtPath, std::vector<char>& out, int& outW, int& outH);
	void SaveTileGridToTxt  (const std::string& txtPath, const std::vector<char>& grid, int w, int h);
	void BindEditorOverlay(GameApp& app, EditorOverlay& overlay);
	#endif

	/*--------------------------- scene(mainmenu/pausemenu)  ---------------------------*/
	void BuildMainMenu(SceneManager& sceneManager, GuiSystem& mainMenuGui, MainMenu& mainMenu, bool& isPausedByESC, bool& shouldQuit, GameApp& app);
	void BuildPauseMenu(SceneManager& sceneManager, GuiSystem& pauseGui, PauseMenu& pauseMenu, bool& isPausedByESC, bool& shouldQuit, GameApp& app);
}