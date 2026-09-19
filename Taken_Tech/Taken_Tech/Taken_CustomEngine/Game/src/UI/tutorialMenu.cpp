/**
 * @file      tutorialMenu.cpp
 * @author    Woh Kye le
 * @email     w.kyele
 * @date      2026-02-19
 * @brief     Implements the TutorialMenu class functionality, including the spawning and logic update of the demo player entity and rendering the control instructions.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "Core/engine.hpp"
#include "Graphics/renderer.h"
#include "Graphics/camera2d.h"
#include "Graphics/meshrenderer.h"
#include "Graphics/spriteanimator.h"
#include "Core/transform.h"
#include "Core/resourceManager.h"
#include "Physics/ForceSystem.hpp"
#include "Core/gameApp.h"
#include "UI/tutorialMenu.h"
#include "playerController.h"
#include "Mechanics/interaction.hpp"

#include <GLFW/glfw3.h>

#include <json.hpp>
#include <fstream>
#include <algorithm>
#include "Core/assetsPath.h"
#include "Math/matrix3x3.h"
#include "Graphics/vertex2d.h"
#include "Font/FontRenderer.h"
#include "Core/loader.h"

extern void SpriteAnimator_Update(SpriteAnimator& an, float dt);


using json = nlohmann::json;

static bool AabbOverlap_(const Vector2& aPos, const Vector2& aSize, const Vector2& bPos, const Vector2& bSize) {
    const float aMinX = aPos.x;
    const float aMaxX = aPos.x + aSize.x;
    const float aMinY = aPos.y;
    const float aMaxY = aPos.y + aSize.y;
    const float bMinX = bPos.x;
    const float bMaxX = bPos.x + bSize.x;
    const float bMinY = bPos.y;
    const float bMaxY = bPos.y + bSize.y;
    return (aMinX < bMaxX) && (aMaxX > bMinX) && (aMinY < bMaxY) && (aMaxY > bMinY);
}

static bool IsWallEntity_(GameApp& app, Entity e) {
    const std::string tag = app.GetPrefabTag(e);
    if (tag == "Wall" || tag == "WallVariant") return true;
    if (auto* obj = app.entityManager.GetEntity(e)) {
        const std::string& n = obj->name;
        if (n.rfind("WallBlock_", 0) == 0) return true;
    }
    return false;
}

static bool CollidesWithAnyWall_(GameApp& app, const Vector2& pos, const Vector2& size) {
    auto& entities = app.GetEntitiesPublic();
    for (Entity e : entities) {
        if (!IsWallEntity_(app, e)) continue;
        if (Transform* t = app.GetTransform(e)) {
            if (AabbOverlap_(pos, size, t->GetPosition(), t->GetScale()))
                return true;
        }
    }
    return false;
}

static bool IsDoorEntity_(GameApp& app, Entity e) {
    const std::string tag = app.GetPrefabTag(e);
    if (tag == "Doorway") return true;
    if (auto* obj = app.entityManager.GetEntity(e)) {
        const std::string& n = obj->name;
        if (n.rfind("Door_", 0) == 0) return true;
    }
    return false;
}

/**
 * @brief Constructs a new TutorialMenu object.
 *
 * Initializes the tutorial menu camera with default dimensions (800x600).
 */
TutorialMenu::TutorialMenu() : ctrlCam(1400.0f, 700.0f) {}

/**
 * @brief Builds the tutorial menu with necessary dependencies.
 *
 * Stores references to the GameApp and sets up the escape and play callbacks.
 * Also initiates the spawning of the demo player.
 *
 * @param gui              Reference to the GuiSystem.
 * @param appRef           Reference to the GameApp instance.
 * @param onEscapeToPause  Callback function to be executed when the escape key is pressed.
 * @param onPlay           Callback function to be executed when the play button is pressed (only on last tutorial level).
 */
void TutorialMenu::Build(GuiSystem& gui, GameApp& appRef, std::function<void()> onEscapeToPause, std::function<void()> onPlay) {
    app = &appRef;
    escapeCallback = std::move(onEscapeToPause);
    playCallback = std::move(onPlay);
    
    ReloadTextures(gui);

    unsigned int buttonTex = ResourceManager::GetTexture("menu_button");

    // Initialize the main textbox panel
    GuiPanel textbox(Vector2(0, 0), Vector2(0, 0), "", textboxTex);
    textbox.id = "tutorial.textbox";
    gui.AddPanel(textbox);

    // Initialize Navigation Buttons once in Build
    // Sizes and positions will be set dynamically in LayoutUI()
    GuiButton prevBtn(Vector2(0, 0), Vector2(0, 0), "Back", [this, &gui]() { PrevTutorial(gui); }, buttonTex);
    prevBtn.id = "tutorial.prev";
    prevBtn.baseFontSize = layout.btnBaseFontSize;
    prevBtn.visible = false; // Hidden by default
    gui.AddButton(prevBtn);

    GuiButton nextBtn(Vector2(0, 0), Vector2(0, 0), "Next", [this, &gui]() { NextTutorial(gui); }, buttonTex);
    nextBtn.id = "tutorial.next";
    nextBtn.baseFontSize = layout.btnBaseFontSize;
    nextBtn.visible = false; // Hidden by default
    gui.AddButton(nextBtn);

    GuiButton playBtn(Vector2(0, 0), Vector2(0, 0), "Play", [this]() { if (playCallback) playCallback(); }, buttonTex);
    playBtn.id = "tutorial.play";
    playBtn.baseFontSize = layout.btnBaseFontSize;
    playBtn.visible = false; // Hidden by default
    gui.AddButton(playBtn);

    SetTutorialStep(TutorialStep::Tut0, gui);

    SpawnDemoPlayer();
}



/**
 * @brief Sets the current tutorial step and updates the UI.
 * 
 * Clears existing scene entities, loads the new step layout from JSON,
 * updates the instruction text, and re-spawns the demo player if needed.
 * 
 * @param step The tutorial step to activate.
 * @param gui  Reference to the GuiSystem.
 */
void TutorialMenu::SetTutorialStep(TutorialStep step, GuiSystem& gui) {
    SaveCarryoverForStepChange(step);

    currentStep = step;
    justChangedStep = true;
    hud.Reset(); // Reset visual interpolation on each step change

    if (step == TutorialStep::Tut2) {
        tut2EnemySpawned = false;
        hasTut2EnemyPos = false;
        lastTut2EnemyPos = { 0.0f, 0.0f };
    }
    
    // Clear existing entities except demo player 
    if (app) {
        app->DestroyAllNonPersistentEntities();
        demoSpawned = false; 
        demoEntity = 0;
    }

    std::string path = GetStepScenePath(step);
    if (path.empty()) return;

    LoadStepSceneAndEntities(path, gui);

    EnsureTextboxPanel(gui);

    // Re-spawn demo player if it was destroyed by LoadScene
    if (app && !demoSpawned) {
        RespawnDemoAndApplyCarryover();
        ApplyCorpsePositionIfTut3();
    }
    // Ensure corpse position is corrected 
    if (app) {
        ApplyCorpsePositionIfTut3();
    }
}

/**
 * @brief Saves carry-over state (player/enemy positions, ability) before a step change.
 */
void TutorialMenu::SaveCarryoverForStepChange(TutorialStep nextStep) {
    if (app && demoEntity != 0) {
        if (auto* t = app->GetTransform(demoEntity)) {
            carriedPlayerPos = t->GetPosition();
            hasCarriedPos = true;
        }
        if (auto* pc = app->GetController(demoEntity)) {
            carriedAbility = pc->GetAbility();
        }
    }
    if (app && currentStep == TutorialStep::Tut2 && nextStep == TutorialStep::Tut3) {
        hasCarriedPos = true;
        if (demoEntity != 0) {
            if (auto* t = app->GetTransform(demoEntity)) {
                carriedPlayerPos = t->GetPosition();
            }
        }
        bool assigned = false;
        if (hasTut2EnemyPos && !(lastTut2EnemyPos.x == 0.0f && lastTut2EnemyPos.y == 0.0f)) {
            carriedMobPos = lastTut2EnemyPos;
            assigned = true;
        }
        if (!assigned) {
            auto& entities = app->GetEntitiesPublic();
            Vector2 foundPos{ 0.0f, 0.0f };
            bool found = false;
            for (Entity e : entities) {
                if (auto* obj = app->entityManager.GetEntity(e)) {
                    if (obj->name == "TutorialEnemy") {
                        if (auto* t = app->GetTransform(e)) {
                            foundPos = t->GetPosition();
                            found = true;
                            break;
                        }
                    }
                }
            }
            if (!found) {
                for (Entity e : entities) {
                    const std::string tag = app->GetPrefabTag(e);
                    if (tag == "ranged_mini_boss" || tag == "EnemyContact" || tag == "burrow_mini_boss" || tag == "heal_mini_boss") {
                        if (auto* t = app->GetTransform(e)) {
                            foundPos = t->GetPosition();
                            found = true;
                            break;
                        }
                    }
                }
            }
            if (found) {
                carriedMobPos = foundPos;
                hasTut2EnemyPos = true;
                lastTut2EnemyPos = foundPos;
                assigned = true;
            }
        }
        if (!assigned) {
            // As a final fallback, keep previous carriedMobPos if it was valid; otherwise keep as-is
            // Do not overwrite with (0,0)
        }
    } else if (app && (int)currentStep >= (int)TutorialStep::Tut0 && (int)currentStep <= (int)TutorialStep::Tut5) {
        if ((int)nextStep > (int)currentStep) {
            hasCarriedPos = true;
            if (demoEntity != 0) {
                if (auto* t = app->GetTransform(demoEntity)) {
                    carriedPlayerPos = t->GetPosition();
                }
                if (auto* pc = app->GetController(demoEntity)) {
                    carriedAbility = pc->GetAbility();
                }
            }
        }
    } else if ((int)nextStep < (int)TutorialStep::Tut0 || (int)nextStep > (int)TutorialStep::Tut7) {
        hasCarriedPos = false;
        carriedAbility = PlayerAbility::DEFAULT;
    }
}

/**
 * @brief Resolves the JSON scene path for a tutorial step.
 */
std::string TutorialMenu::GetStepScenePath(TutorialStep step) const {
    switch (step) {
    case TutorialStep::Tut0: return "scene/tutorial/tut0.json";
    case TutorialStep::Tut1: return "scene/tutorial/tut1.json";
    case TutorialStep::Tut2: return "scene/tutorial/tut2.json";
    case TutorialStep::Tut3: return "scene/tutorial/tut3.json";
    case TutorialStep::Tut4: return "scene/tutorial/tut4.json";
    case TutorialStep::Tut5: return "scene/tutorial/tut5.json";
    case TutorialStep::Tut6: return "scene/tutorial/tut6.json";
    case TutorialStep::Tut7: return "scene/tutorial/tut7.json";
    default: return "";
    }
}

/**
 * @brief Loads layout and scene entities for the given path.
 */
void TutorialMenu::LoadStepSceneAndEntities(const std::string& path, GuiSystem& gui) {
    if (!LoadLayout(path, gui)) {
        layout.currentText = "Tutorial step content missing for " + path;
        return;
    }
    if (app) {
        SceneRuntime::LoadScene(*app, path);
        demoSpawned = false;
        demoEntity = 0;
        auto& entities = app->GetEntitiesPublic();
        DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Tutorial] Loaded ", (int)entities.size(), " entities for step ", (int)currentStep, "\n");
        for (Entity e : entities) {
            if (auto* obj = app->entityManager.GetEntity(e)) {
                DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "  - Entity ", (int)e, ": ", obj->name, "\n");
            }
        }
    }
}

/**
 * @brief Ensures the tutorial textbox panel exists and is updated.
 */
void TutorialMenu::EnsureTextboxPanel(GuiSystem& gui) {
    GuiPanel* panel = gui.FindPanel("tutorial.textbox");
    if (!panel) {
        GuiPanel newPanel(Vector2(0, 0), Vector2(0, 0), layout.currentText, textboxTex);
        newPanel.id = "tutorial.textbox";
        gui.AddPanel(newPanel);
    } else {
        panel->label = layout.currentText;
        panel->texture = textboxTex;
    }
}

/**
 * @brief Spawns demo player if needed and applies carried player state.
 */
void TutorialMenu::RespawnDemoAndApplyCarryover() {
    SpawnDemoPlayer();
    if (hasCarriedPos && (int)currentStep >= (int)TutorialStep::Tut0 && (int)currentStep <= (int)TutorialStep::Tut7 && demoEntity != 0) {
        if (auto* t = app->GetTransform(demoEntity)) {
            t->SetPosition(carriedPlayerPos);
        }
        if ((int)currentStep >= (int)TutorialStep::Tut4) {
            if (auto* pc = app->GetController(demoEntity)) {
                pc->SetAbility(carriedAbility);
                if (pc->GetAbility() == PlayerAbility::DEFAULT) {
                    pc->SetAbility(PlayerAbility::PROJECTILE);
                }
                if (currentStep == TutorialStep::Tut4) {
                    pc->setMutationLevel(10);
                } else if ((int)currentStep >= (int)TutorialStep::Tut5) {
                    pc->setMutationLevel(0);
                } else {
                    pc->setMutationLevel(50);
                }
            }
        }
    }
}

/**
 * @brief Applies carried enemy position to the corpse in Tut3 after scene load.
 */
void TutorialMenu::ApplyCorpsePositionIfTut3() {
    if (hasCarriedPos && currentStep == TutorialStep::Tut3) {
        Entity corpse = 0;
        auto& entities = app->GetEntitiesPublic();
        for (Entity e : entities) {
            if (auto* obj = app->entityManager.GetEntity(e)) {
                if (obj->name == "TutorialEnemyCorpse") {
                    corpse = e;
                    break;
                }
            }
        }
        if (!corpse) {
            for (Entity e : entities) {
                if (app->GetEnemyController(e)) continue; 
                const std::string tag = app->GetPrefabTag(e);
                if (tag == "ranged_mini_boss" || tag == "EnemyContact" || tag == "burrow_mini_boss" || tag == "heal_mini_boss") {
                    corpse = e;
                    break;
                }
            }
        }
        if (corpse) {
            if (auto* t = app->GetTransform(corpse)) {
                t->SetPosition(carriedMobPos);
            }
        }
    }
}
/**
 * @brief Advances to the next tutorial step if available.
 * @param gui Reference to the GuiSystem.
 */
void TutorialMenu::NextTutorial(GuiSystem& gui) {
    int next = (int)currentStep + 1;
    if (next < (int)TutorialStep::Count) {
        SetTutorialStep((TutorialStep)next, gui);
    }
}

/**
 * @brief Returns to the previous tutorial step if available.
 * @param gui Reference to the GuiSystem.
 */
void TutorialMenu::PrevTutorial(GuiSystem& gui) {
    int prev = (int)currentStep - 1;
    if (prev >= 0) {
        SetTutorialStep((TutorialStep)prev, gui);
    }
}

/**
 * @brief Reloads textures used in the tutorial menu.
 * 
 * Imports and assigns textures for the text box, navigation buttons, and HUD elements.
 * 
 * @param gui Reference to the GuiSystem (unused).
 */
void TutorialMenu::ReloadTextures(GuiSystem& gui) {
    (void)gui;
    
    // Force reload texture directly from file to bypass potential resource manager caching issues
    // or incorrect previous loads
    std::string path = AssetPath("textures/Panels/Textbox_UI.png");
    ResourceManager::ImportTexture(path, "Text_box");
    textboxTex = ResourceManager::GetTexture("Text_box");

    std::string btnPath = AssetPath("Textures/UI/menu_button.png");
    ResourceManager::ImportTexture(btnPath, "menu_button");

    // Import HP Bar textures
    std::string hpBgPath = AssetPath("Textures/UI/Empty_HP_Bar.png");
    std::string hpFillPath = AssetPath("Textures/UI/Fill_HP_Bar.png");
    std::string mutBgPath = AssetPath("Textures/UI/Empty_Mutation_Bar.png");
    std::string mutFillPath = AssetPath("Textures/UI/Filling_Mutation_Bar.png");

    ResourceManager::ImportTexture(hpBgPath, "Empty_HP_Bar");
    ResourceManager::ImportTexture(hpFillPath, "Fill_HP_Bar");
    ResourceManager::ImportTexture(mutBgPath, "Empty_Mutation_Bar");
    ResourceManager::ImportTexture(mutFillPath, "Filling_Mutation_Bar");
}

bool TutorialMenu::LoadLayout(const std::string& path, GuiSystem& gui) {
    (void)gui;
    std::ifstream file(AssetPath(path));
    if (!file.is_open()) return false;

    try {
        json j;
        file >> j;

        if (j.contains("textbox")) {
            auto& tb = j["textbox"];
            if (tb.contains("texture")) layout.textboxTexture = tb["texture"];
            if (tb.contains("text")) layout.currentText = tb["text"];

            if (tb.contains("relPos")) {
                layout.textboxRelPos.x = tb["relPos"]["x"];
                layout.textboxRelPos.y = tb["relPos"]["y"];
            }
            if (tb.contains("relSize")) {
                layout.textboxRelSize.x = tb["relSize"]["x"];
                layout.textboxRelSize.y = tb["relSize"]["y"];
            }

            // New separate text configuration
            if (tb.contains("textRelPos")) {
                layout.textRelPos.x = tb["textRelPos"]["x"];
                layout.textRelPos.y = tb["textRelPos"]["y"];
            }
            if (tb.contains("textSize")) {
                layout.textScale = tb["textSize"];
            }
        }

        if (j.contains("buttons") && j["buttons"].is_array()) {
            layout.buttons.clear();
            for (const auto& b : j["buttons"]) {
                if (b.contains("id")) {
                    TutButtonConfig config;
                    config.id = b["id"].get<std::string>();
                    if (b.contains("relPos") && b["relPos"].is_array()) {
                        config.relPos.x = b["relPos"][0];
                        config.relPos.y = b["relPos"][1];
                    }
                    if (b.contains("relSize") && b["relSize"].is_array()) {
                        config.relSize.x = b["relSize"][0];
                        config.relSize.y = b["relSize"][1];
                    }
                    if (b.contains("fontSize")) config.fontSize = b["fontSize"];
                    layout.buttons.push_back(config);
                }
            }
        }
    }
    catch (...) {
        return false;
    }
    return true;
}

/**
 * @brief Hides the tutorial menu.
 *
 * Sets visibility to false and destroys the demo player to clean up resources.
 */
void TutorialMenu::Hide() {
    visible = false;
    DestroyDemoPlayer();
}

/**
 * @brief Destroys the demo player entity.
 * 
 * Removes the demo entity from the application and restores the previous player entity ID.
 */
void TutorialMenu::DestroyDemoPlayer() {
    if (app && demoEntity != 0) {
        app->DestroyEntity(demoEntity);
        demoEntity = 0;
        demoSpawned = false;
    }
    // Restore previous player
    if (app && previousPlayerEntity != 0) {
        GameApp::playerEntity = previousPlayerEntity;
        previousPlayerEntity = 0;
    }
}

/**
 * @brief Spawns the demo player entity.
 *
 * Initializes the force system if needed, saves the current player entity,
 * and instantiates a new "Player" prefab. Since the default prefab might lack
 * components required for the menu demo (Controller, Physics, specific visuals),
 * this function manually adds and configures them.
 */
void TutorialMenu::SpawnDemoPlayer() {
    if (!app || demoSpawned) return;

    // Ensure ForceSystem is initialized 
    InitForceSystem(*app);

    previousPlayerEntity = GameApp::playerEntity;

    Vector2 spawnPos(200.f, 200.f);
    demoEntity = app->InstantiatePrefab("Player", spawnPos);

    if (demoEntity == 0) {
        GameApp::playerEntity = previousPlayerEntity;
        return;
    }

    // Add PlayerController
    if (!app->GetController(demoEntity)) {
        app->AddController(demoEntity);
    }

    // Add Physics (Speed & Mass)
    app->AddPhysicsSpeedComponent(demoEntity, SpeedComponent(demoEntity, Vector2(0.0f, 0.0f)));
    app->AddPhysicsMassComponent(demoEntity, MassComponent(demoEntity, 1.0f));



    // Setup Visuals (Texture & Animator)
    if (auto* mr = app->GetRenderer(demoEntity)) {
        GLuint tex = ResourceManager::GetTexture("player_idle_right");
        if (tex) mr->SetTexture(tex);
    }

    if (!app->GetAnimator(demoEntity)) {
        SpriteAnimator& an = app->AddAnimator(demoEntity);
        if (const SpriteSheet* sh = ResourceManager::GetSpriteSheet("player_idle_right")) {
            an.SetSheet(sh);
            an.SetSpeedFromFrameDuration(sh->frameDuration);
            an.Restart();
        }
    }

    if (PlayerController* ctl = app->GetController(demoEntity)) {
        ctl->setForceProxy(g_entityForceProxy);
    }

    demoSpawned = true;
}

/**
 * @brief Updates the tutorial menu logic.
 *
 * Processes input for the menu (e.g., Escape key). If visible, it ensures the
 * demo player is spawned and updates its components (Controller, Transform,
 * Physics, Animation). It also handles manual physics integration for the
 * demo player since the main game loop's physics system might be paused or
 * separate.
 *
 * @param in Reference to the input system.
 * @param gui Reference to the GuiSystem.
 */
void TutorialMenu::Update(const eng::Input& in, double dt, GuiSystem& gui, Vector2 viewportPos, Vector2 viewportSize) {
    if (!visible) return;

    LayoutUI(gui);

    // update the dialog widgets
    gui.Update(in, dt, viewportPos, viewportSize);

    if (in.isKeyPressed(GLFW_KEY_ESCAPE)) {
        if (escapeCallback) {
            escapeCallback();
        }
        return;
    }

    if (in.isKeyPressed(GLFW_KEY_RIGHT)) {
        NextTutorial(gui);
    }
    if (in.isKeyPressed(GLFW_KEY_LEFT)) {
        PrevTutorial(gui);
    }

    if (!demoSpawned) {
        SpawnDemoPlayer();
    }

    // Update Camera aspect ratio and viewport to match gameplay (1400x700)
    int w, h;
    glfwGetWindowSize(glfwGetCurrentContext(), &w, &h);
    if (h > 0) {
        ctrlCam.SetOrtho(0.f, 1400.0f, 0.f, 700.0f);
        ctrlCam.SetViewport(0, 0, w, h);
    }

    if (demoEntity != 0) {
        if (auto* ctrl = app->GetController(demoEntity)) {
            if (auto* tr = app->GetTransform(demoEntity)) {

                bool suppressAttackInput = false;
                if (in.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) || in.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                    for (const auto& b : gui.GetButtons()) {
                        if (!b.visible) continue;
                        if (b.pressed || b.hovered) {
                            suppressAttackInput = true;
                            break;
                        }
                    }
                }
                ctrl->SetAttackInputSuppressed(suppressAttackInput);

                // Bind render dependencies so animation works
                MeshRenderer* mr = app->GetRenderer(demoEntity);
                SpriteAnimator* an = app->GetAnimator(demoEntity);
                if (mr && an) {
                    ctrl->BindRenderDeps(mr, an);
                }

                // Update controller logic
                if (GameApp::playerEntity != demoEntity) {
                     GameApp::playerEntity = demoEntity;
                }

                //float dt = (float)eng::deltaTime();
                ctrl->Update(static_cast<float>(dt), *tr, app->GetCollider(demoEntity));

                // Handle interactions (Absorption, Skills) in the tutorial
                Interaction::HandlePlayerAbilityInput(*app, demoEntity, dt);

                // Handle mutation charging
                if (currentStep == TutorialStep::Tut4) {
                    Interaction::HandleMutationChargeUp(*app, demoEntity, static_cast<float>(dt));
                    Interaction::HandleMutationDamage(*app, demoEntity, static_cast<float>(dt));
                } else if ((int)currentStep >= (int)TutorialStep::Tut5) {
                    if (auto* pc = app->GetController(demoEntity)) {
                        pc->setMutationLevel(0);
                    }
                }

                // Update animation/visual state (ApplyState)
                ctrl->Draw(*tr);

                // Update physics speed based on controller input
                ctrl->GetPhysicsSpeed(ctrl->getPlayerVel());

                if (SpeedComponent* spd = app->AddSpeedComponent(demoEntity)) {
                    float maxSpeed = 200.0f;
                    if (spd->speed.Length() > maxSpeed) {
                        spd->speed = spd->speed.Normalized() * maxSpeed;
                    }
                    Vector2 pos = tr->GetPosition();
                    Vector2 size = tr->GetScale();
                    Vector2 delta = spd->speed * static_cast<float>(eng::deltaTime());
                    Vector2 tryPosX = pos; tryPosX.x += delta.x;
                    if (!CollidesWithAnyWall_(*app, tryPosX, size)) pos.x = tryPosX.x;
                    Vector2 tryPosY = pos; tryPosY.y += delta.y;
                    if (!CollidesWithAnyWall_(*app, tryPosY, size)) pos.y = tryPosY.y;
                    if (pos.x < 0) pos.x = 0;
                    if (pos.x > 1400.0f) pos.x = 1400.0f;
                    if (pos.y < 0) pos.y = 0;
                    if (pos.y > 700.0f) pos.y = 700.0f;
                    tr->SetPosition(pos);
                }
            }
        }
    }

    // Door transitions between Tut6 and Tut7 when overlapping a door prefab
    if (app && demoEntity != 0 && (currentStep == TutorialStep::Tut6 || currentStep == TutorialStep::Tut7) && !justChangedStep) {
        if (Transform* pt = app->GetTransform(demoEntity)) {
            Vector2 pPos = pt->GetPosition();
            Vector2 pSize = pt->GetScale();
            auto& entities = app->GetEntitiesPublic();
            for (Entity e : entities) {
                if (!IsDoorEntity_(*app, e)) continue;
                if (Transform* doorTransform = app->GetTransform(e)) {
                    if (AabbOverlap_(pPos, pSize, doorTransform->GetPosition(), doorTransform->GetScale())) {
                        if (currentStep == TutorialStep::Tut6) {
                            NextTutorial(gui);
                            return;
                        } else {
                            PrevTutorial(gui);
                            return;
                        }
                    }
                }
            }
        }
    }

    // Update enemies AI and shooting for the tutorial scene
    if (app) {
        float frameDt = static_cast<float>(eng::deltaTime());
        app->TickAIAndShooting(frameDt);
        app->TickProjectiles(frameDt);
        app->TickScripts(frameDt);
        app->TickAnimations(frameDt);
        HandleAutoTransitions(gui);
    }
    justChangedStep = false;
}

/**
 * @brief Handles auto transitions for Tut2 and Tut3 based on gameplay events.
 * @param gui GUI system for invoking next/prev transitions.
 */
void TutorialMenu::HandleAutoTransitions(GuiSystem& gui) {
    if (!app) return;

    if (currentStep == TutorialStep::Tut2 && !justChangedStep) {
        bool foundEnemyController = false;
        auto& entities = app->GetEntitiesPublic();
        for (Entity e : entities) {
            if (app->GetEnemyController(e)) {
                foundEnemyController = true;
                break;
            }
        }

        if (foundEnemyController) {
            if (!tut2EnemySpawned) {
                DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Tutorial] Found enemy controller in Tut2\n");
                tut2EnemySpawned = true;
            }
            auto& entities2 = app->GetEntitiesPublic();
            for (Entity e : entities2) {
                if (auto* obj = app->entityManager.GetEntity(e)) {
                    if (obj->name == "TutorialEnemy") {
                        if (auto* t = app->GetTransform(e)) {
                            lastTut2EnemyPos = t->GetPosition();
                            hasTut2EnemyPos = true;
                            break;
                        }
                    }
                }
            }
        }

        if (tut2EnemySpawned && !foundEnemyController) {
            bool meleePlaying = false;
            if (demoEntity != 0) {
                if (auto* pc = app->GetController(demoEntity)) {
                    meleePlaying = pc->IsMeleeAnimating();
                }
            }
            if (meleePlaying) {
                // Defer transition until animation completes
            } else {
                DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Tutorial] Enemy controller lost, transitioning to Tut3\n");
                NextTutorial(gui);
                return;
            }
        }
    }

    if (currentStep == TutorialStep::Tut3 && !justChangedStep) {
        if (demoEntity != 0) {
            if (auto* pc = app->GetController(demoEntity)) {
                if (pc->GetAbility() != PlayerAbility::DEFAULT) {
                    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Tutorial] Player absorbed enemy in Tut3, transitioning to Tut4\n");
                    NextTutorial(gui);
                    return;
                }
            }
        }
    }
}

/**
 * @brief Draws the tutorial menu and its components.
 *
 * Renders the demo player using a dedicated camera and shader setup to ensure
 * it appears correctly over the UI. It handles sprite animation uniforms and
 * restores the previous camera state after rendering. Finally, it draws the GUI.
 *
 * @param renderer Reference to the Renderer.
 * @param gui      Reference to the GuiSystem.
 */
void TutorialMenu::Draw(Renderer& renderer, GuiSystem& gui) {
    if (!visible) return;

    if (app) {
        Camera2D* oldCam = renderer.getCamera();
        renderer.setCamera(&ctrlCam);

        // Render all entities in the current tutorial scene
        for (Entity e : app->GetEntitiesPublic()) {
            MeshRenderer* mr = app->GetRenderer(e);
            if (!mr) continue;

            Mesh2D* mesh = mr->GetMesh();
            if (!mesh) continue;

            if (auto* tr = app->GetTransform(e)) {
                // Set shader uniforms for animation if it's the demo entity or has an animator
                Shader* sh = renderer.GetShader();
                if (sh) {
                    sh->Use();
                    SpriteAnimator* an = app->GetAnimator(e);
                    GLuint texId = 0;

                    if (an && an->sheet) {
                        sh->SetInt("u_Frame", an->GetFrame());
                        sh->SetInt("u_Cols", an->GetCols());
                        sh->SetVec2("u_FrameSize", Vector2(an->FrameU(), an->FrameV()));
                        texId = an->sheet->texture.ID();
                    }
                    else {
                        sh->SetInt("u_Frame", 0);
                        sh->SetInt("u_Cols", 1);
                        sh->SetVec2("u_FrameSize", Vector2(1.f, 1.f));
                        texId = mr->GetTexture();
                    }

                    renderer.DrawMesh(
                        *mesh,
                        tr->GetModelMatrix(),
                        mr->GetColor(),
                        texId
                    );
                }
            }
        }

        renderer.setCamera(oldCam);
    }

    // Draw GUI in screen space
    Camera2D* prevCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    // Save previous shader and force default (2D) shader to avoid attribute mismatch
    Shader* prevShader = renderer.GetShader();
    Shader* defaultShader = renderer.GetDefaultShader();
    renderer.SetShader(defaultShader);

    // Reset sprite animation uniforms to default for GUI rendering
    if (defaultShader) {
        defaultShader->Use();
        defaultShader->SetInt("u_Frame", 0);
        defaultShader->SetInt("u_Cols", 1);
        defaultShader->SetVec2("u_FrameSize", Vector2(1.0f, 1.0f));
    }

    // Disable depth test to ensure UI draws on top
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);

    // Enable blending for UI transparency
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    if (!blendEnabled) glEnable(GL_BLEND);
    GLint srcAlpha, dstAlpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    LayoutUI(gui);
    gui.Draw(renderer);

    // Restore blending state
    if (!blendEnabled) glDisable(GL_BLEND);
    glBlendFunc(srcAlpha, dstAlpha);

    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }

    if (app && currentStep >= TutorialStep::Tut1) {
        int w, h;
        glfwGetFramebufferSize(glfwGetCurrentContext(), &w, &h);
        float uiScale = (float)h / 1080.0f;

        // Calculate position directly below the panel in reference coordinates
        // textboxRelPos.y is center, so bottom edge in screen pixels (bottom=0) is:
        float bottomEdge = (layout.textboxRelPos.y * (float)h) - (layout.textboxRelSize.y * (float)h * 0.5f);
        // Distance from top (0) in reference-pixel-top-down
        float posY = ((float)h - bottomEdge) / uiScale + 10.0f;

        // Healthbar appears from Tut1 onwards
        hud.DrawHealthBar(renderer, *app, Vector2(120.0f, posY));

        // Mutation bar appears from Tut4 onwards
        if (currentStep >= TutorialStep::Tut4) {
            Vector2 mutSize = hud.GetMutationSize();
            float virtualWidth = (float)w / uiScale;
            hud.DrawMutationBar(renderer, *app, Vector2(virtualWidth - mutSize.x - 20.0f, posY));
        }

        // Ability Inventory appears from Tut5 onwards
        if (currentStep >= TutorialStep::Tut5) {
            // Draw below the health bar
            hud.DrawAbilityInventory(renderer, *app, Vector2(120.0f, posY + 50.0f));
        }
    }

    // Restore state
    renderer.SetShader(prevShader);
    renderer.setCamera(prevCam);
}

void TutorialMenu::LayoutUI(GuiSystem& gui) {
    int w, h;
    glfwGetWindowSize(glfwGetCurrentContext(), &w, &h);

    // Update Textbox Panel (Center Top)
    float targetW = layout.textboxRelSize.x * w;
    float targetH = layout.textboxRelSize.y * h;
    
    // Position based on relative X in full window, then center the panel on that point
    float targetX = (layout.textboxRelPos.x * w) - (targetW * 0.5f);
    float targetY = (layout.textboxRelPos.y * h) - (targetH * 0.5f);

    GuiPanel* panel = gui.FindPanel("tutorial.textbox");
    if (panel) {
        panel->visible = true;
        panel->texture = textboxTex;
        panel->label = layout.currentText;
        panel->baseFontSize = layout.textScale;
        panel->fontSize = layout.textScale;
        panel->pos = Vector2(targetX, targetY);
        panel->size = Vector2(targetW, targetH);
    }

    // Hide all navigation buttons first
    if (GuiButton* b = gui.FindButton("tutorial.prev")) b->visible = false;
    if (GuiButton* b = gui.FindButton("tutorial.next")) b->visible = false;
    if (GuiButton* b = gui.FindButton("tutorial.play")) b->visible = false;

    for (const auto& config : layout.buttons) {
        GuiButton* btn = gui.FindButton("tutorial." + config.id);
        if (btn) {
            // Special visibility logic based on step
            if (config.id == "prev") btn->visible = (currentStep != TutorialStep::Tut0);
            else if (config.id == "next") btn->visible = (currentStep != TutorialStep::Tut7);
            else if (config.id == "play") btn->visible = (currentStep == TutorialStep::Tut7);
            else btn->visible = true;

            if (btn->visible) {
                btn->size = Vector2(config.relSize.x * w, config.relSize.y * h);
                btn->pos = Vector2((config.relPos.x * w) - (btn->size.x * 0.5f), config.relPos.y * h);
                
                btn->baseFontSize = config.fontSize;
                btn->fontSize = config.fontSize;
            }
        }
    }
}
