/**
* @file     utils.cpp
* @author   Woh Kye Le
* @email    w.kyele,t.weiliangterril, sung.h
* @co-author Tan Wei Liang Terril, Jethro Sung
* @date     2025-09-29
*
* @brief
* This header file contains all helper function definition
*  -> mesh related function: buildquad, buildtriangle, buildline, buildpoints, etc
*  -> Helpers to set up the game scene framebuffer
*  -> Helpers to draw the world, GUI layer, and editor viewport texture
* 
* Provides a collection of rendering and editor-support utilities that serve as foundational helpers for the engine’s graphics pipeline and toolchain. 
* It includes mesh-builder functions for shapes like quads, circles, lines, and outlines; scene-rendering helpers for managing game-viewport FBOs; 
* world-drawing compositors that render ECS systems, minimap, and debug layers; GUI-layer rendering utilities; and a range of editor-integration functions such as
* tile-map loading, saving, and FBO texture forwarding.
*
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Core/utils.h"
#include "Core/gameApp.h"
#include "Core/Systems/systemManager.h"
#if ENABLE_EDITOR
#include "Editor/editor.h"
#include "Editor/RoomEditor.h"
#include "Editor/UiEditor.h"
#endif
#include "UI/miniMap.h"
#include "UI/guiSys.h"
#include "StressTestManager.h"
#include <filesystem>
#include "Core/assetsPath.h"
#include <fstream>
#include "Input/DebugConsole.hpp"
#include "Core/sceneManager.h"
#include "UI/mainMenu.h"
#include "UI/pauseMenu.h"
#include "World/doorsystem.h"

// =========================================== meshes ===========================================
/**
 * @brief Builds a textured quad mesh using four positions and four colors.
 * The quad fills a 0..1 UV space and is made of two triangles.
 *
 * @param positions  Four vertex positions in order: BL, BR, TR, TL.
 * @param colors     Per-vertex color tint.
 * @return Mesh2D    A ready-to-use quad mesh.
 */
Mesh2D Utility::BuildQuad(const std::vector<Vector2>& positions,
	const std::vector<Vector3>& colors) {
	Mesh2D mesh;
	// Create 4 vertices with position, texcoord, and color
	std::vector<Vertex2D> verts = {
		Vertex2D(positions[0], Vector2(0,0), colors[0]), // bottom-left
		Vertex2D(positions[1], Vector2(1,0), colors[1]), // bottom-right
		Vertex2D(positions[2], Vector2(1,1), colors[2]), // top-right
		Vertex2D(positions[3], Vector2(0,1), colors[3])  // top-left
	};
	// Define 2 triangles
	std::vector<unsigned int> indices = { 0, 1, 2, 2, 3, 0 };
	mesh.Reserve(verts.size(), indices.size());
	mesh.SetVertices(verts);
	mesh.SetIndices(indices);
	return mesh;
}

/**
 * @brief Creates a simple triangle mesh using 3 positions and 3 colors.
 * @param positions  Three vertex positions.
 * @param colors     Three vertex colors.
 * @return Mesh2D    The triangle mesh.
 */
Mesh2D Utility::BuildTri(const std::vector<Vector2>& positions,
	const std::vector<Vector3>& colors) {
	Mesh2D mesh;
	std::vector<Vertex2D> verts = {
		Vertex2D(positions[0], {0,0}, colors[0]),
		Vertex2D(positions[1], {1,0}, colors[1]),
		Vertex2D(positions[2], {0.5f,1}, colors[2])
	};
	std::vector<unsigned int> indices = { 0,1,2 };
	mesh.Reserve(verts.size(), indices.size());
	mesh.SetVertices(verts);
	mesh.SetIndices(indices);
	mesh.SetPrimitive(GL_TRIANGLES);
	return mesh;
}

/**
 * @brief Build a line mesh between two points.
 * @param positions 2 positions that define the line.
 * @param colors    Color at each endpoint.
 * @param lineWidth OpenGL line width to use.
 */
Mesh2D Utility::BuildLine(const std::vector<Vector2>& positions,
	const std::vector<Vector3>& colors,
	float lineWidth) {
	Mesh2D mesh;
	std::vector<Vertex2D> verts = {
		Vertex2D(positions[0], {0,0}, colors[0]),
		Vertex2D(positions[1], {1,0}, colors[1])
	};
	std::vector<unsigned int> indices = { 0,1 };
	mesh.Reserve(verts.size(), indices.size());
	mesh.SetVertices(verts);
	mesh.SetIndices(indices);
	mesh.SetPrimitive(GL_LINES);
	mesh.SetLineWidth(lineWidth);
	return mesh;
}

/**
 * @brief Build a single point mesh.
 * @param positions Expected to contain 1 position.
 * @param colors    Color of the point.
 * @param pointSize OpenGL point size to use.
 */
Mesh2D Utility::BuildPoint(const std::vector<Vector2>& positions,
	const std::vector<Vector3>& colors,
	float pointSize) {
	Mesh2D mesh;
	std::vector<Vertex2D> verts = {
		Vertex2D(positions[0], {0,0}, colors[0])
	};
	std::vector<unsigned int> indices = { 0 };
	mesh.Reserve(verts.size(), indices.size());
	mesh.SetVertices(verts);
	mesh.SetIndices(indices);
	mesh.SetPrimitive(GL_POINTS);
	mesh.SetPointSize(pointSize);
	return mesh;
}

/**
 * @brief Build a circle mesh, either filled or outline.
 * The circle is centered at (0.5, 0.5) with radius 0.5,
 * so it fits nicely in a 0..1 square.
 */
Mesh2D Utility::BuildCircle(int segments,
	const Vector3& color,
	bool filled) {
	segments = std::max(3, segments);

	std::vector<Vertex2D> verts;
	std::vector<unsigned int> indices;

	if (filled) {
		// Triangle fan: center + ring
		verts.reserve(segments + 1);
		indices.reserve(segments * 3);

		// center at (0.5, 0.5) so circle fits in a 0..1 square
		verts.emplace_back(Vector2(0.5f, 0.5f),
			Vector2(0.5f, 0.5f),
			color);

		const float cx = 0.5f;
		const float cy = 0.5f;
		const float r = 0.5f;

		for (int i = 0; i < segments; ++i) {
			float t = static_cast<float>(i) / segments;
			float angle = t * 2.0f * 3.1415926f;
			float x = cx + r * std::cos(angle);
			float y = cy + r * std::sin(angle);

			verts.emplace_back(Vector2(x, y),
				Vector2((x - cx) / (2 * r) + 0.5f,
					(y - cy) / (2 * r) + 0.5f),
				color);
		}

		for (int i = 0; i < segments; ++i) {
			int next = (i + 1) % segments;
			indices.push_back(0);           // center
			indices.push_back(1 + i);
			indices.push_back(1 + next);
		}
	}
	else {
		// Line loop circle
		verts.reserve(segments);
		indices.reserve(segments);

		const float cx = 0.5f;
		const float cy = 0.5f;
		const float r = 0.5f;

		for (int i = 0; i < segments; ++i) {
			float t = static_cast<float>(i) / segments;
			float angle = t * 2.0f * 3.1415926f;
			float x = cx + r * std::cos(angle);
			float y = cy + std::sin(angle) * r;

			verts.emplace_back(Vector2(x, y),
				Vector2(0.f, 0.f),
				color);
			indices.push_back(i);
		}

		Mesh2D m;
		m.Reserve(verts.size(), indices.size());
		m.SetVertices(verts);
		m.SetIndices(indices);
		m.SetPrimitive(GL_LINE_LOOP);
		return m;
	}

	Mesh2D mesh;
	mesh.Reserve(verts.size(), indices.size());
	mesh.SetVertices(verts);
	mesh.SetIndices(indices);
	// default primitive = GL_TRIANGLES for filled fan
	return mesh;
}

/**
 * @brief Creates a simple 1x1 rectangle outline using a GL_LINE_LOOP.
 * @return Mesh2D    Rectangle outline mesh.
 */
Mesh2D Utility::BuildRectOutline() {
	// 4 corners of a unit rect
	std::vector<Vertex2D> v = {
		Vertex2D({0,0}, {0,0}, {1,1,1}),
		Vertex2D({1,0}, {0,0}, {1,1,1}),
		Vertex2D({1,1}, {0,0}, {1,1,1}),
		Vertex2D({0,1}, {0,0}, {1,1,1}),
	};
	// Line list: 0-1, 1-2, 2-3, 3-0
	std::vector<unsigned> i = { 0, 1, 2, 3 };

	Mesh2D m;
	m.Reserve(v.size(), i.size());
	m.SetVertices(v);
	m.SetIndices(i);
	m.SetPrimitive(GL_LINE_LOOP);
	return m;
}



// =========================================== scenes ===========================================
/**
 * @brief Begins the game scene rendering phase.
 *
 * If the editor is active, an off-screen framebuffer (FBO) is used.
 * Otherwise, the scene is drawn directly to the back-buffer.
 *
 * @param r           Renderer handling the drawing.
 * @param usingEditor True if the editor viewport is active.
 * @param fbw         Framebuffer width.
 * @param fbh         Framebuffer height.
 */
void Utility::BeginGameScene(Renderer& r, bool usingEditor, int fbw, int fbh) {
	if (usingEditor) {
		if (fbw > 0 && fbh > 0) r.CreateOrResizeSceneTarget(fbw, fbh);
		r.BindSceneTarget();
		r.Clear(0.2f, 0.2f, 0.2f, 1.0f);
	}
	else {
		// Game-only: make sure we render directly to the window
		// and that the viewport matches the current framebuffer size.
		if (fbw > 0 && fbh > 0) {
			// This is the same call that runs when you F1 the editor:
			// glBindFramebuffer(0) + glViewport(0,0,fbw,fbh)
			r.UnbindSceneTarget(fbw, fbh);
		}
		else {
			// Fallback safety: at least bind the default framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		r.Clear(0.2f, 0.2f, 0.2f, 1.0f);
	}
}

/**
 * @brief Ends the scene rendering phase.
 *
 * If the editor is active, this unbinds the off-screen framebuffer so the
 * result can be displayed inside the editor viewport.
 *
 * @param r           Renderer instance.
 * @param usingEditor Whether the editor is currently visible.
 * @param fbw         Framebuffer width.
 * @param fbh         Framebuffer height.
 */
void Utility::EndGameScene(Renderer& r, bool usingEditor, int fbw, int fbh) {
	if (usingEditor) r.UnbindSceneTarget(fbw, fbh);
}



// ======================================= World composite =======================================
/**
 * @brief Draws the main game world.
 *
 * This includes:
 *  - All systems (RenderSystem, AnimationSystem, etc.)
 *  - Minimap rendering
 *  - Stress test visual overlays when active
 *
 * @param r        Renderer instance.
 * @param systems  SystemManager responsible for drawing all ECS systems.
 * @param minimap  Minimap HUD renderer.
 * @param stress   Stress-test manager for debugging large batches of entities.
 */
void Utility::DrawWorld(Renderer& r, SystemManager& systems, MinimapHUD& minimap, StressTestManager& stress, GameApp& app) {
	systems.DrawAll(r); // draw all gameObjects
	minimap.Draw(r, app);	// draw minimap
	stress.Draw(r);	    // render test case
	app.GetDoorSystem().DrawActionHints(r, app);
}



// ======================================= Gui layering ==========================================
/**
 * @brief Draws the GUI layer on top of the world.
 *
 * GUI is rendered without a camera and then the previous camera
 * is restored.
 *
 * @param r        Renderer instance.
 * @param gui      GUI system containing buttons and overlays.
 * @param visible  Whether the GUI should be drawn.
 */
void Utility::DrawGuiLayer(Renderer& r, GuiSystem& gui, bool visible) {
	if (!visible) return;
	Camera2D* prev = r.getCamera();
	r.setCamera(nullptr);
	gui.Draw(r);
	r.setCamera(prev);
}



// ========================================== Editor =============================================
#if ENABLE_EDITOR
/**
 * @brief Sends the rendered scene texture to the editor viewport.
 *
 * When the editor is visible, this passes the scene FBO texture so it can
 * be shown inside the editor's Game View window. When the editor is hidden,
 * the editor receives a null texture.
 *
 * @param overlay      Editor overlay instance.
 * @param r            Renderer instance.
 * @param usingEditor  True if the editor viewport is currently shown.
 */
void Utility::ProvideEditorSceneTexture(EditorOverlay* overlay, Renderer& r, bool usingEditor) {
	if (!overlay) return;
	if (usingEditor) {
		overlay->SetSceneTexture(
			static_cast<unsigned int>(r.GetSceneTexture()),
			r.GetSceneWidth(), r.GetSceneHeight());
	}
	else {
		overlay->SetSceneTexture(0, 0, 0);
	}
}

/**
 * @brief Sends the rendered scene texture to the Roomeditor viewport, overload function
 *
 * When the Roomeditor is visible, this passes the scene FBO texture so it can
 * be shown inside the editor's Game View window. When the editor is hidden,
 * the editor receives a null texture.
 *
 * @param editor       RoomEditor instance.
 * @param r            Renderer instance.
 * @param usingEditor  True if the room editor viewport is currently shown.
 */
void Utility::ProvideEditorSceneTexture(RoomEditor* editor, Renderer& r, bool usingEditor){
	if (!editor) return;

	if (usingEditor) {
		editor->SetSceneTexture(
			static_cast<unsigned int>(r.GetSceneTexture()),
			r.GetSceneWidth(), r.GetSceneHeight()
		);
	}
	else {
		editor->SetSceneTexture(0, 0, 0);
	}
}

void Utility::ProvideEditorSceneTexture(UiEditor* editor, Renderer& r, bool usingEditor) {
	if (!editor) return;

	if (usingEditor) {
		editor->SetSceneTexture(
			static_cast<unsigned int>(r.GetSceneTexture()),
			r.GetSceneWidth(), r.GetSceneHeight()
		);
	}
	else {
		editor->SetSceneTexture(0, 0, 0);
	}
}

/**
 * @brief Load a tile grid from a plain-text map file.
 *
 * Each line in the file is treated as a row. Lines are padded with spaces
 * so that all rows share the same width. The grid is stored in row-major
 * order: out[y * outW + x].
 *
 * @param txtPath Path to the .txt file (usually under Assets/maps).
 * @param out     Output flat grid of characters.
 * @param outW    Output width of the grid.
 * @param outH    Output height of the grid.
 */
void Utility::LoadTileGridFromTxt(const std::string& txtPath, std::vector<char>& out, int& outW, int& outH) {
	out.clear();
	outW = outH = 0;

	std::ifstream in(txtPath);
	if (!in.is_open()) {
		DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[Utility] LoadTileGridFromTxt: failed to open ", txtPath, "\n");
		return;
	}

	std::vector<std::string> rows;
	std::string line;

	size_t maxW = 0;
	while (std::getline(in, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();


		rows.push_back(line);
		maxW = std::max(maxW, line.size());
	}

	if (rows.empty() || maxW == 0) {
		DebugConsole::Get().AddFormattedMessage(LogLevel::Warning, "[Utility] LoadTileGridFromTxt: no usable rows in ", txtPath, "\n");
		return;
	}

	outH = static_cast<int>(rows.size());
	outW = static_cast<int>(maxW);

	out.resize(outW * outH);

	for (int y = 0; y < outH; ++y) {
		const std::string& row = rows[y];
		for (int x = 0; x < outW; ++x) {
			char c = (x < static_cast<int>(row.size())) ? row[x] : ' ';
			out[y * outW + x] = c;
		}
	}

	DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Utility] LoadTileGridFromTxt: ", outW, "x", outH, " from ", txtPath, "\n");
}


/**
 * @brief Save a tile grid into a plain-text map file.
 *
 * Writes the flat grid as @p h rows of @p w characters each, followed by
 * a newline. This is the inverse of LoadTileGridFromTxt().
 *
 * @param txtPath Destination file path (Assets/maps/...).
 * @param grid    Flat character grid.
 * @param w       Grid width.
 * @param h       Grid height.
 */
void Utility::SaveTileGridToTxt(const std::string& txtPath, const std::vector<char>& grid, int w, int h) {
	if (w <= 0 || h <= 0 || (int)grid.size() != w * h) {
		DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[Utility] SaveTileGridToTxt: invalid grid size\n");
		return;
	}

	std::ofstream out(txtPath);
	if (!out.is_open()) {
		DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[Utility] SaveTileGridToTxt: failed to open ", txtPath, " for write\n");
		return;
	}

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			out << grid[y * w + x];
		}
		out << '\n';
	}
}

/**
	* @brief Connects the editor overlay’s tile-edit events to real engine behavior.
	*
	* Installs callbacks for:
	*  - Enter Tile Edit → loads TXT grid from disk
	*  - Exit Tile Edit  → currently no-op
	*  - Apply Tiles     → saves TXT grid and requests deferred scene reload
	*
	* Automatically maps:
	*  scene/entities_LevelX.json → maps/entities_LevelX.txt
	*  labyrinth.json             → maps/labyrinth.txt
	*
	* @param app     Reference to GameApp.
	* @param overlay EditorOverlay instance.
*/
void Utility::BindEditorOverlay(GameApp& app, EditorOverlay& overlay) {
	using std::string;
	using std::vector;
	namespace fs = std::filesystem;

	// --- Tile Edit Enter: load TXT into the editor grid ---
	overlay.onTileEditEnter = [&app, &overlay]() {
		// Decide which scene we're editing
		const string scenePath =
			!overlay.currentLevelPath.empty()
			? overlay.currentLevelPath
			: app.GetActiveScenePath();

		// Map scene JSON to TXT grid path
		string txtPath;
		if (scenePath.find("labyrinth.json") != string::npos) {
			txtPath = AssetPath("maps/labyrinth.txt");
		}
		else {
			fs::path scene(scenePath);            // e.g. Assets/scene/entities_Level1.json
			string base = scene.stem().string();  // entities_Level1
			txtPath = AssetPath("maps/" + base + ".txt");
		}

		vector<char> flat;
		int w = 0, h = 0;
		Utility::LoadTileGridFromTxt(txtPath, flat, w, h);
		if (w > 0 && h > 0) {
			overlay.SetTileGrid(flat, w, h);
		}
		};

	// --- Tile Edit Exit ---
	overlay.onTileEditExit = []() {
		// empty in design 
		};

	// --- Apply: save grid back to TXT and reload the level (deferred) ---
	overlay.onTileApply =
		[&app, &overlay](const std::vector<char>& flat, int w, int h) {
		namespace fs = std::filesystem;


		std::string sceneRel;
		if (!overlay.currentLevelPath.empty()) {

			sceneRel = overlay.currentLevelPath;
		}
		else {
			// Fallback: derive a relative "scene/XXX.json" from whatever GameApp uses
			std::string active = app.GetActiveScenePath();   // may be absolute or asset-rooted
			fs::path p(active);
			std::string base = p.stem().string();
			std::string ext = p.extension().string();       // .json
			sceneRel = "scene/" + base + ext;               // -> "scene/entities_Level2.json"
		}

		// Map scene JSON to TXT grid path
		std::string txtPath;
		if (sceneRel.find("labyrinth.json") != std::string::npos) {
			txtPath = AssetPath("maps/labyrinth.txt");
		}
		else {
			fs::path scene(sceneRel);
			std::string base = scene.stem().string(); // entities_LevelX
			txtPath = AssetPath("maps/" + base + ".txt");
		}

		Utility::SaveTileGridToTxt(txtPath, flat, w, h);

		//reload using the normalized relative path
		DebugConsole::Get().Info("[Editor] Tile Apply -> reload scene '" + sceneRel + "'");
		overlay.RequestLevelReload(sceneRel);
		};
}
#endif // ENABLE_EDITOR

// ==================================== mainmenu/pausemenu =======================================


/**
	* @brief Builds the main menu UI and binds button callbacks.
	*
	* Callbacks installed:
	*  - Play      → switch to Playing
	*  - Settings  → switch to Settings
	*  - Quit      → set quit flag
	*
	* @param sceneManager Scene manager controlling state.
	* @param mainMenuGui GUI system used by the main menu.
	* @param mainMenu    MainMenu controller.
	* @param isPausedByESC Reference to pause flag.
	* @param shouldQuit    Reference to quit request flag.
	* @param app           GameApp instance.
*/
void Utility::BuildMainMenu(SceneManager& sceneManager, GuiSystem& mainMenuGui, MainMenu& mainMenu, bool& isPausedByESC, bool& shouldQuit, GameApp& app) {
	sceneManager.SetState(SceneState::MainMenu);

	mainMenu.Build(
		mainMenuGui,

		// --- On Play ---
		[&sceneManager, &isPausedByESC, &app]() {
			DebugConsole::Get().Info("[MainMenu] Play clicked\n");
			sceneManager.SetState(SceneState::Playing);
			isPausedByESC = false;
		},

		// --- On Tutorial ---
		[&sceneManager]() {
			DebugConsole::Get().Info("[MainMenu] Tutorial clicked\n");
			sceneManager.SetState(SceneState::Tutorial);
		},

		// --- On Settings ---
		[&sceneManager]() {
			DebugConsole::Get().Info("[MainMenu] Settings clicked\n");
			sceneManager.SetState(SceneState::Settings);
		},

		// --- On Credits ---
		[&sceneManager]() {
			DebugConsole::Get().Info("[MainMenu] Credits clicked\n");
			sceneManager.SetState(SceneState::Credits);
		},

		// --- On Quit ---
		[&shouldQuit]() {
			DebugConsole::Get().Info("[MainMenu] Quit clicked\n");
			shouldQuit = true;
		}
	);
}

/**
	* @brief Builds the pause menu UI and binds button callbacks.
	*
	* Callbacks installed:
	*  - Resume            → return to Playing
	*  - Back to Main Menu → return to MainMenu
	*  - Quit              → set quit flag
	*
	* @param sceneManager Scene manager controlling state.
	* @param pauseGui     GUI system used by the pause menu.
	* @param pauseMenu    PauseMenu controller.
	* @param isPausedByESC Reference to ESC pause flag.
	* @param shouldQuit    Reference to quit request flag.
	* @param app           GameApp instance.
*/
void Utility::BuildPauseMenu(SceneManager& sceneManager, GuiSystem& pauseGui, PauseMenu& pauseMenu, bool& isPausedByESC, bool& shouldQuit, GameApp& app) {
	pauseMenu.Build(
		pauseGui,

		// --- Resume ---
		[&sceneManager, &isPausedByESC]() {
			DebugConsole::Get().Info("[PauseMenu] Resume\n");
			sceneManager.SetState(SceneState::Playing);
			isPausedByESC = false;
		},

		// --- Back to Main Menu ---
		[&sceneManager, &isPausedByESC, &app]() {
			DebugConsole::Get().Info("[PauseMenu] Back to Main Menu\n");
			sceneManager.SetState(SceneState::MainMenu);
			isPausedByESC = false;
		},

		// --- Quit ---
		[&shouldQuit]() {
			DebugConsole::Get().Info("[PauseMenu] Quit\n");
			shouldQuit = true;
		}
	);
}