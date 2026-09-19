/**
 * @file      editor.cpp
 * @author    Jethro Sung
 * @email     sung.h, w.kyele, sweeyongdillon.sng, jianlin.low, t.weiliangterril
 * @co-author Woh Kye Le, Sng Swee Yong Dillon, Low JianLin, Tan Wei Liang Terril
 * @date      2025-11-07
 *
 * @brief     Implements the EditorOverlay class declared in editor.h.
 *
 * This file defines the full in-game editor overlay used by the Taken_Tech
 * engine. It builds an ImGui-based multi-panel interface with a docking
 * layout, providing tools for runtime scene editing such as:
 *
 * - Entity Hierarchy: Browsing, selecting, focusing, and deleting entities.
 * - Property Inspector: Editing Transform, MeshRenderer, Collider,
 *   SpriteAnimator, and ScriptComponent fields.
 * - Spawner Panel Creating objects, players, textured props, and recording
 *   deletion/creation for undo/redo.
 * - Viewport Interaction: Dragging entities in world space, hit-testing,
 *   direct click-selection, and ImGuizmo transform manipulation.
 * - Gizmo Mode: Translate/rotate/scale operations with undo/redo tracking.
 * - Tilemap Editor: Per-level tile painting, caching tiles per scene, and
 *   triggering reload on apply.
 * - Asset Browser: Assign textures and preview audio files.
 * - Play Controls: Play/Stop runtime switching, undo/redo buttons, and
 *   keyboard shortcuts.
 * - Performance Panel: FPS display and per-system timing information.
 * - Debug Console: Scrollable log with color-coded categories.
 * - History Panel: Visual view of the Undo/Redo stack.
 *
 * The editor collaborates with the ECS through IComponentContext and interacts
 * with GameApp via callback functions (onPlay, onUndo, onTileApply, etc.).
 * ImGui, ImGuizmo, and GLFW/OpenGL3 backends are initialized, updated, and
 * destroyed within this file.
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#ifndef ENABLE_EDITOR
#define ENABLE_EDITOR 0
#endif

#if ENABLE_EDITOR
#include "Editor/editor.h"
#include "Editor/ImGuiHost.h"
#include "Core/transform.h"
#include "Physics/collider.h"
#include "Graphics/meshrenderer.h"
#include "Core/Systems/systemManager.h"
#include <GLFW/glfw3.h>
#include "Core/engine.hpp"
#include "Input/input.h"
#include "Graphics/spriteanimator.h"
#include "Core/componentcontext.h"
#include <filesystem>
#include <cctype>
#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "Audio/audio.h"
#include "Editor/UndoRedoManager.h"
#include "Editor/PlayStopManager.h"
#include "Editor/EditorShared.h"
#include "Particle/particleEmitter.h"
#include "Light/lightComponent.h"
#include <fstream>
#define NOMINMAX
#undef APIENTRY
#include <shobjidl.h>

std::string OpenFolderDialog() {
    std::string outPath;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileOpenDialog* pFolderDialog;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFolderDialog));
        if (SUCCEEDED(hr)) {
            DWORD dwOptions;
            if (SUCCEEDED(pFolderDialog->GetOptions(&dwOptions))) {
                pFolderDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
            }
            if (SUCCEEDED(pFolderDialog->Show(NULL))) {
                IShellItem* pItem;
                if (SUCCEEDED(pFolderDialog->GetResult(&pItem))) {
                    PWSTR pszFilePath;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                        outPath.resize(size_needed - 1);
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &outPath[0], size_needed, NULL, NULL);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFolderDialog->Release();
        }
        CoUninitialize();
    }
    return outPath;
}

 // -----------------------------------------------------------------------------
 // helper functions 
 // -----------------------------------------------------------------------------

 // --- logical <-> screen mapping (keep spawnPos in a fixed logical space) ---
static ImVec2 gRefWorkSize{ 0,0 };   // first viewport size 
static bool   gRefSet = false;
// Current on-screen rect of the game image inside the Viewport win
static ImVec2 gViewportImagePos{ 0,0 };
static ImVec2 gViewportImageSize{ 0,0 };
// Audio 
static bool gAudioErrorPopup = false;
static std::string gAudioErrorMessage;
constexpr const char* kAudioPayloadType = "AUDIO_PATH"; // payload type string for ImGui drag/drop

static Audio::VoiceHandle gAudioPreviewVoice = 0; // Handle for the currently playing audio preview (0 = none)

/**
 * @brief Check if a file extension is a supported audio format.
 *
 * Compares the given lowercase extension string (including the leading dot)
 * against the set of audio formats supported by the editor preview path.
 *
 * @param extLower File extension in lowercase (e.g. ".wav", ".mp3", ".ogg").
 * @return true if the extension is one of the supported audio types, false otherwise.
 */
static bool IsAudioFileExt(const std::string& extLower) {
    return extLower == ".wav" ||
        extLower == ".mp3" ||
        extLower == ".ogg";
}

/**
 * @brief Converts logical coordinates (bottom-left origin) into ImGui screen coordinates.
 *
 * Maintains consistent UI positions across different viewport sizes.
 *
 * @param L  Logical position in reference coordinate space.
 * @param vp Pointer to current ImGui viewport.
 * @return ImVec2 Screen-space position (top-left origin).
*/
static inline ImVec2 LogicalToScreen(const ImVec2& L, const ImGuiViewport* vp) {
    // map from logical coords (refWorkSize, origin bottom-left) to screen
    const float sx = vp->WorkSize.x / gRefWorkSize.x;
    const float sy = vp->WorkSize.y / gRefWorkSize.y;
    return ImVec2(
        vp->WorkPos.x + L.x * sx,
        vp->WorkPos.y + (vp->WorkSize.y - L.y * sy) // flip Y to top-left origin
    );
}

/**
 * @brief Converts ImGui screen coordinates (top-left origin) back to logical space.
 *
 * Used when dragging entities or computing mouse positions relative to the logical grid.
 *
 * @param S  Screen position (e.g., from ImGui IO or mouse).
 * @param vp Pointer to current ImGui viewport.
 * @return Vector2 Logical position in the editor reference space.
*/
static inline Vector2 ScreenToLogical(const ImVec2& S, const ImGuiViewport* vp) {
    // invert the mapping above
    const float sx = vp->WorkSize.x / gRefWorkSize.x;
    const float sy = vp->WorkSize.y / gRefWorkSize.y;

    const float x = (S.x - vp->WorkPos.x) / sx;
    const float y = (vp->WorkSize.y - (S.y - vp->WorkPos.y)) / sy;
    return Vector2(x, y);
}

// ------------------------------------------------------------------------------

/**
 * @brief Constructs and initializes the EditorOverlay.
 *
 * Sets up ImGui with GLFW and OpenGL3 backends and centers the spawn position
 * in the middle of the current window. A reference to the ECS context is stored
 * for all entity/component access.
 *
 * @param ctx Reference to the global ECS component context.
*/
EditorOverlay::EditorOverlay(IComponentContext& ctx)
    : context(ctx)
{
    // --- Initialize ImGui Context ---

    GLFWwindow* window = glfwGetCurrentContext();
    if (!window)
        return;

    // Acquire shared ImGui + backends (ref-counted)
    ImGuiHost::Get().Acquire(window, "#version 330");

    // Editor-specific settings (safe to do here)
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.1f; // keep your old base scale

    if (window) {
        int ww = 0, wh = 0;
        glfwGetWindowSize(window, &ww, &wh);
        spawnPos = { static_cast<float>(ww) * 0.5f, static_cast<float>(wh) * 0.5f };
    }
}

/**
 * @brief Cleans up the ImGui context and backends upon dtor.
*/
EditorOverlay::~EditorOverlay(){
    // --- Shutdown backends and destroy context ---
    ImGuiHost::Get().Release();
}


/**
 * @brief Draws all enabled editor panels and UI.
 *
 * Panel overview:
 * - Performance: frame timing + system metrics
 * - Spawner: instantiate new objects
 * - Hierarchy: entity list + selection
 * - Property Editor: edit Transform/Collider/script properties
 * - Assets Browser: texture assignment. audio testing 
 * - Level Selector: load/save scene files
 * - PlayBar: toggle simulation runtime mode
 *
 * @param dt Delta time in seconds (used for performance metrics)
 */
static std::string FormatSize(uintmax_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f KB", (double)bytes / 1024.0);
        return std::string(buf);
    }
    if (bytes < 1024 * 1024 * 1024) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f MB", (double)bytes / (1024.0 * 1024.0));
        return std::string(buf);
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    return std::string(buf);
}

static void DrawBuildSizeAnalyzer(bool& show) {
    if (!show) return;

    static bool init = false;
    static std::vector<std::string> allAssets;
    static std::unordered_map<std::string, bool> selectedAssets;
    static std::unordered_map<std::string, uintmax_t> assetSizes;
    
    auto SaveSelection = [&]() {
        std::ofstream ofs("build_size_selection.txt");
        if (ofs.is_open()) {
            for (const auto& pair : selectedAssets) {
                if (pair.second) {
                    ofs << pair.first << "\n";
                }
            }
        }
    };

    if (!init) {
        init = true;
        std::ifstream ifs("build_size_selection.txt");
        std::string line;
        while (std::getline(ifs, line)) {
            if (!line.empty()) {
                selectedAssets[line] = true;
            }
        }
        
        std::error_code ec;
        if (std::filesystem::exists("Assets", ec)) {
            for (auto& entry : std::filesystem::recursive_directory_iterator("Assets", ec)) {
                if (entry.is_regular_file()) {
                    std::string path = entry.path().string();
                    std::replace(path.begin(), path.end(), '\\', '/');
                    allAssets.push_back(path);
                    assetSizes[path] = entry.file_size(ec);
                    if (selectedAssets.find(path) == selectedAssets.end()) {
                        selectedAssets[path] = false;
                    }
                }
            }
        }
    }

    if (ImGui::Begin("Build Size Analyzer", &show)) {
        if (ImGui::Button("Refresh Assets")) {
            allAssets.clear();
            assetSizes.clear();
            std::error_code ec;
            if (std::filesystem::exists("Assets", ec)) {
                for (auto& entry : std::filesystem::recursive_directory_iterator("Assets", ec)) {
                    if (entry.is_regular_file()) {
                        std::string path = entry.path().string();
                        std::replace(path.begin(), path.end(), '\\', '/');
                        allAssets.push_back(path);
                        assetSizes[path] = entry.file_size(ec);
                        if (selectedAssets.find(path) == selectedAssets.end()) {
                            selectedAssets[path] = false;
                        }
                    }
                }
            }
        }

        uintmax_t totalSize = 0;
        for (const auto& asset : allAssets) {
            if (selectedAssets[asset]) {
                totalSize += assetSizes[asset];
            }
        }
        
        ImGui::Text("Total Selected Size: %s", FormatSize(totalSize).c_str());
        
        if (ImGui::Button("Select All")) {
            for (const auto& asset : allAssets) {
                selectedAssets[asset] = true;
            }
            SaveSelection();
        }
        ImGui::SameLine();
        if (ImGui::Button("Deselect All")) {
            for (const auto& asset : allAssets) {
                selectedAssets[asset] = false;
            }
            SaveSelection();
        }

        ImGui::Separator();

        ImGui::BeginChild("AssetList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), true);
        bool changed = false;
        for (const auto& asset : allAssets) {
            bool selected = selectedAssets[asset];
            std::string label = asset + " (" + FormatSize(assetSizes[asset]) + ")";
            if (ImGui::Checkbox(label.c_str(), &selected)) {
                selectedAssets[asset] = selected;
                changed = true;
            }
        }
        if (changed) {
            SaveSelection();
        }
        ImGui::EndChild();

        if (ImGui::Button("Export Selected Assets")) {
            std::string exportDir = OpenFolderDialog();
            if (!exportDir.empty()) {
                std::error_code ec;
                for (const auto& pair : selectedAssets) {
                    if (pair.second) {
                        std::filesystem::path srcPath(pair.first);
                        std::filesystem::path dstPath = std::filesystem::path(exportDir) / srcPath;
                        std::filesystem::create_directories(dstPath.parent_path(), ec);
                        std::filesystem::copy_file(srcPath, dstPath, std::filesystem::copy_options::overwrite_existing, ec);
                    }
                }
            }
        }
    }
    ImGui::End();
}

void EditorOverlay::Draw(float dt) {
    if (!visible) return;
    if (!glfwGetCurrentContext()) return;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    // ---------------------------------------------------------------------
    // Fullscreen host window + DockSpace
    // ---------------------------------------------------------------------
    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;

    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##DockHost", nullptr, hostFlags);
    ImGui::PopStyleVar(2);

    static bool s_showBuildSizeAnalyzer = false;

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Build Size Analyzer")) {
                s_showBuildSizeAnalyzer = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    DrawBuildSizeAnalyzer(s_showBuildSizeAnalyzer);

    ImGuiIO& io = ImGui::GetIO();
    bool ctrl = io.KeyCtrl;

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
        if (onUndo) onUndo();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
        if (onRedo) onRedo();
    }


    // HARD GUARD: when viewport is minimized / zero-sized,
    if (vp->WorkSize.x <= 1.0f || vp->WorkSize.y <= 1.0f) {
        // Clear the viewport image rect so helpers early-out safely
        gViewportImagePos = ImVec2(0.0f, 0.0f);
        gViewportImageSize = ImVec2(0.0f, 0.0f);
        ImGui::End();
        return;
    }

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

    // ---------------------------------------------------------------------
    // One-time auto-layout: build dock tree and dock our windows.
    // This runs only on the first frame of the editor (or after ini reset).
    // ---------------------------------------------------------------------
    if (firstDockBuild_) {
        firstDockBuild_ = false;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGuiDockNodeFlags dockspaceFlags = static_cast<ImGuiDockNodeFlags>(
            static_cast<int>(ImGuiDockNodeFlags_DockSpace) |
            static_cast<int>(ImGuiDockNodeFlags_PassthruCentralNode));

        ImGui::DockBuilderAddNode(dockspace_id, dockspaceFlags);
        ImGui::DockBuilderSetNodeSize(dockspace_id, vp->WorkSize);

        ImGuiID dock_main_id = dockspace_id; // central node

        // 1) Top strip: Play / Camera / Gizmo
        ImGuiID dock_top = ImGui::DockBuilderSplitNode(
            dock_main_id, ImGuiDir_Up, 0.12f, nullptr, &dock_main_id);

        // 2) Left/right sidebars
        ImGuiID dock_left = ImGui::DockBuilderSplitNode(
            dock_main_id, ImGuiDir_Left, 0.24f, nullptr, &dock_main_id);
        ImGuiID dock_right = ImGui::DockBuilderSplitNode(
            dock_main_id, ImGuiDir_Right, 0.22f, nullptr, &dock_main_id);

        // 3) Bottom strip: Level selector
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(
            dock_main_id, ImGuiDir_Down, 0.15f, nullptr, &dock_main_id);
        // dock_main_id is now the central "game" region above the level selector.

        // 4) Left side split: Performance (top), Spawner+Assets (bottom)
        ImGuiID dock_left_bottom = ImGui::DockBuilderSplitNode(
            dock_left, ImGuiDir_Down, 0.45f, nullptr, &dock_left);

        // 5) Right side split: Hierarchy (top), Properties (bottom)
        ImGuiID dock_right_bottom = ImGui::DockBuilderSplitNode(
            dock_right, ImGuiDir_Down, 0.45f, nullptr, &dock_right);

        // Central dock for game viewport rect computation
        centerDockId_ = dock_main_id;

        // --- Dock windows ---

        // Left column
        ImGui::DockBuilderDockWindow("Debug Console", dock_left);
        ImGui::DockBuilderDockWindow("Performance", dock_left);
        ImGui::DockBuilderDockWindow("History", dock_left);
        ImGui::DockBuilderDockWindow("Spawner", dock_left_bottom);
        ImGui::DockBuilderDockWindow("Assets Browser", dock_left_bottom);

        // Right column: Hierarchy (top), Tilemap + Properties (bottom)
        ImGui::DockBuilderDockWindow("Hierarchy", dock_right);
        ImGui::DockBuilderDockWindow("Item Palette##Overlay", dock_right);
        ImGui::DockBuilderDockWindow("Tilemap Editor", dock_right);
        ImGui::DockBuilderDockWindow("Property Selector##Overlay", dock_right_bottom);

        // Bottom: level selector
        ImGui::DockBuilderDockWindow("Level Selector", dock_bottom);

        // Top: play/stop + camera mode + gizmo (as tabs)
        ImGui::DockBuilderDockWindow("Play Controls", dock_top);
        ImGui::DockBuilderDockWindow("Camera Mode", dock_top);
        ImGui::DockBuilderDockWindow("Gizmo Mode", dock_top);
        ImGui::DockBuilderDockWindow("Build Size Analyzer", dock_top);

        // Viewport remains a floating, NoDocking window.
        // ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    // ---------------------------------------------------------------------
    // Compute desired game viewport rect from center dock node
    // ---------------------------------------------------------------------
    ImVec2 gameWinPos = vp->WorkPos;
    ImVec2 gameWinSize = vp->WorkSize;

    if (ImGuiDockNode* centerNode = ImGui::DockBuilderGetNode(centerDockId_)) {
        if (centerNode->Size.x > 32.0f && centerNode->Size.y > 32.0f) {
            gameWinPos = centerNode->Pos;
            gameWinSize = centerNode->Size;

            // add padding inside the dock borders
            const float pad = 8.0f;
            gameWinPos.x += pad;
            gameWinPos.y += pad;
            gameWinSize.x = std::max(0.0f, gameWinSize.x - 2.0f * pad);
            gameWinSize.y = std::max(0.0f, gameWinSize.y - 2.0f * pad);
        }
    }

    // ---------------------------------------------------------------------
    // ImGuizmo + camera / font scaling
    // ---------------------------------------------------------------------
    ImGuizmo::BeginFrame();

    // now only sets framebuffer scale
    SyncCameraViewportToWorkArea_(vp);

    // remember first work size as logical reference
    if (!gRefSet) {
        gRefWorkSize = vp->WorkSize;
        gRefSet = true;
    }

    // detect main viewport resize to rescale fonts
    const bool viewportChanged =
        (lastWorkSize.x != vp->WorkSize.x || lastWorkSize.y != vp->WorkSize.y);

    if (viewportChanged) {
        firstDockBuild_ = true;

        ImGuiIO& io2 = ImGui::GetIO();

        float scale = 1.0f;
        if (gRefWorkSize.y > 0.0f)
            scale = vp->WorkSize.y / gRefWorkSize.y;

        // clamp font scaling
        if (scale < 0.75f) scale = 0.75f;
        if (scale > 1.75f) scale = 1.75f;

        io2.FontGlobalScale = 1.1f * scale; // 1.1f = ctor base
    }

    lastWorkSize = vp->WorkSize;

    // ---------------------------------------------------------------------
    // Top bars
    // ---------------------------------------------------------------------
    DrawPlayBar();
    DrawGizmoBar();

    // ---------------------------------------------------------------------
    // Helper to draw the game viewport window safely
    // ---------------------------------------------------------------------
    auto DrawGameViewportWindow = [&](const ImVec2& winPos,
        const ImVec2& winSize,
        const ImGuiViewport* mainVp)
        {
            ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
            ImGui::SetNextWindowViewport(mainVp->ID);

            bool viewportVisible = ImGui::Begin("Viewport", nullptr,
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoNavFocus |
                ImGuiWindowFlags_NoDocking);

            if (!viewportVisible) {
                // window is collapsed/hidden: clear rect so others know there is no image
                gViewportImagePos = ImVec2(0.0f, 0.0f);
                gViewportImageSize = ImVec2(0.0f, 0.0f);
                ImGui::End();
                return;
            }

            ImVec2 imagePos = ImGui::GetCursorScreenPos();
            ImVec2 imageSize = ImGui::GetContentRegionAvail();

            // remember where the game image lives this frame
            gViewportImagePos = imagePos;
            gViewportImageSize = imageSize;

            const bool hasValidTexture =
                (sceneTextureId != 0 && sceneTexWidth > 0 && sceneTexHeight > 0 &&  
                    imageSize.x > 0.0f && imageSize.y > 0.0f);

            if (hasValidTexture) {
                ImGui::Image(
                    (ImTextureID)(intptr_t)sceneTextureId,
                    imageSize,
                    ImVec2(0, 1),
                    ImVec2(1, 0)
                );

                bool viewportHovered =
                    ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

                ImGuiIO& io = ImGui::GetIO();
                if (viewportHovered)
                    io.WantCaptureMouse = false;

                // gizmo / drag / drop only when we actually have an image
                DrawGizmoInViewport();
                HandleViewportDrag();
                DrawViewportDropTarget();
            }
            else {
                // no valid texture or zero-size: mark as invalid so helpers early-out
                gViewportImagePos = ImVec2(0.0f, 0.0f);
                gViewportImageSize = ImVec2(0.0f, 0.0f);
            }

            ImGui::End();
        };

    DrawGameViewportWindow(gameWinPos, gameWinSize, vp);

    // ---------------------------------------------------------------------
    // Panels + viewport
    // ---------------------------------------------------------------------

    const bool disablePanels = (isPlaying && !isPaused); // Play = locked, Pause = editable like Stop

    if (disablePanels) {
        ImGui::BeginDisabled(true);
    }

    DrawSpawner();
    DrawAssetsBrowser();
    DrawHierarchy();
    DrawItemPalette();
    DrawDebugConsole();
    DrawTilemapEditor();
    DrawPropertyEditor();
    DrawLevelSelector();
    DrawHistory();

  
    if (disablePanels) {
        ImGui::EndDisabled();
    }

    // ---------------------------------------------------------------------
    // Misc panels & camera follow
    // ---------------------------------------------------------------------
    UpdateSelectionCameraFollow();  // based on selected entity

    DrawPerformance(dt);
    DrawCameraModePanel();
    HandleNavigationClick();

    // ---------------------------------------------------------------------
    // Audio error popup (modal)
    // ---------------------------------------------------------------------
    if (gAudioErrorPopup)
    {
        ImGui::OpenPopup("Audio Error");
        gAudioErrorPopup = false;
    }

    if (ImGui::BeginPopupModal("Audio Error", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        // Center popup in main viewport
        const ImGuiViewport* vpPopup = ImGui::GetMainViewport();
        ImVec2 viewportCenter = vpPopup->GetCenter();
        ImVec2 windowSize = ImGui::GetWindowSize();

        ImVec2 pos(
            viewportCenter.x - windowSize.x * 0.5f,
            viewportCenter.y - windowSize.y * 0.5f
        );

        ImGui::SetWindowPos(pos, ImGuiCond_Always);

        ImGui::TextWrapped("%s", gAudioErrorMessage.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}



/**
    * @brief Displays real-time performance metrics.
    *
    * Shows frame time, FPS, entity count, and system update/draw timings
    * reported by the ECS SystemManager. Used to help profile in-editor
    * runtime performance.
    *
    * @param dt Delta time for calculating frame duration.
*/
void EditorOverlay::DrawPerformance(float dt) {
    if (!glfwGetCurrentContext()) return;
    double frameMs = dt * 1000.0;

    static ImGuiWindowClass cls;
    cls.ClassId = ImHashStr("EditorTabGroup");
    cls.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
    ImGui::SetNextWindowClass(&cls);

    ImGui::Begin("Performance", nullptr,
        ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Frame: %.3f ms", frameMs);
    ImGui::Text("FPS:   %.1f", (dt > 0.f ? 1.f / dt : 0.f));

    const auto& entities = context.GetEntities();
    ImGui::Text("Entities: %d", (int)entities.size());

    const auto& timings = context.GetSystemManager().GetTimings();
    if (!timings.empty()) {
        ImGui::Separator();
        ImGui::Text("System Timings");
        for (auto& t : timings)
            ImGui::Text("%s: %.3f ms (upd)  %.3f ms (draw)",
                t.name.c_str(), t.lastUpdateMs, t.lastDrawMs);
    }
    ImGui::End();
}
/**
    * @brief Draws the main play/stop control bar.
    *
    * Provides editor buttons for:
    * - Entering play mode (simulated runtime)
    * - Stopping play mode (returning to editor)
    * - Undo / Redo buttons with Ctrl+Z / Ctrl+Y shortcuts
    *
    * Calls the corresponding callback functions (onPlay, onStop, onUndo, onRedo)
    * if assigned by GameApp.
*/
void EditorOverlay::DrawPlayBar()
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Play Controls", nullptr, flags);

    const float btnW = 110.0f;
    const float btnH = 28.0f;
    const float gap = 10.0f;

    EditorPlayControlsState st{ isPlaying, isPaused };
    EditorPlayControlsCallbacks cb{ onPlay, onPause, onStop };

    PlayControlsUIResult r = PlayStopSystem::DrawPlayControls(st, cb, btnW, btnH, gap);

    if (r.pressed) {
       
        PlayStopSystem::UpdatePlayControls(st, cb, r.action);

        isPlaying = st.isPlaying;
        isPaused = st.isPaused;
    }

    if (ImGui::Button("Undo (Ctrl+Z)", ImVec2(btnW, btnH))) {
        if (onUndo) onUndo();
    }
    ImGui::SameLine(0.0f, gap);
    if (ImGui::Button("Redo (Ctrl+Y)", ImVec2(btnW, btnH))) {
        if (onRedo) onRedo();
    }

    ImGui::End();
}

/**
    * @brief Displays the object spawning panel.
    *
    * Allows the user to:
    * - Choose a prefab/texture
    * - Set spawn position, scale, and rotation
    * - Spawn objects or players
    * - Remove selected entities or remove all
    *
    * Also draws a world-position anchored crosshair inside the viewport,
    * showing where new entities will be created.
    *
    * Records creation/destruction events for Undo/Redo when not in play mode.
*/
void EditorOverlay::DrawSpawner() {
    if (!glfwGetCurrentContext()) return;
    const ImGuiViewport* vp = ImGui::GetMainViewport();

    ImGui::Begin("Spawner", nullptr,
        ImGuiWindowFlags_NoCollapse);

    // --- capture new relative position if  window is moved ---
    {
        ImVec2 cur = ImGui::GetWindowPos();
        // convert back to % of Work area and clamp to [0..1]
        spawnerPosPct.x = (cur.x - vp->WorkPos.x) / vp->WorkSize.x;
        spawnerPosPct.y = (cur.y - vp->WorkPos.y) / vp->WorkSize.y;
        spawnerPosPct.x = std::max(0.0f, std::min(1.0f, spawnerPosPct.x));
        spawnerPosPct.y = std::max(0.0f, std::min(1.0f, spawnerPosPct.y));
    }

    static const char* kChoices[] = { "crate", "decorative" };
    ImGui::Combo("Texture", &texChoice, kChoices, IM_ARRAYSIZE(kChoices));
    ImGui::DragFloat2("Position", &spawnPos.x, 1.0f, -2000.0f, 2000.0f);
    ImGui::DragFloat2("Scale", &spawnScale.x, 1.0f, 1.0f, 512.0f);
    ImGui::DragFloat("Rotation", &spawnRotDeg, 1.0f, -360.0f, 360.0f);

    if (ImGui::Button("Add Textured Object"))
    {
        spawnRequest = {
            SpawnRequest::Type::Object,
            spawnPos, spawnScale, spawnRotDeg,
            texChoice,
            kChoices[texChoice]
        };
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Textured Player"))
        spawnRequest = { SpawnRequest::Type::Player, spawnPos, spawnScale, spawnRotDeg, texChoice };

    const auto& entities = context.GetEntities();
    const float fullW = ImGui::GetContentRegionAvail().x;
   /* if (ImGui::BeginListBox("Entities", ImVec2(fullW, 120))) {
        for (int i = 0; i < (int)entities.size(); ++i) {
            bool sel = (i == selectedEntity);
            std::string label = "Entity " + std::to_string(entities[i]);
            if (ImGui::Selectable(label.c_str(), sel))
                selectedEntity = i;
            if (sel)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }*/

    if (ImGui::BeginListBox("Entities", ImVec2(fullW, 120))) {
        for (int i = 0; i < (int)entities.size(); ++i) {
            bool sel = (i == selectedEntity);

            // Use an ID so the visible label doesn't need to be unique
            ImGui::PushID((int)entities[i]);

            // Stack buffer = no operator new / std::string allocs here
            char label[64];
            std::snprintf(label, sizeof(label), "Entity %u", (unsigned)entities[i]);

            if (ImGui::Selectable(label, sel))
                selectedEntity = i;

            if (sel) ImGui::SetItemDefaultFocus();

            ImGui::PopID();
        }
        ImGui::EndListBox();
    }

    if (selectedEntity >= 0 && selectedEntity < (int)entities.size())
    {
        if (ImGui::Button("Remove Selected"))
        {
            Entity del = entities[selectedEntity];

            int gx = 0, gy = 0;
            if (TryFindLabyrinthWallCell(del, gx, gy))
            {
                RemoveLabyrinthWallVariant(gx, gy);
                UnregisterLabyrinthWallEntity(gx, gy);
            }

            if ((!isPlaying || isPaused) && onRecordEntityDeleted)
                onRecordEntityDeleted(del);

            pendingDestroyList.push_back(del);
            selectedEntity = -1;
        }
    }


    if (ImGui::Button("Remove All")) {
        for (auto e : entities)
        {
            if ((!isPlaying || isPaused) && onRecordEntityDeleted)
                onRecordEntityDeleted(e);
            pendingDestroyList.push_back(e);
        }
    }

    GLFWwindow* win = glfwGetCurrentContext();
    if (win && activeCamera) {

        // Only draw if we know where the game image is
        if (gViewportImageSize.x > 0.0f && gViewportImageSize.y > 0.0f)
        {
            // 1) World -> camera framebuffer (bottom-left origin, pixels)
            Vector2 screenBL_FB = activeCamera->WorldToScreen(spawnPos);

            // 2) Normalize into [0,1] in the camera viewport
            float u = screenBL_FB.x / (float)activeCamera->viewportWidth;
            float v = screenBL_FB.y / (float)activeCamera->viewportHeight;

            // Optional clamp
            u = std::clamp(u, 0.0f, 1.0f);
            v = std::clamp(v, 0.0f, 1.0f);

            // 3) Map into the viewport image rect (top-left origin)
            ImVec2 p;
            p.x = gViewportImagePos.x + u * gViewportImageSize.x;
            p.y = gViewportImagePos.y + (1.0f - v) * gViewportImageSize.y;

            // Crosshair size based on image size
            const float minDim = std::min(gViewportImageSize.x, gViewportImageSize.y);
            const float r = std::clamp(minDim * 0.006f, 4.0f, 16.0f);
            const float halfLine = r * 2.0f;

            ImDrawList* dl = ImGui::GetForegroundDrawList();
            const ImU32 col = IM_COL32(255, 230, 0, 255);
            dl->AddCircleFilled(p, r, col, 16);
            dl->AddLine(ImVec2(p.x - halfLine, p.y), ImVec2(p.x + halfLine, p.y), col, 2.0f);
            dl->AddLine(ImVec2(p.x, p.y - halfLine), ImVec2(p.x, p.y + halfLine), col, 2.0f);
        }
    }

    // expose spawner window rect to position Assets Browser
    ImVec2 spawnerPos = ImGui::GetWindowPos();
    ImVec2 spawnerSize = ImGui::GetWindowSize();

    lastSpawnerPos = ImGui::GetWindowPos();
    lastSpawnerSize = ImGui::GetWindowSize();

    ImGui::End();
}

/**
    * @brief Shows a tree/list of all entities in the scene.
    *
    * Allows:
    * - Searching by name
    * - Selecting an entity
    * - Expanding nodes to show component lists
    * - Clicking to focus the camera on the selected entity
    * - Deleting the selected entity
    *
    * Integrates with camera focus, gizmo mode, selection state,
    * and Undo/Redo tracking.
*/
void EditorOverlay::DrawHierarchy()
{
    if (!glfwGetCurrentContext()) return;
 
    //  guard collapsed/hidden window
    if (!ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();   //
        return;        
    }

    // Search filter bar
    static ImGuiTextFilter filter;
    ImGui::Text("Search:");
    float w = std::max(1.0f, ImGui::GetContentRegionAvail().x - 20.0f);
    filter.Draw("EntityFilter", w);

    const auto& entities = context.GetEntities();
    if (entities.empty()) {
        ImGui::TextDisabled("No entities in scene.");
    }
    else {
        // Scrollable area so entity list will not grow too large
        if (ImGui::BeginChild("EntityList", ImVec2(0, -40), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {

            const auto& sigs = context.GetEntitySignatures();
            for (int i = 0; i < (int)entities.size(); ++i) {
                std::string label = context.GetEntityDisplayName(entities[i]);

                // Skip entities that don't match the filter text
                if (!filter.PassFilter(label.c_str()))
                    continue;

                bool sel = (i == selectedEntity);

                // Expandable tree node per entity
                if (ImGui::TreeNodeEx(label.c_str(),
                    ImGuiTreeNodeFlags_SpanFullWidth |
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    (sel ? ImGuiTreeNodeFlags_Selected : 0)))
                {
                    // --- Component list ---
                    auto it = sigs.find(entities[i]);
                    if (it != sigs.end()) {
                        const Signature& sig = it->second;
                        ImGui::TextDisabled("Components:");
                        if (sig.test(TRANSFORM))         ImGui::BulletText("Transform");
                        if (sig.test(MESHRENDERER))     ImGui::BulletText("MeshRenderer");
                        if (sig.test(COLLIDER))         ImGui::BulletText("Collider");
                        if (sig.test(PLAYERCONTROLLER)) ImGui::BulletText("PlayerController");
                        if (sig.test(SPRITEANIMATOR))   ImGui::BulletText("SpriteAnimator");
                        if (sig.test(ENEMYCONTROLLER)) ImGui::BulletText("EnemyController");
                    }

                    ImGui::TreePop();
                }

                if (ImGui::IsItemClicked()) {
                    selectedEntity = i;
                    focusPropsNextFrame = true;

                    // Only move camera / follow when gizmo is enabled
                    if (gizmoEnabled) {
                        // camera focus on selection from hierarchy
                        if (onRequestCameraLock) onRequestCameraLock(false);
                        if (activeCamera) {
                            Entity e = entities[i];
                            if (const Transform* t = context.TryGetTransform(e)) {
                                Vector2 size = t->GetScale();
                                if (const Collider* c = context.TryGetCollider(e)) size = c->size;
                                const Vector2 center = t->GetPosition() + size * 0.5f;

                                /*activeCamera->setPosition(center - Vector2(activeCamera->viewportWidth * 0.5f,
                                    activeCamera->viewportHeight * 0.5f));*/
                                Vector2 viewSize = activeCamera->GetViewSizeWorld();
                                activeCamera->setPosition(center - viewSize * 0.5f);
                            }
                        }
                        // Start editor-side follow on this selection
                        followSelection = true;
                        followEntity = entities[i];
                        navMode = false; // we’re in follow selection mode, not Nav
                    }
                }   else {
                    followSelection = false;
                }
            }

            ImGui::EndChild();
        }
    }

    ImGui::Separator();
    if (selectedEntity >= 0 && selectedEntity < (int)entities.size()) {
        // Cache selected entity/index for this UI blk
        const int   selIndex = selectedEntity;
        const Entity selEntity = entities[selIndex];

        bool deletedThisFrame = false;

        if (ImGui::Button("Delete Selected")) {
            Entity del = entities[selectedEntity];

            int gx = 0, gy = 0;
            if (TryFindLabyrinthWallCell(del, gx, gy))
            {
                RemoveLabyrinthWallVariant(gx, gy);
                UnregisterLabyrinthWallEntity(gx, gy);
            }

            if ((!isPlaying || isPaused) && onRecordEntityDeleted)
                onRecordEntityDeleted(del);

            pendingDestroyList.push_back(del);
            deletedThisFrame = true;
        }
        ImGui::SameLine();
        ImGui::Text("Selected: %d", (int)selEntity);

        // Only clear selection aft finished using
        if (deletedThisFrame)
        {
            selectedEntity = -1;
            followSelection = false;
            followEntity = 0;
        }
    }

    ImGui::End();
}

/**
 * @brief Displays the Item Palette for placing prefabs and wall variants.
 * 
 * Provides a tabbed interface for:
 * - Enemies: Spawning enemy prefabs.
 * - Environment: Spawning environment prefabs (excluding Player).
 * - Walls: Placing labyrinth wall variants with specific textures.
 * 
 * Supports drag-and-drop for prefabs and wall textures into the scene.
 */
void EditorOverlay::DrawItemPalette()
{
    if (!ImGui::Begin("Item Palette##Overlay"))
    {
        ImGui::End();
        return;
    }

    static const char* kWallKeys[] = {
        "wall_B&T",
        "wall_BL_1","wall_BL_2",
        "wall_BR_1","wall_BR_2",
        "wall_B_2",
        "wall_L_1","wall_L_2","wall_L_3",
        "wall_R_1","wall_R_2","wall_R_3",
        "wall_TL","wall_TR",
        "Floor_Tile"
    };

    if (ImGui::BeginTabBar("##OverlayPaletteTabs"))
    {

        /* ---------------- Enemies ---------------- */

        if (ImGui::BeginTabItem("Enemies"))
        {
            for (const auto& prefabName : availablePrefabs_)
            {
                if (prefabName.find("Enemy") == std::string::npos)
                    continue;

                unsigned int tex =
                    getPrefabTexture ? getPrefabTexture(prefabName) : 0;

                if (tex)
                {
                    ImGui::Image((ImTextureID)(uintptr_t)tex,
                        ImVec2(24, 24),
                        ImVec2(0, 1),
                        ImVec2(1, 0));
                    ImGui::SameLine();
                }

                ImGui::Selectable(prefabName.c_str());

                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload(
                        "PREFAB_KEY",
                        prefabName.c_str(),
                        prefabName.size() + 1);

                    ImGui::Text("%s", prefabName.c_str());

                    ImGui::EndDragDropSource();
                }
            }

            ImGui::EndTabItem();
        }

        /* ---------------- Environment ---------------- */

        if (ImGui::BeginTabItem("Environment"))
        {
            for (const auto& prefabName : availablePrefabs_)
            {
                if (prefabName == "Player")
                    continue;

                unsigned int tex =
                    getPrefabTexture ? getPrefabTexture(prefabName) : 0;

                if (tex)
                {
                    ImGui::Image((ImTextureID)(uintptr_t)tex,
                        ImVec2(24, 24),
                        ImVec2(0, 1),
                        ImVec2(1, 0));
                    ImGui::SameLine();
                }

                ImGui::Selectable(prefabName.c_str());

                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload(
                        "PREFAB_KEY",
                        prefabName.c_str(),
                        prefabName.size() + 1);

                    ImGui::Text("%s", prefabName.c_str());

                    ImGui::EndDragDropSource();
                }
            }

            ImGui::EndTabItem();
        }

        /* ---------------- Walls ---------------- */

        if (ImGui::BeginTabItem("Walls"))
        {
            for (const char* key : kWallKeys)
            {
                unsigned int tex =
                    getTextureID ? getTextureID(key) : 0;

                if (tex)
                {
                    ImGui::Image((ImTextureID)(uintptr_t)tex,
                        ImVec2(24, 24),
                        ImVec2(0, 1),
                        ImVec2(1, 0));
                    ImGui::SameLine();
                }

                ImGui::Selectable(key);

                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload(
                        "WALL_TEXTUREKEY",
                        key,
                        strlen(key) + 1);

                    ImGui::Text("%s", key);

                    ImGui::EndDragDropSource();
                }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();

    if (ImGui::Button("Save Labyrinth Variants"))
    {
        if (onSaveLabyrinthVariants)
        {
            onSaveLabyrinthVariants();

            DebugConsole::Get().AddFormattedMessage(
                LogLevel::Info,
                "File type: Editor.cpp",
                "Details: [Item Palette] Saved labyrinth variants\n");
        }
    }

    ImGui::SameLine();
    ImGui::TextDisabled("Writes updated wall variants to JSON");

    ImGui::End();
}

/**
    * @brief Enables direct dragging of entities inside the gameplay viewport.
    *
    * Steps performed:
    * 1. Hit-test mouse against entity bounds in world space.
    * 2. Select entity when clicked.
    * 3. Initiate dragging if the click is within pick radius.
    * 4. Convert screen-space mouse to world coordinates via the camera.
    * 5. Move entity while respecting parent transforms and collider blocking.
    * 6. Record before/after transform states for Undo/Redo.
    *
    * @note Dragging is disabled when gizmo is active, ImGui is consuming input,
    *       or the editor is in navigation mode.
*/
void EditorOverlay::HandleViewportDrag()
{
    if (!glfwGetCurrentContext()) return;
    if (navMode) return;
    ImGuiIO& io = ImGui::GetIO();

    if (gizmoEnabled) {
        if (ImGuizmo::IsUsing() || ImGuizmo::IsOver()) return;
    }

    bool hasSelection = (selectedEntity >= 0 && selectedEntity < (int)context.GetEntities().size());
    if (hasSelection) {
        if (ImGuizmo::IsUsing())
            return;
        if (ImGuizmo::IsOver() && (io.MouseDown[0] || io.MouseDown[1]))
            return;
    }

    if (io.WantCaptureMouse) return; // skip if ImGui using mouse

    const auto& entities = context.GetEntities();
    if (entities.empty()) return;
    //if (selectedEntity < 0 || selectedEntity >= (int)entities.size())
    //    return;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 mouseTL = io.MousePos;

    // Need a valid viewport image rect
    if (gViewportImageSize.x <= 0.0f || gViewportImageSize.y <= 0.0f)
        return;

    // Reject if mouse is outside the *game image* rect
    if (mouseTL.x < gViewportImagePos.x ||
        mouseTL.y < gViewportImagePos.y ||
        mouseTL.x > gViewportImagePos.x + gViewportImageSize.x ||
        mouseTL.y > gViewportImagePos.y + gViewportImageSize.y) {
        return;
    }

    auto& in = eng::input();

    // Mouse relative to the game image, normalized [0,1]
    float u = (mouseTL.x - gViewportImagePos.x) / gViewportImageSize.x;
    float v = (gViewportImagePos.y + gViewportImageSize.y - mouseTL.y) / gViewportImageSize.y;

    // Extra safety: if somehow outside, bail
    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
        return;

    // Map to camera framebuffer pixels (bottom-left origin)
    float fbX = u * (float)activeCamera->viewportWidth;
    float fbY = v * (float)activeCamera->viewportHeight;
    Vector2 mouseBL(fbX, fbY);

    // Fallback if somehow no camera (rare)
    Vector2 mouse = activeCamera
        ? activeCamera->ScreenToWorld(mouseBL)
        : ScreenToLogical(mouseTL, vp);

    if (in.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        int hitIndex = -1;
        for (int i = (int)entities.size() - 1; i >= 0; --i) { // back-to-front
            Entity e = entities[i];
            const Transform* t = context.TryGetTransform(e);
            if (!t) continue;

            // Skip merged wall blocks
            if (context.TryGetRenderer(e) == nullptr)
                continue;

            Vector2 size = t->GetScale();
            if (const Collider* c = context.TryGetCollider(e))
                size = c->size;

            const Vector2 aMin = t->GetPosition();
            const Vector2 aMax = aMin + size;

            const bool inside =
                (mouse.x >= aMin.x && mouse.x <= aMax.x) &&
                (mouse.y >= aMin.y && mouse.y <= aMax.y);

            if (inside) { hitIndex = i; break; }
        }
        if (hitIndex != -1) {
            selectedEntity = hitIndex;
            focusPropsNextFrame = true;
            if (gizmoEnabled) {

                // --- camera: focus on clicked entity instead of player ---
                if (onRequestCameraLock) onRequestCameraLock(false); // unlock follow-player
                if (activeCamera) {
                    // center camera on entity
                    Entity e = entities[hitIndex];
                    if (const Transform* t = context.TryGetTransform(e)) {
                        Vector2 size = t->GetScale();
                        if (const Collider* c = context.TryGetCollider(e)) size = c->size;
                        const Vector2 center = t->GetPosition() + size * 0.5f;
                        // Camera pos is bottom-left; move so that 'center' lands in the middle of the view
                        Vector2 viewSize = activeCamera->GetViewSizeWorld();
                        activeCamera->setPosition(center - viewSize * 0.5f);
                    }
                }
                // Start following this clicked entity
                followSelection = true;
                followEntity = entities[hitIndex];
                navMode = false;
            } else {
                followSelection = false;
            }
        }
        else {
            // click on empty space: close prop (by clearing selection)
            selectedEntity = -1;
            dragging = false;
            dragEntity = INVALID_ENTITY;
            if (gizmoEnabled)
            gizmoOp = ImGuizmo::TRANSLATE;

            // Stop editor-side follow and hand control back to default camera
            followSelection = false;
            followEntity = 0;

            // --- camera: refocus back on player ---
            if (!navMode && onRequestCameraLock) onRequestCameraLock(true);  // GameApp will re-center next update
        }
    }

    // Delete key (works immediately after viewport selection)
    if (!io.WantTextInput) {
        if (in.isKeyPressed(GLFW_KEY_DELETE)) {
            if (selectedEntity >= 0 && selectedEntity < (int)entities.size()) {
                Entity del = entities[selectedEntity];

                int gx = 0, gy = 0;
                if (TryFindLabyrinthWallCell(del, gx, gy))
                {
                    RemoveLabyrinthWallVariant(gx, gy);
                    UnregisterLabyrinthWallEntity(gx, gy);
                }

                if ((!isPlaying || isPaused) && onRecordEntityDeleted)
                {
					onRecordEntityDeleted(del);
                }
                pendingDestroyList.push_back(del);
                selectedEntity = -1;
                dragging = false;
                dragEntity = INVALID_ENTITY;
                if (gizmoEnabled)
                gizmoOp = ImGuizmo::TRANSLATE;
                return; // stop further processing this frame
            }
        }
    }

    // If nothing selected after possible pick, we’re done
    if (selectedEntity < 0 || selectedEntity >= (int)entities.size())
        return;

    Entity e = entities[selectedEntity];

    bool hovered = false;
    Vector2 centerScreenBL{ 0.f, 0.f };

    if (const Transform* t = context.TryGetTransform(e)) {
        Vector2 size = t->GetScale();
        if (const Collider* c = context.TryGetCollider(e))
            size = c->size;

        // Center in world space (meshes are 0..1 anchored at top-left)
        const Vector2 centerWorld = t->GetPosition() + size * 0.5f;

        // Project center to screen (BL pixels) for a zoom-invariant pick radius
        centerScreenBL = activeCamera
            ? activeCamera->WorldToScreen(centerWorld)
            : centerWorld; // if no camera, treat world==screen (BL)

        constexpr float kPickRadiusPx = 18.0f;
        const float dx = mouseBL.x - centerScreenBL.x;
        const float dy = mouseBL.y - centerScreenBL.y;
        hovered = (dx * dx + dy * dy) <= (kPickRadiusPx * kPickRadiusPx);
    }

    // Start drag (pressed or fallback down) 
    if (in.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) ||
        (!dragging && in.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))) {
        if (const Transform* t = context.TryGetTransform(e)) {
            // Use hovered test 
            if (hovered) {
                dragging = true;
                dragEntity = e;
                const bool allowEdit = (!isPlaying) || isPaused;
                if (allowEdit && onRecordBeforeTransform && e != INVALID_ENTITY)
                    onRecordBeforeTransform(e);

                if (const Transform* t2 = context.TryGetTransform(e))
                    dragOffset = t2->GetPosition() - mouse;
            }
        }
    }

    // Drag update / end
    if (dragging) {
        if (!in.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
        {
            dragging = false;

            // FIX — UNDO AFTER
            const bool allowEdit = (!isPlaying) || isPaused;
            if (allowEdit && onRecordAfterTransform && dragEntity != INVALID_ENTITY)
                onRecordAfterTransform(dragEntity);

            dragEntity = INVALID_ENTITY;
            return;
        }

        if (in.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
            if (Transform* t = context.GetTransform(dragEntity)) {
                Vector2 startPos = t->GetPosition();
                Vector2 target = mouse + dragOffset;
                const Collider* myCol = context.TryGetCollider(dragEntity);

                auto applyWorldPos = [&](Transform* t, const Vector2& worldPos) {
                    if (t->GetParent() == INVALID_ENTITY) {
                        t->SetPosition(worldPos); // root-safe: mirrors locals internally
                        return;
                    }
                    if (const Transform* p = context.TryGetTransform(t->GetParent())) {
                        // world -> parent space (inverse TRS)
                        Vector2 rel = worldPos - p->GetPosition();

                        // un-rotate by parent
                        /*const float pr = -p->GetRotation();
                        const float c = std::cos(pr), s = std::sin(pr);*/
                        const float prDeg = -p->GetRotation();
                        const float prRad = prDeg * (3.14159265358979323846f / 180.0f);
                        const float c = std::cos(prRad), s = std::sin(prRad);
                        Vector2 unrot{ c * rel.x - s * rel.y, s * rel.x + c * rel.y };

                        // un-scale by parent (component-wise)
                        t->SetLocalPosition(unrot);
                        /*const Vector2 ps = p->GetScale();
                        Vector2 local{
                            (ps.x != 0.f ? unrot.x / ps.x : unrot.x),
                            (ps.y != 0.f ? unrot.y / ps.y : unrot.y)
                        };
                        t->SetLocalPosition(local);*/
                    }
                    else {
                        // parent missing? fall back to world write
                        t->SetPosition(worldPos);
                    }
                };

                if (myCol) {
                    const auto& all = context.GetEntities();
                    Vector2 clampedPos = target; // will be adjusted if blocked

                    for (Entity other : all) {
                        if (other == dragEntity) continue;

                        const Collider* c = context.TryGetCollider(other);
                        const Transform* ot = context.TryGetTransform(other);
                        if (!c || !ot) continue;

                        // WORLD-space AABBs
                        Vector2 aMin = clampedPos;
                        Vector2 aMax = clampedPos + myCol->size;
                        Vector2 bMin = ot->GetPosition();
                        Vector2 bMax = ot->GetPosition() + c->size;

                        // Overlap test
                        bool overlapX = (aMin.x < bMax.x) && (aMax.x > bMin.x);
                        bool overlapY = (aMin.y < bMax.y) && (aMax.y > bMin.y);

                        if (overlapX && overlapY) {
                            Vector2 prevMin = startPos;
                            Vector2 prevMax = startPos + myCol->size;

                            bool fromLeft = prevMax.x <= bMin.x;
                            bool fromRight = prevMin.x >= bMax.x;
                            bool fromTop = prevMax.y <= bMin.y;
                            bool fromBottom = prevMin.y >= bMax.y;

                            if (fromLeft)        clampedPos.x = bMin.x - myCol->size.x;
                            else if (fromRight)  clampedPos.x = bMax.x;
                            else if (fromTop)    clampedPos.y = bMin.y - myCol->size.y;
                            else if (fromBottom) clampedPos.y = bMax.y;
                        }
                    }

                    applyWorldPos(t, clampedPos);
                }
                else {
                    applyWorldPos(t, target);
                }
            }
        }
        //else {
        //    dragging = false;
        //    dragEntity = INVALID_ENTITY;
        //}
    }
}


/**
    * @brief Shows editable component properties for the selected entity.
    *
    * Supports editing:
    * - Transform (position, scale, rotation)
    * - MeshRenderer (color, texture, mesh)
    * - Collider (size, trigger)
    * - SpriteAnimator (frames, speed, sheet info)
    * - ScriptComponent (script selection from registered names)
    *
    * Also exposes prefab instance controls:
    * - Revert to Prefab
    * - Apply Instance to Prefab
    * - Save All Prefabs
    *
    * Records transform edits as Undo/Redo operations when applicable.
*/
void EditorOverlay::DrawPropertyEditor()
{
    if (!glfwGetCurrentContext()) return;
    const auto& entities = context.GetEntities();
    if (selectedEntity < 0 || selectedEntity >= (int)entities.size())
        return;

    Entity e = entities[selectedEntity];

    if (focusPropsNextFrame) {
        ImGui::SetNextWindowFocus();
        focusPropsNextFrame = false;
    }

    ImGui::Begin("Property Selector##Overlay", nullptr,
        ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("##PropertySelectorTabsOverlay"))
    {
        if (ImGui::BeginTabItem("Properties"))
        {
            ImGui::Text("Entity ID: %d", e);

            // --- Transform ---
            if (Transform* t = context.GetTransform(e))
            {
                ImGui::SeparatorText("Transform");
                Vector2 position = t->GetPosition();
                Vector2 scale = t->GetScale();
                float rot = t->GetRotation();

                bool changed = false;
                changed |= ImGui::DragFloat2("Position", &position.x, 1.0f);
                changed |= ImGui::DragFloat2("Scale", &scale.x, 0.1f, 0.01f, 512.f);
                changed |= ImGui::DragFloat("Rotation", &rot, 1.0f, -360.f, 360.f);

                // BEGIN — only once
                if (changed && !transformEditing)
                {
                    transformEditing = true;
                    transformEditEntity = e;

                    const bool allowEdit = (!isPlaying) || isPaused;
                    if (allowEdit && onRecordBeforeTransform)
                        onRecordBeforeTransform(e);
                }

                if (changed) {
                    if (t->GetParent() == INVALID_ENTITY) {
                        // Root: world setters (your Transform mirrors locals for roots)
                        t->SetPosition(position);
                        t->SetScale(scale);
                        t->SetRotation(rot);
                    }
                    else if (const Transform* p = context.TryGetTransform(t->GetParent())) {
                        // --- position: world -> local ---
                        Vector2 rel = position - p->GetPosition();
                        const float pr = -p->GetRotation();
                        const float c = std::cos(pr), s = std::sin(pr);
                        Vector2 unrot{ c * rel.x - s * rel.y, s * rel.x + c * rel.y };
                        t->SetLocalPosition(unrot);

                        // --- rotation: local = world - parentWorld ---
                        t->SetLocalRotation(rot - p->GetRotation());

                        // --- scale: keep child size independent of parent ---
                        t->SetLocalScale(scale);
                    }
                    else {
                        // Parent not found: treat as root
                        t->SetPosition(position);
                        t->SetScale(scale);
                        t->SetRotation(rot);
                    }
                }
                // END — only once
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && transformEditing)
                {
                    transformEditing = false;

                    const bool allowEdit = (!isPlaying) || isPaused;
                    if (allowEdit && onRecordAfterTransform && transformEditEntity != INVALID_ENTITY)
                        onRecordAfterTransform(transformEditEntity);
                }
            }

            // ---------------------------------------------------------------------
            // Component presence toggles (Add/Remove components in Edit mode)
            // ---------------------------------------------------------------------
            ImGui::SeparatorText("Components");

    if (isPlaying && !isPaused)
    {
        ImGui::TextDisabled("Component add/remove disabled while Playing.");
    }
    else
    {
        // Static defaults for newly created components
        static Vector3 newRendererColor{ 1.f, 1.f, 1.f };
        static int     newRendererTextureIndex = 0;

        static Vector2 newColliderSize{ 32.f, 32.f };
        static bool    newColliderTrigger = false;

        // Defaults for a newly-added SpriteAnimator
        static int   newAnimStart = 0;
        static int   newAnimEnd = 0;
        static float newAnimSpeed = 8.0f;

        bool hasRenderer = (context.TryGetRenderer(e) != nullptr);
        bool hasCollider = (context.TryGetCollider(e) != nullptr);
        bool hasAnimator = (context.TryGetAnimator(e) != nullptr);
        bool hasController = (context.TryGetController(e) != nullptr);
        bool hasEmitter = (context.TryGetEmitter(e) != nullptr);
        bool hasSpeed = (context.TryGetSpeed(e) != nullptr);
        bool hasMass = (context.TryGetMass(e) != nullptr);
        bool hasPersistentTag = context.HasPersistentTag(e);
        bool hasLight = (context.TryGetLight(e) != nullptr);
        bool hasEnemyController = (context.TryGetEnemyController(e) != nullptr);


        // -------------------------------------------------
        // MeshRenderer creation parameters (when missing)
        // -------------------------------------------------
        if (!hasRenderer)
        {
            ImGui::Text("New MeshRenderer:");

            // Color for the new renderer
            ImGui::ColorEdit3("Color##newRenderer", &newRendererColor.x);

            // Texture selection for the new renderer
            const auto& textures = context.GetTextureList();
            if (!textures.empty())
            {
                // Build display names for ImGui
                std::vector<const char*> texNames;
                texNames.reserve(textures.size());
                for (auto& s : textures)
                    texNames.push_back(s.c_str());

                if (newRendererTextureIndex >= (int)texNames.size())
                    newRendererTextureIndex = (int)texNames.size() - 1;
                if (newRendererTextureIndex < 0)
                    newRendererTextureIndex = 0;

                ImGui::Combo("Texture##newRenderer",
                    &newRendererTextureIndex,
                    texNames.data(),
                    (int)texNames.size());
            }
            else
            {
                ImGui::TextDisabled("No textures loaded.");
            }
        }

        // -------------------------------------------------
        // Collider creation parameters (when missing)
        // -------------------------------------------------
        if (!hasCollider)
        {
            ImGui::Text("New Collider:");
            ImGui::DragFloat2("Size##newCollider", &newColliderSize.x, 1.0f, 1.0f, 512.0f);
            ImGui::Checkbox("Is Trigger##newCollider", &newColliderTrigger);
        }

        // -------------------------------------------------
        // SpriteAnimator creation parameters (when missing)
        // -------------------------------------------------
        if (!hasAnimator)
        {
            ImGui::Text("New SpriteAnimator:");
            ImGui::DragInt("Start Frame##newAnim", &newAnimStart, 1, 0, 1000);
            ImGui::DragInt("End Frame##newAnim", &newAnimEnd, 1, 0, 1000);
            ImGui::DragFloat("Speed##newAnim", &newAnimSpeed, 0.1f, 0.0f, 60.0f);
        }

        // -------------------------------------------------
        // MeshRenderer toggle
        // -------------------------------------------------
        bool rendererToggle = hasRenderer;
        if (ImGui::Checkbox("MeshRenderer##has", &rendererToggle))
        {
            bool beforeHasRenderer = hasRenderer;
            Vector3 beforeColor{ 1.f, 1.f, 1.f };
            unsigned beforeTexture = 0;
            Mesh2D* beforeMesh = nullptr;

            MeshRenderer* mr = context.GetRenderer(e);

            if (hasRenderer && mr)
            {
                beforeColor = mr->GetColor();
                beforeTexture = static_cast<unsigned>(mr->GetTexture());
                beforeMesh = mr->GetMesh();
            }

            if (rendererToggle && !hasRenderer)
            {
                if (MeshRenderer* newMr = context.AddRendererComponent(e))
                {
                    auto it = removedRendererState.find(e);
                    if (it != removedRendererState.end() && it->second.valid)
                    {
                        newMr->SetColor(it->second.color);
                        newMr->SetTexture(it->second.texture);
                        newMr->SetMesh(it->second.mesh);
                    }
                    else
                    {
                        newMr->SetColor(newRendererColor);

                        const auto& textures = context.GetTextureList();
                        if (!textures.empty() &&
                            newRendererTextureIndex >= 0 &&
                            newRendererTextureIndex < (int)textures.size())
                        {
                            const std::string& texName = textures[newRendererTextureIndex];
                            context.ApplyTexture(e, texName);
                        }
                    }
                }
            }
            else if (!rendererToggle && hasRenderer)
            {
                removedRendererState[e] = {
                    true,
                    beforeColor,
                    beforeTexture,
                    beforeMesh
                };

                context.RemoveRendererComponent(e);
            }

            bool afterHasRenderer = false;
            Vector3 afterColor{ 1.f, 1.f, 1.f };
            unsigned afterTexture = 0;
            Mesh2D* afterMesh = nullptr;

            if (MeshRenderer* afterMr = context.GetRenderer(e))
            {
                afterHasRenderer = true;
                afterColor = afterMr->GetColor();
                afterTexture = static_cast<unsigned>(afterMr->GetTexture());
                afterMesh = afterMr->GetMesh();
            }

            if (undoRedoPtr)
            {
                undoRedoPtr->Push_MeshRendererToggle(
                    e,
                    beforeHasRenderer,
                    beforeColor,
                    beforeTexture,
                    beforeMesh,
                    afterHasRenderer,
                    afterColor,
                    afterTexture,
                    afterMesh
                );
            }
        }

        // -------------------------------------------------
        // Collider toggle
        // -------------------------------------------------
        bool colliderToggle = hasCollider;
        if (ImGui::Checkbox("Collider##has", &colliderToggle))
        {
            bool beforeHasCollider = hasCollider;
            Vector2 beforeSize{ 1.f, 1.f };
            bool beforeTrigger = false;

            if (Collider* oldC = context.GetCollider(e))
            {
                beforeSize = oldC->size;
                beforeTrigger = oldC->isTrigger;
            }

            if (colliderToggle && !hasCollider)
            {
                if (Collider* c = context.AddColliderComponent(e))
                {
                    auto it = removedColliderStates.find(e);
                    if (it != removedColliderStates.end() && it->second.valid)
                    {
                        c->size = it->second.size;
                        c->isTrigger = it->second.isTrigger;
                    }
                    else
                    {
                        c->size = newColliderSize;
                        c->isTrigger = newColliderTrigger;
                    }
                }
            }
            else if (!colliderToggle && hasCollider)
            {
                removedColliderStates[e] =
                {
                    true,
                    beforeSize,
                    beforeTrigger
                };

                context.RemoveColliderComponent(e);
            }

            bool afterHasCollider = false;
            Vector2 afterSize{ 1.f, 1.f };
            bool afterTrigger = false;

            if (Collider* afterC = context.GetCollider(e))
            {
                afterHasCollider = true;
                afterSize = afterC->size;
                afterTrigger = afterC->isTrigger;
            }

            if (undoRedoPtr)
            {
                undoRedoPtr->Push_ColliderToggle(
                    e,
                    beforeHasCollider,
                    beforeSize,
                    beforeTrigger,
                    afterHasCollider,
                    afterSize,
                    afterTrigger
                );
            }
        }

        // -------------------------------------------------
        // SpriteAnimator toggle
        // -------------------------------------------------
        bool animatorToggle = hasAnimator;
        if (ImGui::Checkbox("SpriteAnimator##has", &animatorToggle))
        {
            bool beforeHasAnimator = hasAnimator;
            int beforeCur = 0;
            int beforeStart = 0;
            int beforeEnd = 0;
            float beforeSpeed = 0.0f;
            const SpriteSheet* beforeSheet = nullptr;

            SpriteAnimator* oldA = context.GetAnimator(e);
            if (hasAnimator && oldA)
            {
                beforeCur = oldA->cur;
                beforeStart = oldA->startFrame;
                beforeEnd = oldA->endFrame;
                beforeSpeed = oldA->speed;
                beforeSheet = oldA->sheet;
            }

            if (animatorToggle && !hasAnimator)
            {
                if (SpriteAnimator* a = context.AddAnimatorComponent(e))
                {
                    auto it = removedAnimatorStates.find(e);
                    if (it != removedAnimatorStates.end() && it->second.valid)
                    {
                        a->cur = it->second.cur;
                        a->startFrame = it->second.startFrame;
                        a->endFrame = it->second.endFrame;
                        a->speed = it->second.speed;
                        a->sheet = it->second.sheet;
                    }
                    else
                    {
                        a->startFrame = newAnimStart;
                        a->endFrame = newAnimEnd;
                        a->cur = newAnimStart;
                        a->speed = newAnimSpeed;
                    }
                }
            }
            else if (!animatorToggle && hasAnimator)
            {
                removedAnimatorStates[e] =
                {
                    true,
                    beforeCur,
                    beforeStart,
                    beforeEnd,
                    beforeSpeed,
                    beforeSheet
                };

                context.RemoveAnimatorComponent(e);
            }

            bool afterHasAnimator = false;
            int afterCur = 0;
            int afterStart = 0;
            int afterEnd = 0;
            float afterSpeed = 0.0f;
            const SpriteSheet* afterSheet = nullptr;

            if (SpriteAnimator* afterA = context.GetAnimator(e))
            {
                afterHasAnimator = true;
                afterCur = afterA->cur;
                afterStart = afterA->startFrame;
                afterEnd = afterA->endFrame;
                afterSpeed = afterA->speed;
                afterSheet = afterA->sheet;
            }

            if (undoRedoPtr)
            {
                undoRedoPtr->Push_SpriteAnimatorToggle(
                    e,
                    beforeHasAnimator,
                    beforeCur,
                    beforeStart,
                    beforeEnd,
                    beforeSpeed,
                    beforeSheet,
                    afterHasAnimator,
                    afterCur,
                    afterStart,
                    afterEnd,
                    afterSpeed,
                    afterSheet
                );
            }
        }

        // -------------------------------------------------
        // PlayerController toggle
        // -------------------------------------------------
        bool controllerToggle = hasController;
        if (ImGui::Checkbox("PlayerController##has", &controllerToggle))
        {
            const bool beforeHasController = hasController;
            const bool afterHasController = controllerToggle;

            if (afterHasController && !beforeHasController)
            {
                context.AddControllerComponent(e);
            }
            else if (!afterHasController && beforeHasController)
            {
                context.RemoveControllerComponent(e);
            }

            if (undoRedoPtr && beforeHasController != afterHasController)
            {
                undoRedoPtr->Push_PlayerControllerToggle(
                    e,
                    beforeHasController,
                    afterHasController
                );
            }
        }

        // -------------------------------------------------
        // SpeedComponent toggle
        // -------------------------------------------------
        bool speedToggle = hasSpeed;
        if (ImGui::Checkbox("SpeedComponent##has", &speedToggle))
        {
            const bool beforeHasSpeed = hasSpeed;
            const bool afterHasSpeed = speedToggle;

            if (afterHasSpeed && !beforeHasSpeed)
            {
                context.AddSpeedComponent(e);
            }
            else if (!afterHasSpeed && beforeHasSpeed)
            {
                context.RemoveSpeedComponent(e);
            }

            if (undoRedoPtr && beforeHasSpeed != afterHasSpeed)
            {
                undoRedoPtr->Push_SpeedComponentToggle(
                    e,
                    beforeHasSpeed,
                    afterHasSpeed
                );
            }
        }

        // -------------------------------------------------
        // MassComponent toggle
        // -------------------------------------------------
        bool massToggle = hasMass;
        if (ImGui::Checkbox("MassComponent##has", &massToggle))
        {
            const bool beforeHasMass = hasMass;
            const bool afterHasMass = massToggle;

            if (afterHasMass && !beforeHasMass)
            {
                context.AddMassComponent(e);
            }
            else if (!afterHasMass && beforeHasMass)
            {
                context.RemoveMassComponent(e);
            }

            if (undoRedoPtr && beforeHasMass != afterHasMass)
            {
                undoRedoPtr->Push_MassComponentToggle(
                    e,
                    beforeHasMass,
                    afterHasMass
                );
            }
        }


        // -------------------------------------------------
        // ParticleEmitter toggle
        // -------------------------------------------------
        bool emitterToggle = hasEmitter;
        if (ImGui::Checkbox("ParticleEmitter##has", &emitterToggle))
        {
            bool beforeHasEmitter = hasEmitter;
            bool beforeEnabled = false;
            float beforeRate = 50.f;
            float beforeParticleLife = 0.6f;
            Vector2 beforeOffset{ 0.f, 0.f };
            Vector2 beforeVelMin{ -30.f, 50.f };
            Vector2 beforeVelMax{ 30.f, 90.f };
            Vector3 beforeColorStart{ 1.f, 1.f, 1.f };
            Vector3 beforeColorEnd{ 0.8f, 0.8f, 0.8f };
            float beforeSizeStart = 10.f;
            float beforeSizeEnd = 1.f;
            Mesh2D* beforeQuad = nullptr;
            unsigned beforeTexture = 0;
            float beforeTimeAccumulator = 0.f;

            if (ParticleEmitter* oldEm = context.GetEmitter(e))
            {
                beforeEnabled = oldEm->enabled;
                beforeRate = oldEm->rate;
                beforeParticleLife = oldEm->particleLife;
                beforeOffset = oldEm->offset;
                beforeVelMin = oldEm->velMin;
                beforeVelMax = oldEm->velMax;
                beforeColorStart = oldEm->colorStart;
                beforeColorEnd = oldEm->colorEnd;
                beforeSizeStart = oldEm->sizeStart;
                beforeSizeEnd = oldEm->sizeEnd;
                beforeQuad = oldEm->quad;
                beforeTexture = static_cast<unsigned>(oldEm->texture);
                beforeTimeAccumulator = oldEm->timeAccumulator;
            }

            if (emitterToggle && !hasEmitter)
            {
                if (ParticleEmitter* em = context.AddEmitterComponent(e))
                {
                    auto it = removedEmitterStates.find(e);
                    if (it != removedEmitterStates.end() && it->second.valid)
                    {
                        em->enabled = it->second.enabled;
                        em->rate = it->second.rate;
                        em->particleLife = it->second.particleLife;
                        em->offset = it->second.offset;
                        em->velMin = it->second.velMin;
                        em->velMax = it->second.velMax;
                        em->colorStart = it->second.colorStart;
                        em->colorEnd = it->second.colorEnd;
                        em->sizeStart = it->second.sizeStart;
                        em->sizeEnd = it->second.sizeEnd;
                        em->quad = it->second.quad;
                        em->texture = static_cast<GLuint>(it->second.texture);
                        em->timeAccumulator = it->second.timeAccumulator;
                    }
                }
            }
            else if (!emitterToggle && hasEmitter)
            {
                removedEmitterStates[e] =
                {
                    true,
                    beforeEnabled,
                    beforeRate,
                    beforeParticleLife,
                    beforeOffset,
                    beforeVelMin,
                    beforeVelMax,
                    beforeColorStart,
                    beforeColorEnd,
                    beforeSizeStart,
                    beforeSizeEnd,
                    beforeQuad,
                    beforeTexture,
                    beforeTimeAccumulator
                };

                context.RemoveEmitterComponent(e);
            }

            bool afterHasEmitter = false;
            bool afterEnabled = false;
            float afterRate = 50.f;
            float afterParticleLife = 0.6f;
            Vector2 afterOffset{ 0.f, 0.f };
            Vector2 afterVelMin{ -30.f, 50.f };
            Vector2 afterVelMax{ 30.f, 90.f };
            Vector3 afterColorStart{ 1.f, 1.f, 1.f };
            Vector3 afterColorEnd{ 0.8f, 0.8f, 0.8f };
            float afterSizeStart = 10.f;
            float afterSizeEnd = 1.f;
            Mesh2D* afterQuad = nullptr;
            unsigned afterTexture = 0;
            float afterTimeAccumulator = 0.f;

            if (ParticleEmitter* afterEm = context.GetEmitter(e))
            {
                afterHasEmitter = true;
                afterEnabled = afterEm->enabled;
                afterRate = afterEm->rate;
                afterParticleLife = afterEm->particleLife;
                afterOffset = afterEm->offset;
                afterVelMin = afterEm->velMin;
                afterVelMax = afterEm->velMax;
                afterColorStart = afterEm->colorStart;
                afterColorEnd = afterEm->colorEnd;
                afterSizeStart = afterEm->sizeStart;
                afterSizeEnd = afterEm->sizeEnd;
                afterQuad = afterEm->quad;
                afterTexture = static_cast<unsigned>(afterEm->texture);
                afterTimeAccumulator = afterEm->timeAccumulator;
            }

            if (undoRedoPtr)
            {
                undoRedoPtr->Push_ParticleEmitterToggle(
                    e,
                    beforeHasEmitter,
                    beforeEnabled,
                    beforeRate,
                    beforeParticleLife,
                    beforeOffset,
                    beforeVelMin,
                    beforeVelMax,
                    beforeColorStart,
                    beforeColorEnd,
                    beforeSizeStart,
                    beforeSizeEnd,
                    beforeQuad,
                    beforeTexture,
                    beforeTimeAccumulator,
                    afterHasEmitter,
                    afterEnabled,
                    afterRate,
                    afterParticleLife,
                    afterOffset,
                    afterVelMin,
                    afterVelMax,
                    afterColorStart,
                    afterColorEnd,
                    afterSizeStart,
                    afterSizeEnd,
                    afterQuad,
                    afterTexture,
                    afterTimeAccumulator
                );
            }
        }
        // -------------------------------------------------
        // PersistentTag toggle
        // -------------------------------------------------
        bool persistentTagToggle = hasPersistentTag;
        if (ImGui::Checkbox("PersistentTag##has", &persistentTagToggle))
        {
            const bool beforeHasPersistentTag = hasPersistentTag;
            const bool afterHasPersistentTag = persistentTagToggle;

            if (afterHasPersistentTag && !beforeHasPersistentTag)
            {
                context.AddPersistentTag(e);
            }
            else if (!afterHasPersistentTag && beforeHasPersistentTag)
            {
                context.RemovePersistentTag(e);
            }

            if (undoRedoPtr && beforeHasPersistentTag != afterHasPersistentTag)
            {
                undoRedoPtr->Push_PersistentTagToggle(
                    e,
                    beforeHasPersistentTag,
                    afterHasPersistentTag
                );
            }
        }
        // -------------------------------------------------
        // LightComponent toggle
        // -------------------------------------------------
        bool lightToggle = hasLight;
        if (ImGui::Checkbox("LightComponent##has", &lightToggle))
        {
            LightSnapshot beforeLight{};
            beforeLight = hasLight ? CaptureLightSnapshot(context, e) : LightSnapshot{};
            beforeLight.entity = e;
            beforeLight.hasLight = hasLight;

            if (lightToggle && !hasLight)
            {
                context.AddLightComponent(e);
            }
            else if (!lightToggle && hasLight)
            {
                context.RemoveLightComponent(e);
            }

            LightSnapshot afterLight = CaptureLightSnapshot(context, e);
            afterLight.entity = e;
            afterLight.hasLight = (context.TryGetLight(e) != nullptr);

            if (undoRedoPtr)
            {
                undoRedoPtr->Push_LightToggle(e, beforeLight, afterLight);
            }
        }

        // -------------------------------------------------
        // EnemyController toggle
        // -------------------------------------------------
        bool enemyControllerToggle = hasEnemyController;
        if (ImGui::Checkbox("EnemyController##has", &enemyControllerToggle))
        {
            const bool beforeHasEnemyController = hasEnemyController;

            if (enemyControllerToggle && !beforeHasEnemyController)
            {
                context.AddEnemyControllerComponent(e);
            }
            else if (!enemyControllerToggle && beforeHasEnemyController)
            {
                context.RemoveEnemyControllerComponent(e);
            }

            const bool afterHasEnemyController =
                (context.TryGetEnemyController(e) != nullptr);

            if (undoRedoPtr && beforeHasEnemyController != afterHasEnemyController)
            {
                undoRedoPtr->Push_EnemyControllerToggle(
                    e,
                    beforeHasEnemyController,
                    afterHasEnemyController
                );
            }
        }
    }


    ImGui::Separator();

    // --- MeshRenderer ---
    if (MeshRenderer* mr = context.GetRenderer(e))
    {
        ImGui::SeparatorText("MeshRenderer");
        Vector3 oldColor = mr->GetColor();
        unsigned oldTexture = static_cast<unsigned>(mr->GetTexture());
        Mesh2D* oldMesh = mr->GetMesh();

        Vector3 col = oldColor;

        if (ImGui::ColorEdit3("Color", &col.x))
        {
            mr->SetColor(col);

            if (undoRedoPtr)
            {
                undoRedoPtr->Push_MeshRendererEdit(
                    e,
                    oldColor, col,
                    oldTexture, oldTexture,
                    oldMesh, oldMesh
                );
            }
        }


        GLuint texID = mr->GetTexture();
        ImGui::Text("Texture ID: %u", texID);

        Mesh2D* mesh = mr->GetMesh();
        ImGui::Text("Mesh: %p", (void*)mesh);
    }

    // --- Collider ---
    if (Collider* c = context.GetCollider(e))
    {
        ImGui::SeparatorText("Collider");

        const bool allowEdit = (!isPlaying) || isPaused;

        Vector2 oldSize = c->size;
        bool oldTrigger = c->isTrigger;

        Vector2 size = oldSize;
        bool trigger = oldTrigger;

        bool changed = false;

        if (ImGui::DragFloat2("Size", &size.x, 1.0f, 1.f, 512.f))
            changed = true;

        if (ImGui::Checkbox("Is Trigger", &trigger))
            changed = true;

        if (changed)
        {
            c->size = size;
            c->isTrigger = trigger;

            if (allowEdit && undoRedoPtr)
            {
                const bool actuallyChanged =
                    (oldSize.x != size.x) ||
                    (oldSize.y != size.y) ||
                    (oldTrigger != trigger);

                if (actuallyChanged)
                {
                    undoRedoPtr->Push_ColliderUpdate(
                        e,
                        oldSize, size,
                        oldTrigger, trigger
                    );
                }
            }
        }
    }

    // --- SpriteAnimator ---
    if (SpriteAnimator* a = context.GetAnimator(e))
    {
        int oldCur = a->cur;
        int oldStart = a->startFrame;
        int oldEnd = a->endFrame;
        float oldSpeed = a->speed;
        const SpriteSheet* oldSheet = a->sheet;

        int cur = oldCur;
        int start = oldStart;
        int end = oldEnd;
        float speed = oldSpeed;

        bool changed = false;
        changed |= ImGui::DragInt("Current Frame", &cur, 1, start, end);
        changed |= ImGui::DragInt("Start Frame", &start, 1, 0, 1000);
        changed |= ImGui::DragInt("End Frame", &end, 1, 0, 1000);
        changed |= ImGui::DragFloat("Speed (FPS)", &speed, 0.1f, 0.0f, 60.0f);

        if (changed)
        {
            a->cur = cur;
            a->startFrame = start;
            a->endFrame = end;
            a->speed = speed;

            if (undoRedoPtr)
            {
                const bool actuallyChanged =
                    (oldCur != cur) ||
                    (oldStart != start) ||
                    (oldEnd != end) ||
                    (oldSpeed != speed);

                if (actuallyChanged)
                {
                    undoRedoPtr->Push_SpriteAnimatorEdit(
                        e,
                        oldCur, cur,
                        oldStart, start,
                        oldEnd, end,
                        oldSpeed, speed,
                        oldSheet, oldSheet
                    );
                }
            }
        }

        if (a->sheet)
        {
            ImGui::Text("Sheet: %dx%d frames", a->sheet->cols, a->sheet->rows);
            ImGui::Text("Frame Size: %dx%d", a->sheet->frameW, a->sheet->frameH);
        }
        else
        {
            ImGui::TextDisabled("No sprite sheet assigned");
        }
    }

    // --- ParticleEmitter ---
    if (ParticleEmitter* em = context.GetEmitter(e))
    {
        ImGui::SeparatorText("ParticleEmitter");

        bool oldEnabled = em->enabled;
        float oldRate = em->rate;
        float oldParticleLife = em->particleLife;
        Vector2 oldOffset = em->offset;
        Vector2 oldVelMin = em->velMin;
        Vector2 oldVelMax = em->velMax;
        Vector3 oldColorStart = em->colorStart;
        Vector3 oldColorEnd = em->colorEnd;
        float oldSizeStart = em->sizeStart;
        float oldSizeEnd = em->sizeEnd;
        Mesh2D* oldQuad = em->quad;
        unsigned oldTexture = static_cast<unsigned>(em->texture);
        float oldTimeAccumulator = em->timeAccumulator;

        bool enabled = oldEnabled;
        float rate = oldRate;
        float particleLife = oldParticleLife;
        Vector2 offset = oldOffset;
        Vector2 velMin = oldVelMin;
        Vector2 velMax = oldVelMax;
        Vector3 colorStart = oldColorStart;
        Vector3 colorEnd = oldColorEnd;
        float sizeStart = oldSizeStart;
        float sizeEnd = oldSizeEnd;

        bool changed = false;

        changed |= ImGui::Checkbox("Emitter Enabled", &enabled);
        changed |= ImGui::DragFloat("Rate", &rate, 0.1f, 0.0f, 1000.0f);
        changed |= ImGui::DragFloat("Particle Life", &particleLife, 0.01f, 0.01f, 20.0f);

        changed |= ImGui::DragFloat2("Offset", &offset.x, 0.1f);
        changed |= ImGui::DragFloat2("Velocity Min", &velMin.x, 0.1f);
        changed |= ImGui::DragFloat2("Velocity Max", &velMax.x, 0.1f);

        changed |= ImGui::ColorEdit3("Start Color", &colorStart.x);
        changed |= ImGui::ColorEdit3("End Color", &colorEnd.x);

        changed |= ImGui::DragFloat("Start Size", &sizeStart, 0.1f, 0.0f, 1000.0f);
        changed |= ImGui::DragFloat("End Size", &sizeEnd, 0.1f, 0.0f, 1000.0f);

        if (changed)
        {
            em->enabled = enabled;
            em->rate = rate;
            em->particleLife = particleLife;
            em->offset = offset;
            em->velMin = velMin;
            em->velMax = velMax;
            em->colorStart = colorStart;
            em->colorEnd = colorEnd;
            em->sizeStart = sizeStart;
            em->sizeEnd = sizeEnd;

            if (undoRedoPtr)
            {
                const bool actuallyChanged =
                    (oldEnabled != enabled) ||
                    (oldRate != rate) ||
                    (oldParticleLife != particleLife) ||
                    (oldOffset.x != offset.x) || (oldOffset.y != offset.y) ||
                    (oldVelMin.x != velMin.x) || (oldVelMin.y != velMin.y) ||
                    (oldVelMax.x != velMax.x) || (oldVelMax.y != velMax.y) ||
                    (oldColorStart.x != colorStart.x) || (oldColorStart.y != colorStart.y) || (oldColorStart.z != colorStart.z) ||
                    (oldColorEnd.x != colorEnd.x) || (oldColorEnd.y != colorEnd.y) || (oldColorEnd.z != colorEnd.z) ||
                    (oldSizeStart != sizeStart) ||
                    (oldSizeEnd != sizeEnd);

                if (actuallyChanged)
                {
                    undoRedoPtr->Push_ParticleEmitterEdit(
                        e,
                        oldEnabled, enabled,
                        oldRate, rate,
                        oldParticleLife, particleLife,
                        oldOffset, offset,
                        oldVelMin, velMin,
                        oldVelMax, velMax,
                        oldColorStart, colorStart,
                        oldColorEnd, colorEnd,
                        oldSizeStart, sizeStart,
                        oldSizeEnd, sizeEnd,
                        oldQuad, oldQuad,
                        oldTexture, oldTexture,
                        oldTimeAccumulator, oldTimeAccumulator
                    );
                }
            }
        }

        ImGui::Text("Texture ID: %u", (unsigned)em->texture);
        ImGui::Text("Quad: %p", (void*)em->quad);
    }

    // --- LightComponent ---
    if (LightComponent* l = context.GetLight(e))
    {
        ImGui::SeparatorText("LightComponent");

        LightSnapshot beforeLight = CaptureLightSnapshot(context, e);

        bool changed = false;

        changed |= ImGui::Checkbox("Light Enabled", &l->enabled);

        changed |= ImGui::Checkbox("Glow Enabled", &l->glow.enabled);
        changed |= ImGui::DragFloat("Glow Radius", &l->glow.radius, 1.0f, 0.0f, 2000.0f);
        changed |= ImGui::ColorEdit3("Glow Color", &l->glow.color.x);
        changed |= ImGui::DragFloat2("Glow Offset", &l->glow.offset.x, 0.1f);
        changed |= ImGui::DragFloat("Glow Intensity", &l->glow.intensity, 0.01f, 0.0f, 20.0f);
        changed |= ImGui::DragFloat("Glow Opacity", &l->glow.opacity, 0.01f, 0.0f, 1.0f);
        changed |= ImGui::DragFloat("Glow Softness", &l->glow.softness, 0.01f, 0.0f, 20.0f);

        changed |= ImGui::Checkbox("Source Enabled", &l->source.enabled);
        changed |= ImGui::DragFloat("Source Radius", &l->source.radius, 1.0f, 0.0f, 2000.0f);
        changed |= ImGui::ColorEdit3("Source Color", &l->source.color.x);
        changed |= ImGui::DragFloat2("Source Offset", &l->source.offset.x, 0.1f);
        changed |= ImGui::DragFloat("Source Intensity", &l->source.intensity, 0.01f, 0.0f, 20.0f);
        changed |= ImGui::DragFloat("Source Opacity", &l->source.opacity, 0.01f, 0.0f, 1.0f);
        changed |= ImGui::DragFloat("Source Attenuation", &l->source.attenuation, 0.01f, 0.0f, 20.0f);

        changed |= ImGui::Checkbox("Ember Enabled", &l->ember.enabled);
        changed |= ImGui::DragFloat("Ember Rate", &l->ember.rate, 0.1f, 0.0f, 1000.0f);
        changed |= ImGui::DragFloat("Ember Particle Life", &l->ember.particleLife, 0.01f, 0.01f, 20.0f);
        changed |= ImGui::DragFloat2("Ember Vel Min", &l->ember.velMin.x, 0.1f);
        changed |= ImGui::DragFloat2("Ember Vel Max", &l->ember.velMax.x, 0.1f);
        changed |= ImGui::ColorEdit3("Ember Start Color", &l->ember.colorStart.x);
        changed |= ImGui::ColorEdit3("Ember End Color", &l->ember.colorEnd.x);
        changed |= ImGui::DragFloat("Ember Start Size", &l->ember.sizeStart, 0.1f, 0.0f, 1000.0f);
        changed |= ImGui::DragFloat("Ember End Size", &l->ember.sizeEnd, 0.1f, 0.0f, 1000.0f);
        changed |= ImGui::Checkbox("Ember Additive", &l->ember.additive);
        changed |= ImGui::DragFloat2("Ember Offset", &l->ember.offset.x, 0.1f);

        changed |= ImGui::Checkbox("Flicker Enabled", &l->flicker.enabled);
        changed |= ImGui::DragFloat("Flicker Intensity Min", &l->flicker.intensityMin, 0.01f, 0.0f, 10.0f);
        changed |= ImGui::DragFloat("Flicker Intensity Max", &l->flicker.intensityMax, 0.01f, 0.0f, 10.0f);
        changed |= ImGui::DragFloat("Flicker Speed", &l->flicker.speed, 0.01f, 0.0f, 50.0f);

        if (changed && undoRedoPtr)
        {
            LightSnapshot afterLight = CaptureLightSnapshot(context, e);
            undoRedoPtr->Push_LightEdit(e, beforeLight, afterLight);
        }
    }


    // --- Logic / Script (game-defined via IComponentContext) ---
    {
        ImGui::SeparatorText("Logic / Script");

        // Ask context what scripts exist in this game
        const auto& scriptNames = context.GetScriptNameList();
        if (scriptNames.empty()) {
            ImGui::TextDisabled("No scripts registered for this game.");
        }
        else {
            // Current script assigned to this entity
            std::string currentName = context.GetEntityScriptName(e);

            // Map currentName -> index in scriptNames
            int currentIndex = 0;
            if (!currentName.empty()) {
                for (int i = 0; i < (int)scriptNames.size(); ++i) {
                    if (scriptNames[i] == currentName) {
                        currentIndex = i;
                        break;
                    }
                }
            }

            // Build a char* array for ImGui::Combo
            std::vector<const char*> labels;
            labels.reserve(scriptNames.size());
            for (const auto& s : scriptNames)
                labels.push_back(s.c_str());

            if (ImGui::Combo("Script Behavior",
                &currentIndex,
                labels.data(),
                (int)labels.size()))
            {
                // User picked a new script – tell the game to update ScriptComponent
                const std::string beforeName = currentName;
                const bool beforeHasScript =
                    !beforeName.empty() && beforeName != "None";

                const std::string afterName = scriptNames[currentIndex];
                const bool afterHasScript =
                    !afterName.empty() && afterName != "None";

                // Apply the change
                context.SetEntityScriptName(e, afterName);

                // record it for undo/redo
                if (undoRedoPtr &&
                    (beforeHasScript != afterHasScript || beforeName != afterName))
                {
                    undoRedoPtr->Push_ScriptEdit(
                        e,
                        beforeHasScript,
                        beforeName,
                        afterHasScript,
                        afterName
                    );
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    // Prefab tools (only visible if this entity came from a prefab)
    // ---------------------------------------------------------------------
    if (isPrefabInstance && isPrefabInstance(e))
    {
        ImGui::SeparatorText("Prefab");

        if (ImGui::Button("Revert to Prefab"))
        {
            if (onPrefabRevertInstance)
                onPrefabRevertInstance(e);
        }

        ImGui::SameLine();

        if (ImGui::Button("Apply Instance to Prefab"))
        {
            if (onPrefabApplyFromInstance)
                onPrefabApplyFromInstance(e);
        }

        if (ImGui::Button("Save Prefabs"))
        {
            if (onPrefabSaveAll)
                onPrefabSaveAll();
        }

        ImGui::TextWrapped(
            "Revert: reset this instance back to its original prefab values.\n"
            "Apply: use this instance as the new prefab defaults and update all "
            "non-overridden instances of this prefab in the current scene.");
    }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lighting effects"))
        {
            LightEffects::DrawLightEffectsTab(context, e, undoRedoPtr);
            if (onLightSave && ImGui::Button("Save Light Data", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                onLightSave(currentLevelPath);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

/**
    * @brief Renders and applies the ImGuizmo transform widget inside the viewport.
    *
    * Provides translate, rotate, and scale operations depending on user selection
    * (T/R/Y hotkeys). Automatically:
    * - Builds the model matrix from Transform fields
    * - Decomposes modified matrices back to Transform
    * - Handles rotation delta to avoid wrap-around errors
    * - Updates collider size when scaling
    * - Records before/after states for Undo/Redo
    *
    * @note Only active when gizmo is enabled and an entity is selected.
*/
void EditorOverlay::DrawGizmoInViewport()
{
    if (!activeCamera || !gizmoEnabled) return;
    const auto& entities = context.GetEntities();
    if (selectedEntity < 0 || selectedEntity >= (int)entities.size()) return;

    Entity e = entities[selectedEntity];
    Transform* t = context.GetTransform(e);
    if (!t) return;

    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 crMin = ImGui::GetWindowContentRegionMin();
    ImVec2 crMax = ImGui::GetWindowContentRegionMax();
    ImVec2 imagePos(
        winPos.x + crMin.x,
        winPos.y + crMin.y
    );
    ImVec2 imageSize(
        crMax.x - crMin.x,
        crMax.y - crMin.y
    );

    // Tell ImGuizmo to draw into this rect, on this window's drawlist
    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(imagePos.x, imagePos.y, imageSize.x, imageSize.y);

    Vector2 position = t->GetPosition();
    Vector2 scale = t->GetScale();
    float rot = t->GetRotation();

    const Matrix3x3& view3 = activeCamera->GetView();
    const Matrix3x3& proj3 = activeCamera->GetProjection();

    float viewM[16] = {
        view3(0,0), view3(0,1), 0, 0,
        view3(1,0), view3(1,1), 0, 0,
        0, 0, 1, 0,
        view3(2,0), view3(2,1), 0, 1
    };
    float projM[16] = {
        proj3(0,0), proj3(0,1), 0, 0,
        proj3(1,0), proj3(1,1), 0, 0,
        0, 0, 1, 0,
        proj3(2,0), proj3(2,1), 0, 1
    };

    // build model from Transform (rot in radians)
    const float DEG2RAD = 3.14159265358979323846f / 180.0f;
    float rr = rot * DEG2RAD;
    float c = cosf(rr), s = sinf(rr);
    float modelM[16] = {
        scale.x * c, -scale.y * s, 0, 0,
        scale.x * s,  scale.y * c, 0, 0,
        0, 0, 1, 0,
        position.x, position.y, 0, 1
    };

    //static ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
    if (gizmoEnabled) {
        if (ImGui::IsKeyPressed(ImGuiKey_T)) gizmoOp = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoOp = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) gizmoOp = ImGuizmo::SCALE;
    }

    // translate in WORLD; rotate/scale in LOCAL
    ImGuizmo::MODE mode = (gizmoOp == ImGuizmo::TRANSLATE) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

    // only capture delta when rotating (prevents ±180° wrap jumps)
    float deltaM[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    float* pDelta = (gizmoOp == ImGuizmo::ROTATE) ? deltaM : nullptr;

    bool now = ImGuizmo::IsUsing();

    // BEGIN — once
    if (now && !gizmoActive)
    {
        gizmoActive = true;
        gizmoEntity = e;

        if (!isPlaying && onRecordBeforeTransform)
            onRecordBeforeTransform(e);
    }

    // manipulate call
    ImGuizmo::Manipulate(viewM, projM, gizmoOp, mode, modelM, pDelta);

    const bool usingNow = ImGuizmo::IsUsing();
    const bool allowEdit = (!isPlaying) || isPaused;

    // BEGIN — once
    if (usingNow && !gizmoActive)
    {
        gizmoActive = true;
        gizmoEntity = e;

        if (allowEdit && onRecordBeforeTransform)
            onRecordBeforeTransform(e);
    }

    // END — once
    if (!usingNow && gizmoActive)
    {
        if (allowEdit && onRecordAfterTransform)
            onRecordAfterTransform(gizmoEntity);

        gizmoActive = false;
    }

    if (!usingNow) return;

    // For translate/scale, writeback from modelM (works well)
    // For rotate, add delta angle to current rotation and normalize
    float pos[3], rotD[3], scl[3];
    ImGuizmo::DecomposeMatrixToComponents(modelM, pos, rotD, scl);

    switch (gizmoOp)
    {
    case ImGuizmo::TRANSLATE:
        t->SetPosition({ pos[0], pos[1] });
        break;

    case ImGuizmo::SCALE:
        t->SetScale({ scl[0], scl[1] });
        if (Collider* ccol = context.GetCollider(e))
            ccol->size = { scl[0], scl[1] };
        break;

    case ImGuizmo::ROTATE:
    {
        // use delta angle to avoid wrap-induced 360s
        float dpos[3], drot[3], dscl[3];
        ImGuizmo::DecomposeMatrixToComponents(deltaM, dpos, drot, dscl);

        auto norm = [](float a)->float {
            while (a > 180.f) a -= 360.f;
            while (a < -180.f) a += 360.f;
            return a;
            };

        float newRot = norm(t->GetRotation() + drot[2]); // deg
        t->SetRotation(newRot);
        break;
    }
    default: break;
    }
}

/**
    * @brief Draws the Gizmo Mode panel.
    *
    * Allows toggling gizmo mode and choosing the current operation:
    * - Translate
    * - Rotate
    * - Scale
    *
    * Also handles T/R/Y hotkeys, and controls camera lock/unlock behavior
    * depending on whether gizmos or navigation mode is active.
*/
void EditorOverlay::DrawGizmoBar(){
    ImGui::Begin("Gizmo Mode", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::BeginDisabled(isPlaying && !isPaused);

    // When gizmo turns on, force nav off and unlock follow-player
    // When gizmo turns off, nothing special (nav can be toggled separately).
    //bool prev = gizmoEnabled;
    if (ImGui::Checkbox("Enable Gizmo", &gizmoEnabled)) {
        if (gizmoEnabled) {
            navMode = false;
            followSelection = false;
            followEntity = 0;
            if (onRequestCameraLock) onRequestCameraLock(false);
        }
        else {
            // Turning gizmo OFF: give control back to default (player-follow) camera
            followSelection = false;
            followEntity = 0;

            // Only relock to player if we're not in nav mode
            if (!navMode && onRequestCameraLock)
                onRequestCameraLock(true);  // tell GameApp to follow player again
        }
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(T=Translate, R=Rotate, Y=Scale)");

    // Disable op buttons when gizmo is OFF
    ImGui::BeginDisabled(!gizmoEnabled);
    {
        // operation buttons (radio style)
        bool tSel = (gizmoOp == ImGuizmo::TRANSLATE);
        bool rSel = (gizmoOp == ImGuizmo::ROTATE);
        bool sSel = (gizmoOp == ImGuizmo::SCALE);

        if (ImGui::RadioButton("Translate", tSel)) gizmoOp = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", rSel))    gizmoOp = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", sSel))     gizmoOp = ImGuizmo::SCALE;

        // hot key for gizmo when it enabled 
        if (gizmoEnabled) {
            if (ImGui::IsKeyPressed(ImGuiKey_T)) gizmoOp = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoOp = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_Y)) gizmoOp = ImGuizmo::SCALE;
        }
    }
    ImGui::EndDisabled(); // inner 
    ImGui::EndDisabled(); // outer (isPlaying)

    ImGui::End();
}

/**
    * @brief Provides UI for editing tile-based labyrinth maps.
    *
    * Features:
    * - Enable/disable tile edit mode
    * - Load and store tile grids per level
    * - Paint tiles using brush types (wall, crate, empty)
    * - Prevent modification of locked cells (room blocks '#')
    * - Apply & reload level through callback
    *
    * @note Tile data is cached per-level so switching scenes preserves edits.
*/
void EditorOverlay::DrawTilemapEditor() {
    if (!glfwGetCurrentContext()) return;

    if (!ImGui::Begin("Tilemap Editor", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    // --- mode toggle (enter / exit tile editing) ---
    ImGui::Checkbox("Enable Tile Editing", &tileEditMode);

    // We use currentLevelPath as the key for per-level tile storage
    auto makeLevelKey = [&]() -> std::string {
        return currentLevelPath.empty() ? std::string("<default>") : currentLevelPath;
        };

    // detect edge
    if (tileEditMode && !tileEditModePrev) {
        // entering tile edit mode for *current* level
        const std::string key = makeLevelKey();

        // 1) If we already cached a grid for this level, restore it
        auto itGrid = tileGridPerLevel.find(key);
        auto itSize = tileSizePerLevel.find(key);
        if (itGrid != tileGridPerLevel.end() && itSize != tileSizePerLevel.end()) {
            tileGrid_ = itGrid->second;
            gridW_ = itSize->second.first;
            gridH_ = itSize->second.second;
        }
        // 2) Otherwise ask the game to load the TXT for this level
        else if (onTileEditEnter) {
            onTileEditEnter();  // GameApp should fill tileGrid_, gridW_, gridH_

            // Cache what we got back, if valid
            if (gridW_ > 0 && gridH_ > 0 && !tileGrid_.empty()) {
                tileGridPerLevel[key] = tileGrid_;
                tileSizePerLevel[key] = { gridW_, gridH_ };
            }
        }
    }
    else if (!tileEditMode && tileEditModePrev) {
        // leaving tile edit mode: keep latest edits in cache for this level
        const std::string key = makeLevelKey();
        if (gridW_ > 0 && gridH_ > 0 && !tileGrid_.empty()) {
            tileGridPerLevel[key] = tileGrid_;
            tileSizePerLevel[key] = { gridW_, gridH_ };
        }
        if (onTileEditExit) onTileEditExit();
    }
    tileEditModePrev = tileEditMode;

    ImGui::Separator();

    if (!tileEditMode) {
        ImGui::TextDisabled("Tile editing disabled. Enable it to edit the labyrinth.");
        ImGui::End();
        return;
    }

    // --- choose brush type ---
    enum class Brush { Empty, Wall, Crate, RoomBlock};
    static Brush brush = Brush::Wall;

    ImGui::Text("Brush:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Wall", brush == Brush::Wall))   brush = Brush::Wall;
    ImGui::SameLine();
    if (ImGui::RadioButton("Crate", brush == Brush::Crate)) brush = Brush::Crate;
    ImGui::SameLine();
    if (ImGui::RadioButton("Empty", brush == Brush::Empty)) brush = Brush::Empty;


    ImGui::Separator();

    // --- the grid itself  ---
    const float cellSize = 18.0f;

    ImGui::BeginChild("TileGrid", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

      for (int y = 1; y < gridH_ - 1; ++y) {
        for (int x = 1; x < gridW_- 1; ++x) {
            int idx = y * gridW_ + x;
            char c = (idx < (int)tileGrid_.size()) ? tileGrid_[idx] : ' ';

            // pick color by tile type
            ImVec4 col(0.2f, 0.2f, 0.2f, 1.0f);                 // empty
            if (c == '1') col = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); // wall
            if (c == '3') col = ImVec4(0.8f, 0.7f, 0.2f, 1.0f); // crate
            if (c == '#') col = ImVec4(0.f, 0.f, 0.f, 0.f);     // roomblock

            ImGui::PushID(idx);
            ImGui::PushStyleColor(ImGuiCol_Button, col);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);

            bool locked = (c == '#'); //uneditable
            ImGui::BeginDisabled(locked);

            if (ImGui::Button("##cell", ImVec2(cellSize, cellSize))) {
                // paint
                char newChar = ' ';
                switch (brush) {
                case Brush::Empty: newChar = '0'; break;
                case Brush::Wall:  newChar = '1'; break;
                case Brush::Crate: newChar = '3'; break;
                }
                tileGrid_[idx] = newChar;
                tileDirty = true;
            }

            ImGui::EndDisabled();

            ImGui::PopStyleColor(3);
            ImGui::PopID();

            ImGui::SameLine();
        }
        ImGui::NewLine();
    }

    ImGui::EndChild();

    ImGui::Separator();

    ImGui::BeginDisabled(!tileDirty);
    if (ImGui::Button("Apply & Reload Level")) {
        // Keep latest tiles in the per-level cache
        auto makeLevelKeyX = [&]() -> std::string {
            return currentLevelPath.empty() ? std::string("<default>") : currentLevelPath;
            };
        const std::string key = makeLevelKeyX();
        if (gridW_ > 0 && gridH_ > 0 && !tileGrid_.empty()) {
            tileGridPerLevel[key] = tileGrid_;
            tileSizePerLevel[key] = { gridW_, gridH_ };
        }

        if (onTileApply) {
            onTileApply(tileGrid_, gridW_, gridH_);
        }
        tileDirty = false;
    }
    ImGui::EndDisabled();

    ImGui::End();
}

/**
    * @brief Renders the in-editor debugging console.
    *
    * Shows color-coded log entries (Info, Success, Warning, Error),
    * displays a legend, allows scrolling, and includes a clear button.
    *
    * Messages are fetched from the global DebugConsole singleton.
*/
void EditorOverlay::DrawDebugConsole()
{
    if (!visible)
        return;

    //ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);

    static ImGuiWindowClass cls;
    cls.ClassId = ImHashStr("EditorTabGroup");
    cls.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
    ImGui::SetNextWindowClass(&cls);

    // Create the Debug Console window
    if (!ImGui::Begin("Debug Console"))
    {
        ImGui::End();
        return;
    }

    {
        ImGui::BeginChild("LegendRegion", ImVec2(0, 40), false);

        const char* title = "--- Legend Information ----";
        float textWidth = ImGui::CalcTextSize(title).x;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float offset = (windowWidth - textWidth) * 0.5f;

        if (offset > 0)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", title);

        // Calculate the approximate width of the legend text
        float legendWidth = ImGui::CalcTextSize("*Info     *Success     *Warning     *Error").x;

        CenterGroup(legendWidth);

        // Draw centered legend
        ImGui::BeginGroup();

        ImGui::TextColored(ImVec4(1, 1, 1, 1), "*");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 1, 1, 1), "Info");
        ImGui::SameLine(0, 20);

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "*");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Success");
        ImGui::SameLine(0, 20);

        ImGui::TextColored(ImVec4(1, 1, 0, 1), "*");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning");
        ImGui::SameLine(0, 20);

        ImGui::TextColored(ImVec4(1, 0, 0, 1), "*");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error");
        ImGui::SameLine(0, 20);

        ImGui::EndGroup();
    }
    ImGui::EndChild();
    ImGui::Separator();

    {
        // Scrollable message region
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, ImGui::GetWindowHeight() * 0.65f), true, ImGuiWindowFlags_HorizontalScrollbar);

        const auto& messages = DebugConsole::Get().GetMessages();
        bool scrollToBottom = (ImGui::GetScrollY() >= ImGui::GetScrollMaxY());


        for (const auto& entry : messages)
        {
            // Determine color by log level
            ImVec4 color;
            switch (entry.level)
            {
            case LogLevel::Info: color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;     // White
            case LogLevel::Success: color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;    // Green
            case LogLevel::Warning: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;  // Yellow
            case LogLevel::Error: color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;    // Red 
            }

            // Draw the message
            ImGui::TextColored(color, "%s", entry.message.c_str());
        }

        // Auto-scroll to bottom
        if (scrollToBottom || DebugConsole::Get().IsScrollToBottom())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
    }
    {
        float windowVisibleX = ImGui::GetWindowContentRegionMax().x;
        float buttonWidth = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2;

        ImGui::SetCursorPosX(windowVisibleX - buttonWidth);
        //ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetStyle().FramePadding.y * 3);
        // Clear button
        if (ImGui::Button("Clear"))
            DebugConsole::Get().Clear();
    }
    ImGui::End();
}

/**
    * @brief Displays the Undo/Redo history panel.
    *
    * Shows the contents of the undo and redo stacks, allowing the user to
    * visualize modification events (entity creation, deletion, transforms).
    *
    * @note Requires a valid pointer to the UndoRedoManager.
*/
void EditorOverlay::DrawHistory()
{
    if (!undoRedoPtr)
        return;

    static ImGuiWindowClass historyClass;
    historyClass.ClassId = ImHashStr("EditorTabGroup");
    historyClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
    ImGui::SetNextWindowClass(&historyClass);

    if (!ImGui::Begin("History"))
    {
        ImGui::End();
        return;
    }

    // Get stacks
    const auto& undo = undoRedoPtr->GetUndoStack();
    const auto& redo = undoRedoPtr->GetRedoStack();

    // Helper: entity → index
    auto findIndex = [&](Entity entity) -> int
        {
            const auto& list = context.GetEntities();
            for (int i = 0; i < (int)list.size(); i++)
                if (list[i] == entity)
                    return i;
            return -1;
        };

    // --------------------------------------------------
    // UNDO SECTION
    // --------------------------------------------------
    ImGui::Text("Undo (%d)", (int)undo.size());
    ImGui::Separator();

    if (undo.empty())
    {
        ImGui::TextDisabled("Undo stack empty.");
    }
    else
    {
        int undoCounter = 1;
        for (int i = (int)undo.size() - 1; i >= 0; --i, ++undoCounter)
        {
            const UndoRecord& r = undo[i];   // <-- FIXED: use UNDO stack
            ImGui::PushID(1000 + i);

            const char* label = "";
            ImVec4 col;

            switch (r.action)
            {
            case UndoRecord::Type::TransformEdit:
                label = "[TransformEdit]";
                col = ImVec4(0.5f, 0.8f, 1.f, 1.f);
                break;
            case UndoRecord::Type::ColliderEdit:
                label = "[ColliderEdit]";
                col = ImVec4(1.f, 0.85f, 0.4f, 1.f);
                break;
            case UndoRecord::Type::ColliderToggle:
                label = "[ColliderToggle]";
                col = ImVec4(0.95f, 0.75f, 0.25f, 1.f);
                break;
            case UndoRecord::Type::EntityCreated:
                label = "[EntityCreated]";
                col = ImVec4(0.6f, 1.f, 0.6f, 1.f);
                break;
            case UndoRecord::Type::EntityDeleted:
                label = "[EntityDeleted]";
                col = ImVec4(1.f, 0.5f, 0.5f, 1.f);
                break;
            case UndoRecord::Type::MeshRendererEdit:
                label = "[MeshRendererEdit]";
                col = ImVec4(1.0f, 0.7f, 0.3f, 1.f);
                break;
            case UndoRecord::Type::MeshRendererToggle:
                label = "[MeshRendererToggle]";
                col = ImVec4(0.9f, 0.55f, 1.0f, 1.f);
                break;
            case UndoRecord::Type::SpriteAnimatorEdit:
                label = "[SpriteAnimatorEdit]";
                col = ImVec4(0.45f, 1.0f, 0.85f, 1.f);
                break;
            case UndoRecord::Type::SpriteAnimatorToggle:
                label = "[SpriteAnimatorToggle]";
                col = ImVec4(0.75f, 0.6f, 1.0f, 1.f);
                break;
            case UndoRecord::Type::ParticleEmitterEdit:
                label = "[ParticleEmitterEdit]";
                col = ImVec4(1.0f, 0.75f, 0.35f, 1.f);
                break;
            case UndoRecord::Type::ParticleEmitterToggle:
                label = "[ParticleEmitterToggle]";
                col = ImVec4(1.0f, 0.6f, 0.2f, 1.f);
                break;
            case UndoRecord::Type::ScriptEdit:
                label = "[ScriptEdit]";
                col = ImVec4(0.95f, 0.9f, 0.35f, 1.f);
                break;
            case UndoRecord::Type::PlayerControllerToggle:
                label = "[PlayerControllerToggle]";
                col = ImVec4(0.55f, 0.9f, 0.55f, 1.f);
                break;
            case UndoRecord::Type::SpeedComponentToggle:
                label = "[SpeedComponentToggle]";
                col = ImVec4(0.45f, 0.85f, 0.95f, 1.f);
                break;
            case UndoRecord::Type::MassComponentToggle:
                label = "[MassComponentToggle]";
                col = ImVec4(0.95f, 0.65f, 0.45f, 1.f);
                break;
            case UndoRecord::Type::PersistentTagToggle:
                label = "[PersistentTagToggle]";
                col = ImVec4(0.75f, 0.85f, 0.45f, 1.f);
                break;
            case UndoRecord::Type::LightEdit:
                label = "[LightEdit]";
                col = ImVec4(1.00f, 0.85f, 0.35f, 1.f);
                break;

            case UndoRecord::Type::LightToggle:
                label = "[LightToggle]";
                col = ImVec4(1.00f, 0.80f, 0.20f, 1.f);
                break;
            case UndoRecord::Type::EnemyControllerToggle:
                label = "[EnemyControllerToggle]";
                col = ImVec4(0.85f, 0.45f, 0.45f, 1.f);
                break;
            default:
                label = "[Unknown]";
                col = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
                break;

            }
            ImGui::PushStyleColor(ImGuiCol_Text, col);

            std::string header =
                std::to_string(undoCounter) + ") " + label + "##undo_" + std::to_string(i);

            bool open = ImGui::TreeNode(header.c_str());

            if (ImGui::IsItemClicked())
            {
                Entity e = INVALID_ENTITY;
                switch (r.action)
                {
                case UndoRecord::Type::TransformEdit: e = r.before.entity; break;
                case UndoRecord::Type::ColliderEdit:
                case UndoRecord::Type::ColliderToggle: e = r.beforeCollider.entity; break;
                case UndoRecord::Type::EntityCreated:
                case UndoRecord::Type::EntityDeleted: e = r.entityInfo.entity; break;
                case UndoRecord::Type::MeshRendererEdit:
                case UndoRecord::Type::MeshRendererToggle: e = r.beforeRenderer.entity; break;
                case UndoRecord::Type::SpriteAnimatorEdit:
                case UndoRecord::Type::SpriteAnimatorToggle: e = r.beforeAnimator.entity; break;
                case UndoRecord::Type::ParticleEmitterEdit:
                case UndoRecord::Type::ParticleEmitterToggle: e = r.beforeEmitter.entity; break;
                case UndoRecord::Type::ScriptEdit: e = r.beforeScript.entity; break;
                case UndoRecord::Type::PlayerControllerToggle: e = r.beforeController.entity; break;
                case UndoRecord::Type::SpeedComponentToggle: e = r.beforeSpeed.entity; break;
                case UndoRecord::Type::MassComponentToggle: e = r.beforeMass.entity; break;
                case UndoRecord::Type::PersistentTagToggle: e = r.beforePersistentTag.entity; break;
                case UndoRecord::Type::LightEdit:
                case UndoRecord::Type::LightToggle: e = r.beforeLight.entity; break;
                case UndoRecord::Type::EnemyControllerToggle: e = r.beforeEnemyController.entity; break;
                default: break;
                }

                int idx = findIndex(e);
                if (idx != -1)
                {
                    selectedEntity = idx;
                    focusPropsNextFrame = true;
                    followSelection = true;
                    followEntity = e;
                }
            }

            if (open)
            {
                Entity e = INVALID_ENTITY;
                switch (r.action)
                {
                case UndoRecord::Type::TransformEdit: e = r.before.entity; break;
                case UndoRecord::Type::ColliderEdit:
                case UndoRecord::Type::ColliderToggle: e = r.beforeCollider.entity; break;
                case UndoRecord::Type::EntityCreated:
                case UndoRecord::Type::EntityDeleted: e = r.entityInfo.entity; break;
                case UndoRecord::Type::MeshRendererEdit:
                case UndoRecord::Type::MeshRendererToggle: e = r.beforeRenderer.entity; break;
                case UndoRecord::Type::SpriteAnimatorEdit:
                case UndoRecord::Type::SpriteAnimatorToggle: e = r.beforeAnimator.entity; break;
                case UndoRecord::Type::ParticleEmitterEdit:
                case UndoRecord::Type::ParticleEmitterToggle: e = r.beforeEmitter.entity; break;
                case UndoRecord::Type::ScriptEdit: e = r.beforeScript.entity; break;
                case UndoRecord::Type::PlayerControllerToggle: e = r.beforeController.entity; break;
                case UndoRecord::Type::SpeedComponentToggle: e = r.beforeSpeed.entity; break;
                case UndoRecord::Type::MassComponentToggle: e = r.beforeMass.entity; break;
                case UndoRecord::Type::PersistentTagToggle: e = r.beforePersistentTag.entity; break;
                case UndoRecord::Type::LightEdit:
                case UndoRecord::Type::LightToggle: e = r.beforeLight.entity; break;
                case UndoRecord::Type::EnemyControllerToggle: e = r.beforeEnemyController.entity; break;
                default: break;
                }

                ImGui::Text("Entity: %u", (unsigned)e);

                if (r.action == UndoRecord::Type::TransformEdit)
                {
                    ImGui::SeparatorText("Before");
                    ImGui::Text("Pos:   %.1f, %.1f", r.before.position.x, r.before.position.y);
                    ImGui::Text("Scale: %.1f, %.1f", r.before.scale.x, r.before.scale.y);
                    ImGui::Text("Rot:   %.1f", r.before.rotation);

                    ImGui::SeparatorText("After");
                    ImGui::Text("Pos:   %.1f, %.1f", r.after.position.x, r.after.position.y);
                    ImGui::Text("Scale: %.1f, %.1f", r.after.scale.x, r.after.scale.y);
                    ImGui::Text("Rot:   %.1f", r.after.rotation);
                }
                else if (r.action == UndoRecord::Type::ColliderEdit ||
                    r.action == UndoRecord::Type::ColliderToggle)
                {
                    ImGui::SeparatorText("Before");
                    ImGui::Text("Has Collider: %s", r.beforeCollider.hasCollider ? "true" : "false");
                    ImGui::Text("Size:      %.1f, %.1f", r.beforeCollider.size.x, r.beforeCollider.size.y);
                    ImGui::Text("IsTrigger: %s", r.beforeCollider.isTrigger ? "true" : "false");

                    ImGui::SeparatorText("After");
                    ImGui::Text("Has Collider: %s", r.afterCollider.hasCollider ? "true" : "false");
                    ImGui::Text("Size:      %.1f, %.1f", r.afterCollider.size.x, r.afterCollider.size.y);
                    ImGui::Text("IsTrigger: %s", r.afterCollider.isTrigger ? "true" : "false");
                }
                else if (r.action == UndoRecord::Type::MeshRendererEdit ||
                    r.action == UndoRecord::Type::MeshRendererToggle)
                {
                    ImGui::SeparatorText("Before");
                    ImGui::Text("Has Renderer: %s", r.beforeRenderer.hasRenderer ? "true" : "false");
                    ImGui::Text("Color: %.2f, %.2f, %.2f",
                        r.beforeRenderer.color.x,
                        r.beforeRenderer.color.y,
                        r.beforeRenderer.color.z);
                    ImGui::Text("Texture ID: %u", r.beforeRenderer.texture);
                    ImGui::Text("Mesh: %p", (void*)r.beforeRenderer.mesh);

                    ImGui::SeparatorText("After");
                    ImGui::Text("Has Renderer: %s", r.afterRenderer.hasRenderer ? "true" : "false");
                    ImGui::Text("Color: %.2f, %.2f, %.2f",
                        r.afterRenderer.color.x,
                        r.afterRenderer.color.y,
                        r.afterRenderer.color.z);
                    ImGui::Text("Texture ID: %u", r.afterRenderer.texture);
                    ImGui::Text("Mesh: %p", (void*)r.afterRenderer.mesh);
                }
                else if (r.action == UndoRecord::Type::SpriteAnimatorEdit ||
                    r.action == UndoRecord::Type::SpriteAnimatorToggle)
                {
                    ImGui::SeparatorText("Before");
                    ImGui::Text("Has Animator: %s", r.beforeAnimator.hasAnimator ? "true" : "false");
                    ImGui::Text("Current Frame: %d", r.beforeAnimator.cur);
                    ImGui::Text("Start Frame: %d", r.beforeAnimator.startFrame);
                    ImGui::Text("End Frame: %d", r.beforeAnimator.endFrame);
                    ImGui::Text("Speed: %.2f", r.beforeAnimator.speed);
                    ImGui::Text("Sheet: %p", (const void*)r.beforeAnimator.sheet);

                    ImGui::SeparatorText("After");
                    ImGui::Text("Has Animator: %s", r.afterAnimator.hasAnimator ? "true" : "false");
                    ImGui::Text("Current Frame: %d", r.afterAnimator.cur);
                    ImGui::Text("Start Frame: %d", r.afterAnimator.startFrame);
                    ImGui::Text("End Frame: %d", r.afterAnimator.endFrame);
                    ImGui::Text("Speed: %.2f", r.afterAnimator.speed);
                    ImGui::Text("Sheet: %p", (const void*)r.afterAnimator.sheet);
                }
                else if (r.action == UndoRecord::Type::ParticleEmitterEdit ||
                    r.action == UndoRecord::Type::ParticleEmitterToggle)
                {
                    ImGui::SeparatorText("Before");
                    ImGui::Text("Has Emitter: %s", r.beforeEmitter.hasEmitter ? "true" : "false");
                    ImGui::Text("Enabled: %s", r.beforeEmitter.enabled ? "true" : "false");
                    ImGui::Text("Rate: %.2f", r.beforeEmitter.rate);
                    ImGui::Text("Life: %.2f", r.beforeEmitter.particleLife);
                    ImGui::Text("Offset: %.1f, %.1f", r.beforeEmitter.offset.x, r.beforeEmitter.offset.y);
                    ImGui::Text("VelMin: %.1f, %.1f", r.beforeEmitter.velMin.x, r.beforeEmitter.velMin.y);
                    ImGui::Text("VelMax: %.1f, %.1f", r.beforeEmitter.velMax.x, r.beforeEmitter.velMax.y);
                    ImGui::Text("ColorStart: %.2f, %.2f, %.2f",
                        r.beforeEmitter.colorStart.x,
                        r.beforeEmitter.colorStart.y,
                        r.beforeEmitter.colorStart.z);
                    ImGui::Text("ColorEnd: %.2f, %.2f, %.2f",
                        r.beforeEmitter.colorEnd.x,
                        r.beforeEmitter.colorEnd.y,
                        r.beforeEmitter.colorEnd.z);
                    ImGui::Text("SizeStart: %.2f", r.beforeEmitter.sizeStart);
                    ImGui::Text("SizeEnd: %.2f", r.beforeEmitter.sizeEnd);
                    ImGui::Text("Texture ID: %u", r.beforeEmitter.texture);
                    ImGui::Text("Quad: %p", (void*)r.beforeEmitter.quad);

                    ImGui::SeparatorText("After");
                    ImGui::Text("Has Emitter: %s", r.afterEmitter.hasEmitter ? "true" : "false");
                    ImGui::Text("Enabled: %s", r.afterEmitter.enabled ? "true" : "false");
                    ImGui::Text("Rate: %.2f", r.afterEmitter.rate);
                    ImGui::Text("Life: %.2f", r.afterEmitter.particleLife);
                    ImGui::Text("Offset: %.1f, %.1f", r.afterEmitter.offset.x, r.afterEmitter.offset.y);
                    ImGui::Text("VelMin: %.1f, %.1f", r.afterEmitter.velMin.x, r.afterEmitter.velMin.y);
                    ImGui::Text("VelMax: %.1f, %.1f", r.afterEmitter.velMax.x, r.afterEmitter.velMax.y);
                    ImGui::Text("ColorStart: %.2f, %.2f, %.2f",
                        r.afterEmitter.colorStart.x,
                        r.afterEmitter.colorStart.y,
                        r.afterEmitter.colorStart.z);
                    ImGui::Text("ColorEnd: %.2f, %.2f, %.2f",
                        r.afterEmitter.colorEnd.x,
                        r.afterEmitter.colorEnd.y,
                        r.afterEmitter.colorEnd.z);
                    ImGui::Text("SizeStart: %.2f", r.afterEmitter.sizeStart);
                    ImGui::Text("SizeEnd: %.2f", r.afterEmitter.sizeEnd);
                    ImGui::Text("Texture ID: %u", r.afterEmitter.texture);
                    ImGui::Text("Quad: %p", (void*)r.afterEmitter.quad);
                }
                else if (r.action == UndoRecord::Type::ScriptEdit)
                {
                    const auto& b = r.beforeScript;
                    const auto& a = r.afterScript;

                    const std::string beforeText =
                        (!b.hasScript || b.scriptName.empty()) ? "None" : b.scriptName;

                    const std::string afterText =
                        (!a.hasScript || a.scriptName.empty()) ? "None" : a.scriptName;

                    ImGui::Text("Before Script: %s", beforeText.c_str());
                    ImGui::Text("After Script:  %s", afterText.c_str());
                }
                else if (r.action == UndoRecord::Type::PlayerControllerToggle)
                {
                    ImGui::SeparatorText("PlayerController");
                    ImGui::Text("Before: %s", r.beforeController.hasController ? "Present" : "Absent");
                    ImGui::Text("After:  %s", r.afterController.hasController ? "Present" : "Absent");
                }
                else if (r.action == UndoRecord::Type::SpeedComponentToggle)
                {
                    ImGui::SeparatorText("SpeedComponent");
                    ImGui::Text("Before: %s", r.beforeSpeed.hasSpeed ? "Present" : "Absent");
                    ImGui::Text("After:  %s", r.afterSpeed.hasSpeed ? "Present" : "Absent");
                }
                else if (r.action == UndoRecord::Type::MassComponentToggle)
                {
                    ImGui::SeparatorText("MassComponent");
                    ImGui::Text("Before: %s", r.beforeMass.hasMass ? "Present" : "Absent");
                    ImGui::Text("After:  %s", r.afterMass.hasMass ? "Present" : "Absent");
                }
                else if (r.action == UndoRecord::Type::PersistentTagToggle)
                {
                    ImGui::SeparatorText("PersistentTag");
                    ImGui::Text("Before: %s", r.beforePersistentTag.hasPersistentTag ? "Present" : "Absent");
                    ImGui::Text("After:  %s", r.afterPersistentTag.hasPersistentTag ? "Present" : "Absent");
                }
                else if (r.action == UndoRecord::Type::LightEdit ||
                    r.action == UndoRecord::Type::LightToggle)
                {
                    ImGui::SeparatorText("LightComponent");
                    ImGui::Text("Before: %s", r.beforeLight.hasLight ? "Present" : "Absent");
                    ImGui::Text("After:  %s", r.afterLight.hasLight ? "Present" : "Absent");

                    if (r.beforeLight.hasLight || r.afterLight.hasLight)
                    {
                        ImGui::Text("Glow Radius: %.2f -> %.2f",
                            r.beforeLight.glowRadius, r.afterLight.glowRadius);
                        ImGui::Text("Glow Intensity: %.2f -> %.2f",
                            r.beforeLight.glowIntensity, r.afterLight.glowIntensity);
                        ImGui::Text("Source Radius: %.2f -> %.2f",
                            r.beforeLight.sourceRadius, r.afterLight.sourceRadius);
                    }
                }
                else if (r.action == UndoRecord::Type::EnemyControllerToggle)
                {
                    ImGui::SeparatorText("EnemyController");
                    ImGui::Text("Before: %s",
                        r.beforeEnemyController.hasEnemyController ? "Present" : "Absent");
                    ImGui::Text("After:  %s",
                        r.afterEnemyController.hasEnemyController ? "Present" : "Absent");
                }
                else
                {
                    ImGui::SeparatorText("Transform Snapshot");
                    ImGui::Text("Pos:   %.1f, %.1f", r.entityInfo.transform.position.x, r.entityInfo.transform.position.y);
                    ImGui::Text("Scale: %.1f, %.1f", r.entityInfo.transform.scale.x, r.entityInfo.transform.scale.y);
                    ImGui::Text("Rot:   %.1f", r.entityInfo.transform.rotation);
                }

                ImGui::TreePop();
            }

            ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }
    ImGui::Spacing();
    ImGui::Separator();


    //---------------------------------
    // REDO SECTION
    //---------------------------------
    ImGui::Text("Redo (%d)", (int)redo.size());
    ImGui::Separator();

    if (redo.empty())
    {
        ImGui::TextDisabled("Redo stack empty.");
    }
    else
    {
        int redoCounter = 1;
        for (int i = (int)redo.size() - 1; i >= 0; --i, ++redoCounter)
        {
            const UndoRecord& r = redo[i];  // <-- FIXED: use REDO stack
            ImGui::PushID(6000 + i);

            const char* label = "";
            ImVec4 col;

            switch (r.action)
            {
            case UndoRecord::Type::TransformEdit:
                label = "[TransformEdit]";
                col = ImVec4(0.5f, 0.8f, 1.f, 1.f);
                break;
            case UndoRecord::Type::ColliderEdit:
                label = "[ColliderEdit]";
                col = ImVec4(1.f, 0.85f, 0.4f, 1.f);
                break;
            case UndoRecord::Type::ColliderToggle:
                label = "[ColliderToggle]";
                col = ImVec4(0.95f, 0.75f, 0.25f, 1.f);
                break;
            case UndoRecord::Type::EntityCreated:
                label = "[EntityCreated]";
                col = ImVec4(0.6f, 1.f, 0.6f, 1.f);
                break;
            case UndoRecord::Type::EntityDeleted:
                label = "[EntityDeleted]";
                col = ImVec4(1.f, 0.5f, 0.5f, 1.f);
                break;
            case UndoRecord::Type::MeshRendererEdit:
                label = "[MeshRendererEdit]";
                col = ImVec4(1.0f, 0.7f, 0.3f, 1.f);
                break;
            case UndoRecord::Type::MeshRendererToggle:
                label = "[MeshRendererToggle]";
                col = ImVec4(0.9f, 0.55f, 1.0f, 1.f);
                break;
            case UndoRecord::Type::SpriteAnimatorEdit:
                label = "[SpriteAnimatorEdit]";
                col = ImVec4(0.45f, 1.0f, 0.85f, 1.f);
                break;
            case UndoRecord::Type::SpriteAnimatorToggle:
                label = "[SpriteAnimatorToggle]";
                col = ImVec4(0.75f, 0.6f, 1.0f, 1.f);
                break;
            case UndoRecord::Type::ParticleEmitterEdit:
                label = "[ParticleEmitterEdit]";
                col = ImVec4(1.0f, 0.75f, 0.35f, 1.f);
                break;
            case UndoRecord::Type::ParticleEmitterToggle:
                label = "[ParticleEmitterToggle]";
                col = ImVec4(1.0f, 0.6f, 0.2f, 1.f);
                break;
            case UndoRecord::Type::ScriptEdit:
                label = "[ScriptEdit]";
                col = ImVec4(0.95f, 0.9f, 0.35f, 1.f);
                break;
            case UndoRecord::Type::PlayerControllerToggle:
                label = "[PlayerControllerToggle]";
                col = ImVec4(0.55f, 0.9f, 0.55f, 1.f);
                break;
            case UndoRecord::Type::SpeedComponentToggle:
                label = "[SpeedComponentToggle]";
                col = ImVec4(0.45f, 0.85f, 0.95f, 1.f);
                break;
            case UndoRecord::Type::MassComponentToggle:
                label = "[MassComponentToggle]";
                col = ImVec4(0.95f, 0.65f, 0.45f, 1.f);
                break;
            case UndoRecord::Type::PersistentTagToggle:
                label = "[PersistentTagToggle]";
                col = ImVec4(0.75f, 0.85f, 0.45f, 1.f);
                break;
            case UndoRecord::Type::LightEdit:
                label = "[LightEdit]";
                col = ImVec4(1.00f, 0.85f, 0.35f, 1.f);
                break;

            case UndoRecord::Type::LightToggle:
                label = "[LightToggle]";
                col = ImVec4(1.00f, 0.80f, 0.20f, 1.f);
                break;
            case UndoRecord::Type::EnemyControllerToggle:
                label = "[EnemyControllerToggle]";
                col = ImVec4(0.85f, 0.45f, 0.45f, 1.f);
                break;
            default:
                label = "[Unknown]";
                col = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
                break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, col);

            std::string header =
                std::to_string(redoCounter) + ") " + label + "##redo_" + std::to_string(i);

            bool open = ImGui::TreeNode(header.c_str());

            if (ImGui::IsItemClicked())
            {
                Entity e = INVALID_ENTITY;
                switch (r.action)
                {
                case UndoRecord::Type::TransformEdit: e = r.before.entity; break;
                case UndoRecord::Type::ColliderEdit:
                case UndoRecord::Type::ColliderToggle: e = r.beforeCollider.entity; break;
                case UndoRecord::Type::EntityCreated:
                case UndoRecord::Type::EntityDeleted: e = r.entityInfo.entity; break;
                case UndoRecord::Type::MeshRendererEdit:
                case UndoRecord::Type::MeshRendererToggle: e = r.beforeRenderer.entity; break;
                case UndoRecord::Type::SpriteAnimatorEdit:
                case UndoRecord::Type::SpriteAnimatorToggle: e = r.beforeAnimator.entity; break;
                case UndoRecord::Type::ParticleEmitterEdit:
                case UndoRecord::Type::ParticleEmitterToggle: e = r.beforeEmitter.entity; break;
                case UndoRecord::Type::ScriptEdit: e = r.beforeScript.entity; break;
                case UndoRecord::Type::PlayerControllerToggle: e = r.beforeController.entity; break;
                case UndoRecord::Type::SpeedComponentToggle: e = r.beforeSpeed.entity; break;
                case UndoRecord::Type::MassComponentToggle: e = r.beforeMass.entity; break;
                case UndoRecord::Type::PersistentTagToggle: e = r.beforePersistentTag.entity; break;
                case UndoRecord::Type::LightEdit:
                case UndoRecord::Type::LightToggle: e = r.beforeLight.entity; break;
                case UndoRecord::Type::EnemyControllerToggle: e = r.beforeEnemyController.entity; break;
                default: break;
                }

                int idx = findIndex(e);
                if (idx != -1)
                {
                    selectedEntity = idx;
                    focusPropsNextFrame = true;
                    followSelection = true;
                    followEntity = e;
                }
            }
            if (open)
            {
                Entity e = INVALID_ENTITY;
                switch (r.action)
                {
                case UndoRecord::Type::TransformEdit: e = r.before.entity; break;
                case UndoRecord::Type::ColliderEdit:
                case UndoRecord::Type::ColliderToggle: e = r.beforeCollider.entity; break;
                case UndoRecord::Type::EntityCreated:
                case UndoRecord::Type::EntityDeleted: e = r.entityInfo.entity; break;
                case UndoRecord::Type::MeshRendererEdit:
                case UndoRecord::Type::MeshRendererToggle: e = r.beforeRenderer.entity; break;
                case UndoRecord::Type::SpriteAnimatorEdit:
                case UndoRecord::Type::SpriteAnimatorToggle: e = r.beforeAnimator.entity; break;
                case UndoRecord::Type::ParticleEmitterEdit:
                case UndoRecord::Type::ParticleEmitterToggle: e = r.beforeEmitter.entity; break;
                case UndoRecord::Type::ScriptEdit: e = r.beforeScript.entity; break;
                case UndoRecord::Type::PlayerControllerToggle: e = r.beforeController.entity; break;
                case UndoRecord::Type::SpeedComponentToggle: e = r.beforeSpeed.entity; break;
                case UndoRecord::Type::MassComponentToggle: e = r.beforeMass.entity; break;
                case UndoRecord::Type::PersistentTagToggle: e = r.beforePersistentTag.entity; break;
                case UndoRecord::Type::LightEdit:
                case UndoRecord::Type::LightToggle: e = r.beforeLight.entity; break;
                case UndoRecord::Type::EnemyControllerToggle: e = r.beforeEnemyController.entity; break;
                default: break;
                }

                ImGui::Text("Entity: %u", (unsigned)e);

                if (r.action == UndoRecord::Type::TransformEdit)
                {
                    ImGui::SeparatorText("Before");
                    ImGui::Text("Pos:   %.1f, %.1f", r.before.position.x, r.before.position.y);
                    ImGui::Text("Scale: %.1f, %.1f", r.before.scale.x, r.before.scale.y);
                    ImGui::Text("Rot:   %.1f", r.before.rotation);

                    ImGui::SeparatorText("After");
                    ImGui::Text("Pos:   %.1f, %.1f", r.after.position.x, r.after.position.y);
                    ImGui::Text("Scale: %.1f, %.1f", r.after.scale.x, r.after.scale.y);
                    ImGui::Text("Rot:   %.1f", r.after.rotation);
                }
                else if (r.action == UndoRecord::Type::ColliderEdit ||
                    r.action == UndoRecord::Type::ColliderToggle)
                {
                    ImGui::SeparatorText("Before");
                    ImGui::Text("Has Collider: %s", r.beforeCollider.hasCollider ? "true" : "false");
                    ImGui::Text("Size:      %.1f, %.1f", r.beforeCollider.size.x, r.beforeCollider.size.y);
                    ImGui::Text("IsTrigger: %s", r.beforeCollider.isTrigger ? "true" : "false");

                    ImGui::SeparatorText("After");
                    ImGui::Text("Has Collider: %s", r.afterCollider.hasCollider ? "true" : "false");
                    ImGui::Text("Size:      %.1f, %.1f", r.afterCollider.size.x, r.afterCollider.size.y);
                    ImGui::Text("IsTrigger: %s", r.afterCollider.isTrigger ? "true" : "false");
                }
                else if (r.action == UndoRecord::Type::MeshRendererEdit ||
                    r.action == UndoRecord::Type::MeshRendererToggle)
                {
                    ImGui::SeparatorText("Before");
                    ImGui::Text("Has Renderer: %s", r.beforeRenderer.hasRenderer ? "true" : "false");
                    ImGui::Text("Color: %.2f, %.2f, %.2f",
                        r.beforeRenderer.color.x,
                        r.beforeRenderer.color.y,
                        r.beforeRenderer.color.z);
                    ImGui::Text("Texture ID: %u", r.beforeRenderer.texture);
                    ImGui::Text("Mesh: %p", (void*)r.beforeRenderer.mesh);

                    ImGui::SeparatorText("After");
                    ImGui::Text("Has Renderer: %s", r.afterRenderer.hasRenderer ? "true" : "false");
                    ImGui::Text("Color: %.2f, %.2f, %.2f",
                        r.afterRenderer.color.x,
                        r.afterRenderer.color.y,
                        r.afterRenderer.color.z);
                    ImGui::Text("Texture ID: %u", r.afterRenderer.texture);
                    ImGui::Text("Mesh: %p", (void*)r.afterRenderer.mesh);
                }
                else if (r.action == UndoRecord::Type::SpriteAnimatorEdit ||
                    r.action == UndoRecord::Type::SpriteAnimatorToggle)
                {
                    ImGui::SeparatorText("Before");
                    ImGui::Text("Has Animator: %s", r.beforeAnimator.hasAnimator ? "true" : "false");
                    ImGui::Text("Current Frame: %d", r.beforeAnimator.cur);
                    ImGui::Text("Start Frame: %d", r.beforeAnimator.startFrame);
                    ImGui::Text("End Frame: %d", r.beforeAnimator.endFrame);
                    ImGui::Text("Speed: %.2f", r.beforeAnimator.speed);
                    ImGui::Text("Sheet: %p", (const void*)r.beforeAnimator.sheet);

                    ImGui::SeparatorText("After");
                    ImGui::Text("Has Animator: %s", r.afterAnimator.hasAnimator ? "true" : "false");
                    ImGui::Text("Current Frame: %d", r.afterAnimator.cur);
                    ImGui::Text("Start Frame: %d", r.afterAnimator.startFrame);
                    ImGui::Text("End Frame: %d", r.afterAnimator.endFrame);
                    ImGui::Text("Speed: %.2f", r.afterAnimator.speed);
                    ImGui::Text("Sheet: %p", (const void*)r.afterAnimator.sheet);
                }
                else if (r.action == UndoRecord::Type::ParticleEmitterEdit ||
                    r.action == UndoRecord::Type::ParticleEmitterToggle)
                {
                    ImGui::SeparatorText("Before");
                    ImGui::Text("Has Emitter: %s", r.beforeEmitter.hasEmitter ? "true" : "false");
                    ImGui::Text("Enabled: %s", r.beforeEmitter.enabled ? "true" : "false");
                    ImGui::Text("Rate: %.2f", r.beforeEmitter.rate);
                    ImGui::Text("Life: %.2f", r.beforeEmitter.particleLife);
                    ImGui::Text("Offset: %.1f, %.1f", r.beforeEmitter.offset.x, r.beforeEmitter.offset.y);
                    ImGui::Text("VelMin: %.1f, %.1f", r.beforeEmitter.velMin.x, r.beforeEmitter.velMin.y);
                    ImGui::Text("VelMax: %.1f, %.1f", r.beforeEmitter.velMax.x, r.beforeEmitter.velMax.y);
                    ImGui::Text("ColorStart: %.2f, %.2f, %.2f",
                        r.beforeEmitter.colorStart.x,
                        r.beforeEmitter.colorStart.y,
                        r.beforeEmitter.colorStart.z);
                    ImGui::Text("ColorEnd: %.2f, %.2f, %.2f",
                        r.beforeEmitter.colorEnd.x,
                        r.beforeEmitter.colorEnd.y,
                        r.beforeEmitter.colorEnd.z);
                    ImGui::Text("SizeStart: %.2f", r.beforeEmitter.sizeStart);
                    ImGui::Text("SizeEnd: %.2f", r.beforeEmitter.sizeEnd);
                    ImGui::Text("Texture ID: %u", r.beforeEmitter.texture);
                    ImGui::Text("Quad: %p", (void*)r.beforeEmitter.quad);

                    ImGui::SeparatorText("After");
                    ImGui::Text("Has Emitter: %s", r.afterEmitter.hasEmitter ? "true" : "false");
                    ImGui::Text("Enabled: %s", r.afterEmitter.enabled ? "true" : "false");
                    ImGui::Text("Rate: %.2f", r.afterEmitter.rate);
                    ImGui::Text("Life: %.2f", r.afterEmitter.particleLife);
                    ImGui::Text("Offset: %.1f, %.1f", r.afterEmitter.offset.x, r.afterEmitter.offset.y);
                    ImGui::Text("VelMin: %.1f, %.1f", r.afterEmitter.velMin.x, r.afterEmitter.velMin.y);
                    ImGui::Text("VelMax: %.1f, %.1f", r.afterEmitter.velMax.x, r.afterEmitter.velMax.y);
                    ImGui::Text("ColorStart: %.2f, %.2f, %.2f",
                        r.afterEmitter.colorStart.x,
                        r.afterEmitter.colorStart.y,
                        r.afterEmitter.colorStart.z);
                    ImGui::Text("ColorEnd: %.2f, %.2f, %.2f",
                        r.afterEmitter.colorEnd.x,
                        r.afterEmitter.colorEnd.y,
                        r.afterEmitter.colorEnd.z);
                    ImGui::Text("SizeStart: %.2f", r.afterEmitter.sizeStart);
                    ImGui::Text("SizeEnd: %.2f", r.afterEmitter.sizeEnd);
                    ImGui::Text("Texture ID: %u", r.afterEmitter.texture);
                    ImGui::Text("Quad: %p", (void*)r.afterEmitter.quad);
                }
                else if (r.action == UndoRecord::Type::ScriptEdit)
                {
                    const auto& b = r.beforeScript;
                    const auto& a = r.afterScript;

                    const std::string beforeText =
                        (!b.hasScript || b.scriptName.empty()) ? "None" : b.scriptName;

                    const std::string afterText =
                        (!a.hasScript || a.scriptName.empty()) ? "None" : a.scriptName;

                    ImGui::Text("Before Script: %s", beforeText.c_str());
                    ImGui::Text("After Script:  %s", afterText.c_str());
                }
                else if (r.action == UndoRecord::Type::PlayerControllerToggle)
                {
                    ImGui::SeparatorText("PlayerController");
                    ImGui::Text("Before: %s", r.beforeController.hasController ? "Present" : "Absent");
                    ImGui::Text("After:  %s", r.afterController.hasController ? "Present" : "Absent");
                }
                else if (r.action == UndoRecord::Type::SpeedComponentToggle)
                {
                    ImGui::SeparatorText("SpeedComponent");
                    ImGui::Text("Before: %s", r.beforeSpeed.hasSpeed ? "Present" : "Absent");
                    ImGui::Text("After:  %s", r.afterSpeed.hasSpeed ? "Present" : "Absent");
                }
                else if (r.action == UndoRecord::Type::MassComponentToggle)
                {
                    ImGui::SeparatorText("MassComponent");
                    ImGui::Text("Before: %s", r.beforeMass.hasMass ? "Present" : "Absent");
                    ImGui::Text("After:  %s", r.afterMass.hasMass ? "Present" : "Absent");
                }
                else if (r.action == UndoRecord::Type::PersistentTagToggle)
                {
                    ImGui::SeparatorText("PersistentTag");
                    ImGui::Text("Before: %s", r.beforePersistentTag.hasPersistentTag ? "Present" : "Absent");
                    ImGui::Text("After:  %s", r.afterPersistentTag.hasPersistentTag ? "Present" : "Absent");
                }
                else if (r.action == UndoRecord::Type::LightEdit ||
                    r.action == UndoRecord::Type::LightToggle)
                {
                    ImGui::SeparatorText("LightComponent");
                    ImGui::Text("Before: %s", r.beforeLight.hasLight ? "Present" : "Absent");
                    ImGui::Text("After:  %s", r.afterLight.hasLight ? "Present" : "Absent");

                    if (r.beforeLight.hasLight || r.afterLight.hasLight)
                    {
                        ImGui::Text("Glow Radius: %.2f -> %.2f",
                            r.beforeLight.glowRadius, r.afterLight.glowRadius);
                        ImGui::Text("Glow Intensity: %.2f -> %.2f",
                            r.beforeLight.glowIntensity, r.afterLight.glowIntensity);
                        ImGui::Text("Source Radius: %.2f -> %.2f",
                            r.beforeLight.sourceRadius, r.afterLight.sourceRadius);
                    }
                }
                else if (r.action == UndoRecord::Type::EnemyControllerToggle)
                {
                    ImGui::SeparatorText("EnemyController");
                    ImGui::Text("Before: %s",
                        r.beforeEnemyController.hasEnemyController ? "Present" : "Absent");
                    ImGui::Text("After:  %s",
                        r.afterEnemyController.hasEnemyController ? "Present" : "Absent");
                }
                else
                {
                    ImGui::SeparatorText("Transform Snapshot");
                    ImGui::Text("Pos:   %.1f, %.1f", r.entityInfo.transform.position.x, r.entityInfo.transform.position.y);
                    ImGui::Text("Scale: %.1f, %.1f", r.entityInfo.transform.scale.x, r.entityInfo.transform.scale.y);
                    ImGui::Text("Rot:   %.1f", r.entityInfo.transform.rotation);
                }

                ImGui::TreePop();
            }

            ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }

    ImGui::End();
}

/**
 * @brief Draws a centered bottom panel for switching between levels.
 *
 * Anchored to the bottom-center of the viewport, this panel provides
 * buttons for loading different scene JSONs via the bound callback.
 */
void EditorOverlay::DrawLevelSelector()
{
    if (!glfwGetCurrentContext()) return;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 0.85f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::Begin("Level Selector", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    float barWidth = ImGui::GetContentRegionAvail().x;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.30f, 0.30f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f, 0.50f, 0.50f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.45f, 1.00f, 0.9f));


    const std::pair<const char*, const char*> levels[] = {
        { "Labyrinth", "scene/labyrinth.json" },
        { "Room 1",    "scene/entities_Level1.json" },
        { "Room 2",    "scene/entities_Level2.json" },
        { "Room 3",    "scene/entities_Level3.json" },
        { "Room 4",    "scene/entities_Level4.json" },
        { "Room 5",    "scene/entities_Level5.json" },
    };

    const float btnW = 90.0f;
    const float btnH = 35.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const int numLevels = IM_ARRAYSIZE(levels);

    float totalBtnW = numLevels * btnW
        + (numLevels - 1) * spacing
        + spacing
        + btnW; // Save button

    if (barWidth > totalBtnW)
        ImGui::SetCursorPosX((barWidth - totalBtnW) * 0.5f);

    for (int i = 0; i < numLevels; ++i) {
        if (ImGui::Button(levels[i].first, ImVec2(btnW, btnH))) {
            // Helper for map key
            auto makeLevelKey = [&](const std::string& path) -> std::string {
                return path.empty() ? std::string("<default>") : path;
                };

            // 1) Save current level's grid into cache (if we are editing)
            if (tileEditMode && gridW_ > 0 && gridH_ > 0 && !tileGrid_.empty()) {
                const std::string prevKey = makeLevelKey(currentLevelPath);
                tileGridPerLevel[prevKey] = tileGrid_;
                tileSizePerLevel[prevKey] = { gridW_, gridH_ };
            }

            // 2) Switch level
            currentLevelPath = levels[i].second;

            // 3) If tile edit mode is ON, try to restore cached grid for new level
            if (tileEditMode) {
                const std::string newKey = makeLevelKey(currentLevelPath);
                auto itGrid = tileGridPerLevel.find(newKey);
                auto itSize = tileSizePerLevel.find(newKey);
                if (itGrid != tileGridPerLevel.end() && itSize != tileSizePerLevel.end()) {
                    // Already cached: just restore
                    tileGrid_ = itGrid->second;
                    gridW_ = itSize->second.first;
                    gridH_ = itSize->second.second;
                    tileDirty = false;
                }
                else if (onTileEditEnter) {
                    // Not cached yet: ask GameApp to load tiles for this new level
                    onTileEditEnter();   // GameApp fills tileGrid_, gridW_, gridH_ for currentLevelPath

                    // Cache what we got back, if valid
                    if (gridW_ > 0 && gridH_ > 0 && !tileGrid_.empty()) {
                        tileGridPerLevel[newKey] = tileGrid_;
                        tileSizePerLevel[newKey] = { gridW_, gridH_ };
                    }
                    tileDirty = false;
                }
            }

            // 4) Ask game to reload the ECS/entities for this level
            if (onLevelChange) {
                onLevelChange(currentLevelPath);
            }
            else {
                // fallback
                RequestLevelReload(currentLevelPath);
            }

        }
        ImGui::SameLine();
    }

    if (ImGui::Button("Save", ImVec2(btnW, btnH))) {
        if (onLevelSave)
            onLevelSave(currentLevelPath);
    }

    ImGui::PopStyleColor(3);
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

/**
    * @brief Displays a scrollable list of available textures/assets.
    *
    * The Assets Browser lists all registered textures (as thumbnails or names)
    * allowing users to:
    *  - preview texture options
    *  - drag a texture payload into the viewport
    *  - apply texture changes to selected entities
    *
    * - Drag begins by holding LMB on a texture entry
    * - Drag payload contains the texture name as a string
    * - Delivery is handled by DrawViewportDropTarget() when released over entity
    *
*/
void EditorOverlay::DrawAssetsBrowser()
{
    if (!glfwGetCurrentContext()) return;

    ImGui::Begin("Assets Browser", nullptr,
        ImGuiWindowFlags_NoCollapse);

    // ---------- Directory browser (navigate anywhere under Assets) ----------
    static std::filesystem::path currentDir = std::filesystem::path("Assets"); // capital A
    static std::string fileSelected;                 // absolute path of picked file
    static std::vector<std::string> s_newlyImported; // UI cache so names appear immediately

    ImGui::Text("Folder: %s", currentDir.string().c_str());
    ImGui::Separator();

    // Up one level (but not above Assets)
    if (currentDir != std::filesystem::path("Assets"))
    {
        if (ImGui::Selectable(".."))
        {
            currentDir = currentDir.parent_path();
            fileSelected.clear();
        }
    }

    // List directories first, then files
    std::vector<std::filesystem::directory_entry> dirs, files;
    std::error_code ec;
    for (auto& entry : std::filesystem::directory_iterator(currentDir, ec))
    {
        if (entry.is_directory()) dirs.push_back(entry);
        else                      files.push_back(entry);
    }

    // Folders
    for (auto& d : dirs)
    {
        std::string name = d.path().filename().string() + "/";
        if (ImGui::Selectable(name.c_str()))
        {
            currentDir = d.path();
            fileSelected.clear();
        }
    }

    // --- Files: textures (.png) + audio (.wav/.mp3/.ogg) -----------------
    for(auto& f : files)
    {
        std::string name = f.path().filename().string();
        std::string ext = f.path().extension().string();
        for (auto& ch : ext) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

        const bool isTexture = (ext == ".png");
        const bool isAudioCandidate = IsAudioFileExt(ext);

        // Ignore everything else (e.g. .txt, .json, etc.)
        if (!isTexture && !isAudioCandidate)
            continue;

        if (ImGui::Selectable(name.c_str()))
        {
            if (isTexture)
                fileSelected = f.path().string(); // eligible for "Import" button
            else
                fileSelected.clear();             // audio is not imported as texture
        }

        if (ImGui::BeginDragDropSource())
        {
            if (isTexture)
            {
                // TEXTURE_NAME payload
                const std::string texName = name.substr(0, name.find_last_of('.'));
                ImGui::SetDragDropPayload("TEXTURE_NAME",
                    texName.c_str(),
                    texName.size() + 1);
                ImGui::Text("Texture: %s", texName.c_str());
            }
            else if (isAudioCandidate)
            {
                // AUDIO_PATH payload: path relative to Assets/
                std::error_code ecPayload;
                auto abs = std::filesystem::weakly_canonical(f.path(), ecPayload);
                auto root = std::filesystem::path("Assets");
                std::string relStr;

                if (!ecPayload)
                {
                    auto rel = std::filesystem::relative(abs, root, ecPayload);
                    if (!ecPayload)
                        relStr = rel.generic_string();  // e.g. "audio/foo.wav"
                }

                if (relStr.empty())
                    relStr = name;

                ImGui::SetDragDropPayload(kAudioPayloadType,
                    relStr.c_str(),
                    relStr.size() + 1);
                ImGui::Text("Audio: %s", relStr.c_str());
            }

            ImGui::EndDragDropSource();
        }
    }

    ImGui::Separator();

    // ---------- Texture list (manager list + newly imported cache) ----------
    const auto& mgrNames = context.GetTextureList();
    std::vector<std::string> allNames;
    allNames.reserve(mgrNames.size() + s_newlyImported.size());
    allNames.insert(allNames.end(), mgrNames.begin(), mgrNames.end());
    for (auto& n : s_newlyImported)
        if (std::find(allNames.begin(), allNames.end(), n) == allNames.end())
            allNames.push_back(n);

    ImGui::Text("Textures:");
    const float fullW = std::max(1.0f, ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginListBox("Entities", ImVec2(fullW, 100)))
    {
        const auto& entities = context.GetEntities();

        for (const auto& name : allNames)
        {
            bool sel = false;
            if (ImGui::Selectable(name.c_str(), sel))
            {
                if (selectedEntity >= 0 && selectedEntity < (int)entities.size())
                    context.ApplyTexture(entities[selectedEntity], name);
            }

            // Drag source for viewport drop
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::SetDragDropPayload("TEXTURE_NAME", name.c_str(), name.size() + 1);
                ImGui::Text("Apply: %s", name.c_str());
                ImGui::EndDragDropSource();
            }
        }

        ImGui::EndListBox();
    }

    // ---------- Import row ----------
    ImGui::Separator();
    if (!fileSelected.empty())
        ImGui::Text("Selected: %s", std::filesystem::path(fileSelected).filename().string().c_str());
    else
        ImGui::TextDisabled("Selected: <none>");

    ImGui::BeginDisabled(fileSelected.empty());
    if (ImGui::Button("Import to Assets/textures"))
    {
        std::filesystem::path src(fileSelected);
        std::filesystem::path dstDir = "Assets/textures";  // capital A
        std::filesystem::create_directories(dstDir, ec);
        if (ec) DebugConsole::Get().Error("Failed to create Assets/textures: " + ec.message());

        std::filesystem::path dst = dstDir / src.filename();

        // Skip copy if already under Assets/textures
        bool needsCopy = true;
        {
            std::error_code ec2;
            auto absSrc = std::filesystem::weakly_canonical(src, ec2);
            auto absDstDi = std::filesystem::weakly_canonical(dstDir, ec2);
            if (!ec2)
            {
                std::string as = absSrc.string();
                std::string ad = absDstDi.string();
#ifdef _WIN32
                std::transform(as.begin(), as.end(), as.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                std::transform(ad.begin(), ad.end(), ad.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
#endif
                if (as.rfind(ad, 0) == 0) { dst = absSrc; needsCopy = false; }
            }
        }

        if (needsCopy)
        {
            std::error_code cpec;
            std::filesystem::copy_file(src, dst,
                std::filesystem::copy_options::overwrite_existing, cpec);
            if (cpec)
                DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "File type: Editor.cpp", "Details: Copy failed : " + cpec.message(), "  from " + src.string(), " to " + dst.string(), "\n");
            else
                DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "File type: Editor.cpp", "Details: Copied: " + dst.string() + "\n");
        }

        const std::string texName = dst.stem().string();

        // Load/register into runtime and ensure it appears in UI immediately
        if (context.ImportTexture(dst.string(), texName))
        {
            if (std::find(s_newlyImported.begin(), s_newlyImported.end(), texName) == s_newlyImported.end())
                s_newlyImported.push_back(texName);
            DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "File type: Editor.cpp", "Details: [Assets] Imported live texture: " + texName + " (" + dst.string() + ")\n");
        }
        else
        {
            DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "File type: Editor.cpp", "Details: [Assets] ImportTexture Failed for: " + dst.string() + "\n");
        }

        fileSelected.clear();
    }
    ImGui::EndDisabled();

    // ---------- Audio preview controls ----------
    ImGui::Separator();
    ImGui::BeginDisabled(gAudioPreviewVoice == 0);
    if (ImGui::Button("Stop Audio Preview"))
    {
        if (gAudioPreviewVoice != 0)
        {
            Audio::Stop(gAudioPreviewVoice, /*fadeOutMs=*/150);
            gAudioPreviewVoice = 0;

            DebugConsole::Get().AddFormattedMessage(
                LogLevel::Info,
                "File type: Editor.cpp",
                "Details: [Editor] Stopped audio preview\n"
            );
        }
    }
    ImGui::EndDisabled();

    ImGui::End();
}

/**
    * @brief Handles drag-and-drop texture assignment inside the 2D world viewport.
    *
    * When the user drags a texture from the Assets Browser and releases it:
    *
    * 1) Convert mouse release point → world coordinates
    * 2) Hit-test to determine which entity (if any) is beneath cursor
    * 3) If valid and entity has a MeshRenderer:
    *      → Update the entity’s assigned texture
    *
    * @warning No action is taken if:
    *    - No hit entity exists
    *    - Entity has no renderer component
    *    - Payload is invalid or no texture matches
*/
void EditorOverlay::DrawViewportDropTarget()
{
    if (!glfwGetCurrentContext()) return;
    if (!activeCamera) return;

    const auto& entities = context.GetEntities();
    if (entities.empty()) return;

    // We are called right after drawing the game Image in the "Viewport" window.
    // The last ImGui item is that Image, so BeginDragDropTarget() will attach
    // the target to the image rect.
    if (!ImGui::BeginDragDropTarget())
        return;

    // --- AUDIO_PATH: play preview, or show error popup --------------------
    if (const ImGuiPayload* audioPayload =
        ImGui::AcceptDragDropPayload(kAudioPayloadType,
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
    {
        const char* relPath = static_cast<const char*>(audioPayload->Data);
        std::string pathStr = (relPath ? relPath : "");

        DebugConsole::Get().AddFormattedMessage(
            LogLevel::Info,
            "File type: Editor.cpp",
            "Details: [Editor] AUDIO_PATH payload received: " + (pathStr.empty() ? std::string("<empty>") : pathStr) + "\n"
        );

        if (!pathStr.empty())
        {
            std::filesystem::path rel(pathStr);
            std::string ext = rel.extension().string();
            for (auto& ch : ext) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

            const bool supported = IsAudioFileExt(ext);

            if (!supported)
            {
                gAudioErrorMessage =
                    "Unsupported audio format: " + ext +
                    "\nSupported formats: .wav, .mp3, .ogg";
                gAudioErrorPopup = true;

                DebugConsole::Get().AddFormattedMessage(
                    LogLevel::Error,
                    "File type: Editor.cpp",
                    "Details: [Editor] Unsupported audio dropped: " + pathStr + "\n"
                );
            }
            else
            {
                std::filesystem::path full = std::filesystem::path("Assets") / rel;

                DebugConsole::Get().AddFormattedMessage(
                    LogLevel::Info,
                    "File type: Editor.cpp",
                    "Details: [Editor] Trying to load + play: " + full.string() + "\n"
                );

                Audio::SoundID sid = Audio::LoadSound(full.string());
                if (sid >= 0)
                {
                    // If we already have a preview playing, stop it first (small fade out)
                    if (gAudioPreviewVoice != 0)
                    {
                        Audio::Stop(gAudioPreviewVoice, /*fadeOutMs=*/150);
                        gAudioPreviewVoice = 0;
                    }

                    Audio::PlayDesc d;
                    d.bus = Audio::Bus::Sfx;
                    d.loop = false;
                    d.gain = 1.0f;

                    // Start new preview and store its handle
                    gAudioPreviewVoice = Audio::Play(sid, d);

                    if (gAudioPreviewVoice == 0)
                    {
                        gAudioErrorMessage =
                            "Failed to start audio playback:\n" + full.string();
                        gAudioErrorPopup = true;

                        DebugConsole::Get().AddFormattedMessage(
                            LogLevel::Error,
                            "File type: Editor.cpp",
                            "Details: [Editor] Failed to start audio playback for: " + full.string() + "\n"
                        );
                    }
                }
                else
                {
                    gAudioErrorMessage =
                        "Failed to load audio file:\n" + full.string();
                    gAudioErrorPopup = true;

                    DebugConsole::Get().AddFormattedMessage(
                        LogLevel::Error,
                        "File type: Editor.cpp",
                        "Details: [Editor] Failed to load audio: " + full.string() + "\n"
                    );
                }
            }
        }
    }

    // --- TEXTURE_NAME: apply texture to entity under cursor ---------------
    if (const ImGuiPayload* payload =
        ImGui::AcceptDragDropPayload("TEXTURE_NAME",
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
    {
        const char* texName = static_cast<const char*>(payload->Data);
        ImGuiIO& io = ImGui::GetIO();

        // Need a valid game image rect (set in EditorOverlay::Draw)
        if (gViewportImageSize.x > 0.0f && gViewportImageSize.y > 0.0f)
        {
            // Mouse normalized inside the game image
            float u = (io.MousePos.x - gViewportImagePos.x) / gViewportImageSize.x;
            float v = (gViewportImagePos.y + gViewportImageSize.y - io.MousePos.y) / gViewportImageSize.y;

            if (u >= 0.0f && u <= 1.0f &&
                v >= 0.0f && v <= 1.0f)
            {
                // Map to camera framebuffer pixels (BL origin)
                float fbX = u * (float)activeCamera->viewportWidth;
                float fbY = v * (float)activeCamera->viewportHeight;
                Vector2 mouseBL(fbX, fbY);

                // World-space mouse
                Vector2 mouseWorld = activeCamera->ScreenToWorld(mouseBL);

                // Top-most hit entity under cursor
                Entity hit = INVALID_ENTITY;
                for (int i = (int)entities.size() - 1; i >= 0; --i)
                {
                    Entity e = entities[i];
                    const Transform* t = context.TryGetTransform(e);
                    if (!t) continue;

                    Vector2 size = t->GetScale();
                    if (const Collider* c = context.TryGetCollider(e))
                        size = c->size;

                    Vector2 aMin = t->GetPosition();
                    Vector2 aMax = aMin + size;

                    bool inside =
                        (mouseWorld.x >= aMin.x && mouseWorld.x <= aMax.x) &&
                        (mouseWorld.y >= aMin.y && mouseWorld.y <= aMax.y);

                    if (inside) { hit = e; break; }
                }

                if (hit != INVALID_ENTITY)
                {
                    context.ApplyTexture(hit, texName);
                    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "File type: Editor.cpp", "Details: [Drag and Drop] Applied '" + std::string(texName) + "' to entity " + std::to_string(hit) + "\n");
                }
            }
        }
    }

    if (const ImGuiPayload* payload =
        ImGui::AcceptDragDropPayload("PREFAB_KEY",
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
    {
        const char* prefabName = static_cast<const char*>(payload->Data);
        ImGuiIO& io = ImGui::GetIO();

        if (gViewportImageSize.x > 0.0f && gViewportImageSize.y > 0.0f &&
            activeCamera && prefabName && prefabName[0] != '\0')
        {
            float u = (io.MousePos.x - gViewportImagePos.x) / gViewportImageSize.x;
            float v = (gViewportImagePos.y + gViewportImageSize.y - io.MousePos.y) / gViewportImageSize.y;

            if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f)
            {
                float fbX = u * (float)activeCamera->viewportWidth;
                float fbY = v * (float)activeCamera->viewportHeight;

                Vector2 mouseWorld = activeCamera->ScreenToWorld(Vector2(fbX, fbY));

                const float tileSize = labyrinthTileSize_;
                int gx = static_cast<int>(std::floor(mouseWorld.x / tileSize));
                int gy = static_cast<int>(std::floor(mouseWorld.y / tileSize));

                Vector2 snappedPos(
                    gx * tileSize,
                    gy * tileSize
                );

                spawnRequest.type = SpawnRequest::Type::Prefab;
                spawnRequest.prefabName = prefabName;
                spawnRequest.pos = snappedPos;
                spawnRequest.rotation = 0.0f;
                UpsertLabyrinthWallVariant(gx, gy, prefabName, false, true);
            }
        }
    }

    if (const ImGuiPayload* payload =
        ImGui::AcceptDragDropPayload("WALL_TEXTUREKEY",
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
    {
        const char* texName = static_cast<const char*>(payload->Data);
        ImGuiIO& io = ImGui::GetIO();

        if (gViewportImageSize.x > 0.0f && gViewportImageSize.y > 0.0f &&
            activeCamera && texName && texName[0] != '\0')
        {
            float u = (io.MousePos.x - gViewportImagePos.x) / gViewportImageSize.x;
            float v = (gViewportImagePos.y + gViewportImageSize.y - io.MousePos.y) / gViewportImageSize.y;

            if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f)
            {
                float fbX = u * (float)activeCamera->viewportWidth;
                float fbY = v * (float)activeCamera->viewportHeight;

                Vector2 mouseWorld = activeCamera->ScreenToWorld(Vector2(fbX, fbY));

                const float tileSize = 100.f; // replace if you already store this centrally
                int gx = static_cast<int>(std::floor(mouseWorld.x / tileSize));
                int gy = static_cast<int>(std::floor(mouseWorld.y / tileSize));

                Entity wall = FindLabyrinthWallEntity(gx, gy);
                if (wall != INVALID_ENTITY)
                {
                    context.ApplyTexture(wall, texName);

                    UpsertLabyrinthWallVariant(gx, gy, texName, true);

                    DebugConsole::Get().AddFormattedMessage(
                        LogLevel::Info,
                        "File type: Editor.cpp",
                        "Details: [Wall DragDrop] Applied '" + std::string(texName) +
                        "' to wall cell (" + std::to_string(gx) + ", " + std::to_string(gy) +
                        "), entity " + std::to_string(wall) + "\n");
                }
                else
                {
                    UpsertLabyrinthWallVariant(gx, gy, texName, true);

                    if (onCreateLabyrinthWallVariant)
                    {
                        Entity newWall = onCreateLabyrinthWallVariant(gx, gy, texName, labyrinthTileSize_);
                        if (newWall != INVALID_ENTITY)
                        {
                            RegisterLabyrinthWallEntity(gx, gy, newWall);
                        }
                    }
                }
            }
        }
    }

    ImGui::EndDragDropTarget();
}

/**
 * @brief Draws the bottom-right “Camera Mode” mini panel.
 * @details
 *  Shows the current camera mode and two buttons:
 *  - **Navigation Camera**: enables free navigation (click to center view),
 *    fires onRequestCameraLock(false).
 *  - **Default Camera**: restores normal/follow camera,
 *    fires onRequestCameraLock(true).
 *  This function only renders UI; the click-to-center behavior is handled in
 *  HandleNavigationClick().
 */
void EditorOverlay::DrawCameraModePanel()
{
    if (!glfwGetCurrentContext()) return;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    ImGui::Begin("Camera Mode", nullptr,
        ImGuiWindowFlags_NoCollapse);


    // Current mode indicator
    ImGui::TextDisabled("Camera Mode:");
    ImGui::SameLine();
    ImGui::Text("%s", navMode ? "Navigation" : "Default");

    // Buttons
    if (ImGui::Button("Navigation Camera"))
    {
        navMode = true;
        followSelection = false;
        gizmoEnabled = false;
        followEntity = 0;
        if (onRequestCameraLock) onRequestCameraLock(false); // unlock follow
    }
    ImGui::SameLine();
    if (ImGui::Button("Default Camera"))
    {
        navMode = false;
        followSelection = false;
        followEntity = 0;
        if (onRequestCameraLock) onRequestCameraLock(true);  // re-lock follow
    }

    ImGui::End();

    // on screen guide on how to use the navigation 
    if (navMode)
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImU32 col = IM_COL32(255, 255, 0, 220);
        const char* tip = "Click anywhere to center camera";
        ImVec2 sz = ImGui::CalcTextSize(tip);
        ImVec2 pos(vp->WorkPos.x + 18.0f,
            vp->WorkPos.y + vp->WorkSize.y - sz.y - 20.0f);
        dl->AddText(pos, col, tip);
    }
}


/**
 * @brief Handles click-to-center behavior when in Navigation mode.
 * @details
 *  When @ref navMode is true and the mouse is clicked inside the ImGui work
 *  area, the click position is converted from screen (BL) to world space,
 *  and the @ref activeCamera is re-centered there.
 *  Does nothing if ImGui is capturing the mouse or if @ref activeCamera is null.
 */
void EditorOverlay::HandleNavigationClick()
{
    if (!navMode) return;
    if (gizmoEnabled) return;
    if (!activeCamera) return;
    if (!glfwGetCurrentContext()) return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return; // ignore if UI takes the mouse

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    // Mouse must be inside the work area
    const ImVec2 m = io.MousePos;
    const bool inside =
        (m.x >= vp->WorkPos.x && m.y >= vp->WorkPos.y &&
            m.x <= vp->WorkPos.x + vp->WorkSize.x &&
            m.y <= vp->WorkPos.y + vp->WorkSize.y);

    if (!inside) return;

    // once clicked, convert to world and center the camera there
    auto& in = eng::input();
    if (in.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        // ImGui TL -> our BL (0..w,0..h)
        Vector2 mouseBL(
            (m.x - vp->WorkPos.x) * workToFBScale_.x,
            (vp->WorkPos.y + vp->WorkSize.y - m.y) * workToFBScale_.y
        );

        // To world
        Vector2 world = activeCamera->ScreenToWorld(mouseBL);

        Vector2 viewSize = activeCamera->GetViewSizeWorld();
        Vector2 newPos = world - (viewSize * 0.5f);
        activeCamera->setPosition(newPos);
    }
}

/**
 * @brief Keep Camera2D viewport in lock-step with ImGui's work area.
 * @param vp Main ImGui viewport (provides WorkPos/WorkSize in pixels).
 *
 */
void EditorOverlay::SyncCameraViewportToWorkArea_(const ImGuiViewport* vp){
    if(!activeCamera || !vp) return;

    ImGuiIO& io = ImGui::GetIO();
    workToFBScale_ = io.DisplayFramebufferScale;
}

Vector2 EditorOverlay::GetWorldMousePosition() const {
    if (!activeCamera) return Vector2(0, 0);
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    if (gViewportImageSize.x <= 0 || gViewportImageSize.y <= 0) return Vector2(0, 0);

    float u = (mousePos.x - gViewportImagePos.x) / gViewportImageSize.x;
    float v = (gViewportImagePos.y + gViewportImageSize.y - mousePos.y) / gViewportImageSize.y;

    float px = u * activeCamera->viewportWidth;
    float py = v * activeCamera->viewportHeight;

    return activeCamera->ScreenToWorld(Vector2(px, py));
}

bool EditorOverlay::IsMouseInViewport() const {
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    return (mousePos.x >= gViewportImagePos.x && mousePos.x <= gViewportImagePos.x + gViewportImageSize.x &&
            mousePos.y >= gViewportImagePos.y && mousePos.y <= gViewportImagePos.y + gViewportImageSize.y);
}

/**
 * @brief Provide the scene render target texture to the editor viewport.
 *
 * The editor uses this texture for the Viewport panel. When the texture or
 * dimensions change, the camera viewport is updated accordingly.
 *
 * @param textureId OpenGL texture ID for the scene color buffer.
 * @param width     Width of the texture in pixels.
 * @param height    Height of the texture in pixels.
 */
void EditorOverlay::SetSceneTexture(unsigned int textureId, int width, int height)
{
    // If nothing changed, do nothing.
    if (textureId == sceneTextureId &&
        width == sceneTexWidth &&
        height == sceneTexHeight){
        return;
    }

    sceneTextureId = textureId;
    sceneTexWidth = width;
    sceneTexHeight = height;

    if (activeCamera && width > 0 && height > 0) {
        activeCamera->SetViewport(0, 0, width, height);
    }
}

/**
 * @brief Keeps the editor camera centered on the selected entity (optional mode).
 *
 * When followSelection is enabled and gizmos are active, the camera is moved so
 * the selected entity stays centered in the viewport. Navigation mode disables
 * auto-follow to avoid fighting manual panning.
 */
void EditorOverlay::UpdateSelectionCameraFollow()
{
    if (!activeCamera) return;
    if (!followSelection) return;
    if (!gizmoEnabled) return;

    // If we’re in Navigation mode, do not auto-follow anything.
    if (navMode) return;

    // If the followed entity disappeared, stop following.
    const Transform* t = context.TryGetTransform(followEntity);
    if (!t) {
        followSelection = false;
        followEntity = 0;
        return;
    }
        
    Vector2 size = t->GetScale();
    if (const Collider* c = context.TryGetCollider(followEntity))
        size = c->size;

    const Vector2 center = t->GetPosition() + size * 0.5f;

    // Camera pos is bottom-left; move so that 'center' lands in the middle of the view
    Vector2 viewSize = activeCamera->GetViewSizeWorld();
    activeCamera->setPosition(center - viewSize * 0.5f);
}

/**
 * @brief Centers the next ImGui item group within the current window.
 * @param groupWidth Total width of the group to center (in pixels).
 */
void EditorOverlay::CenterGroup(float groupWidth)
{
    float windowWidth = ImGui::GetContentRegionAvail().x;
    float cursorX = (windowWidth - groupWidth) * 0.5f;
    if (cursorX < 0) cursorX = 0;
    ImGui::SetCursorPosX(cursorX);
}

/**
 * @brief Combine grid coordinates into a stable hashable key.
 *
 * Used for mapping labyrinth wall entities by their grid cell.
 *
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 * @return 64-bit packed key (gx in high 32 bits, gy in low 32 bits).
 */
long long EditorOverlay::MakeWallCellKey(int gx, int gy)
{
    return (static_cast<long long>(gx) << 32) |
        static_cast<unsigned int>(gy);
}

/**
 * @brief Register a spawned labyrinth wall entity at a grid cell.
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 * @param e  Entity ID to associate with the cell.
 */
void EditorOverlay::RegisterLabyrinthWallEntity(int gx, int gy, Entity e)
{
    labyrinthWallEntities_[MakeWallCellKey(gx, gy)] = e;
}

/**
 * @brief Find the labyrinth wall entity currently registered at a grid cell.
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 * @return Entity ID if found; INVALID_ENTITY otherwise.
 */
Entity EditorOverlay::FindLabyrinthWallEntity(int gx, int gy) const
{
    auto it = labyrinthWallEntities_.find(MakeWallCellKey(gx, gy));
    if (it == labyrinthWallEntities_.end())
        return INVALID_ENTITY;
    return it->second;
}

/**
 * @brief Clear all registered labyrinth wall entity mappings.
 */
void EditorOverlay::ClearLabyrinthWallEntities()
{
    labyrinthWallEntities_.clear();
}

/**
 * @brief Find an in-memory wall variant placement for the given grid cell.
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 * @return Pointer to the matching VariantPlacement or nullptr if not found.
 */
VariantPlacement* EditorOverlay::FindLabyrinthWallVariant(int gx, int gy)
{
    for (auto& v : labyrinthWallVariants_)
    {
        if (v.gx == gx && v.gy == gy)
            return &v;
    }
    return nullptr;
}

/**
 * @brief Find an in-memory wall variant placement for the given grid cell.
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 * @return Pointer to the matching VariantPlacement or nullptr if not found.
 */
const VariantPlacement* EditorOverlay::FindLabyrinthWallVariant(int gx, int gy) const
{
    for (const auto& v : labyrinthWallVariants_)
    {
        if (v.gx == gx && v.gy == gy)
            return &v;
    }
    return nullptr;
}

/**
 * @brief Create or update a labyrinth wall variant placement for a grid cell.
 *
 * If a record already exists, this updates its texture/solid flags and clears
 * any removal marker. Otherwise a new VariantPlacement is appended.
 *
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 * @param textureKey Texture key to store for this wall.
 * @param solid Whether this wall should be treated as solid.
 */
void EditorOverlay::UpsertLabyrinthWallVariant(int gx, int gy, const std::string& textureKey, bool solid, bool isPrefab)
{
    if (VariantPlacement* v = FindLabyrinthWallVariant(gx, gy))
    {
        if (isPrefab) v->type = textureKey;
        v->textureKey = textureKey;
        v->solid = solid;
        v->removed = false;
        return;
    }

    VariantPlacement v;
    if (isPrefab) {
        v.type = textureKey;
    } else {
        v.type = "WallTile";
    }
    v.gx = gx;
    v.gy = gy;
    v.textureKey = textureKey;
    v.solid = solid;
    v.removed = false;
    v.name = "LabyrinthWall_" + std::to_string(gx) + "_" + std::to_string(gy);

    labyrinthWallVariants_.push_back(v);
}

/**
 * @brief Get the current labyrinth wall variant placements.
 * @return Reference to the internal variant list.
 */
const std::vector<VariantPlacement>& EditorOverlay::GetLabyrinthWallVariants() const
{
    return labyrinthWallVariants_;
}

/**
 * @brief Replace the current labyrinth wall variant placements.
 * @param variants New list of variant placements.
 */
void EditorOverlay::SetLabyrinthWallVariants(const std::vector<VariantPlacement>& variants)
{
    labyrinthWallVariants_ = variants;
}

/**
 * @brief Clear all cached labyrinth wall variant placements.
 */
void EditorOverlay::ClearLabyrinthWallVariants()
{
    labyrinthWallVariants_.clear();
}

/**
 * @brief Remove a labyrinth wall entity mapping for a grid cell.
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 */
void EditorOverlay::UnregisterLabyrinthWallEntity(int gx, int gy)
{
    labyrinthWallEntities_.erase(MakeWallCellKey(gx, gy));
}

/**
 * @brief Look up the grid cell for a labyrinth wall entity.
 *
 * This performs a reverse search over the registered mapping.
 *
 * @param e  Entity to locate.
 * @param gx Output grid X coordinate.
 * @param gy Output grid Y coordinate.
 * @return true if the entity was found; false otherwise.
 */
bool EditorOverlay::TryFindLabyrinthWallCell(Entity e, int& gx, int& gy) const
{
    for (const auto& [key, value] : labyrinthWallEntities_)
    {
        if (value == e)
        {
            gx = static_cast<int>(key >> 32);
            gy = static_cast<int>(key & 0xffffffffu);
            return true;
        }
    }
    return false;
}

/**
 * @brief Mark a labyrinth wall cell as removed in the variant list.
 *
 * This records a "removed" variant placement so that persistence can represent
 * explicit deletions even if the base layout still contains a wall at that cell.
 *
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 */
void EditorOverlay::RemoveLabyrinthWallVariant(int gx, int gy)
{
    if (VariantPlacement* v = FindLabyrinthWallVariant(gx, gy))
    {
        v->removed = true;
        v->solid = false;
        v->textureKey.clear();
        return;
    }

    VariantPlacement v;
    v.type = "WallTile";
    v.gx = gx;
    v.gy = gy;
    v.textureKey.clear();
    v.solid = false;
    v.removed = true;
    v.name = "LabyrinthWallRemoved_" + std::to_string(gx) + "_" + std::to_string(gy);

    labyrinthWallVariants_.push_back(v);
}

#endif
