/**
 * @file      playerHUD.cpp
 * @author    Woh kye Le
 * @co-author Tan Wei Liang Terril, Jethro Sung
 * @email     w.kyele,t.weiliangterril,sung.h
 * @date      2026-02-20
 *
 * @brief     Implementation of the PlayerHUD class.
 *            This file handles the logic for initializing and drawing the
 *            player's health bar on the screen.
 *
 * @version   1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "UI/playerHUD.h"
#include "Core/gameApp.h"
#include "Graphics/renderer.h"
#include "Core/utils.h"
#include "Core/componentcontext.h"
#include "playerController.h"
#include "Core/resourceManager.h"
#include "Mechanics/interaction.hpp"
#include "Font/FontRenderer.h"
#include "UI/proximityPromptText.h"

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>

 /**
  * @brief Initializes the HUD resources.
  *
  * This creates the meshes (shapes) for the health bar and loads
  * the images (textures) used for the empty and full health bar.
  */
void PlayerHUD::Init() {

    std::vector<Vector2> pos = { Vector2(0,0), Vector2(1,0), Vector2(1,1), Vector2(0,1) };
    std::vector<Vector3> col = { Vector3(1,1,1), Vector3(1,1,1), Vector3(1,1,1), Vector3(1,1,1) };

    // Create the meshes for both the background frame and the fill bar
    quadMesh = Utility::BuildQuad(pos, col);
    foregroundMesh = Utility::BuildQuad(pos, col);

    // Load health bar textures
    bgTextureID = ResourceManager::GetTexture("Empty_HP_Bar");
    fgTextureID = ResourceManager::GetTexture("Fill_HP_Bar");

    bgMutationTextureID = ResourceManager::GetTexture("Empty_Mutation_Bar");
    fgMutationTextureID = ResourceManager::GetTexture("Filling_Mutation_Bar");

    abilityInventoryTextureID = ResourceManager::GetTexture("abilityInventory");
    burrowIconTextureID = ResourceManager::GetTexture("burrowIcon");
    healingIconTextureID = ResourceManager::GetTexture("healingIcon");
    projectileIconTextureID = ResourceManager::GetTexture("projectileIcon");

    keyItemTextureID = ResourceManager::GetTexture("crate");
    thinkingBubbleTextureIDL = ResourceManager::GetTexture("thinkingBubble_L");
    thinkingBubbleTextureIDR = ResourceManager::GetTexture("thinkingBubble_R");

    initialized = true;
}

/**
 * @brief Resets the HUD state.
 * 
 * Resets smoothed values like the mutation bar percentage to their initial state.
 */
void PlayerHUD::Reset() {
    smoothMutationPercent = 0.0f;
}

/**
 * @brief Draws the health bar on the screen.
 *
 * This function does the following:
 * 1. Checks the player's current health.
 * 2. Calculates how much of the bar should be filled.
 * 3. Switches to a 2D camera mode to draw on top of the game world.
 * 4. Draws the background (empty frame) and then the foreground (fill).
 *
 * @param renderer The system responsible for drawing graphics.
 * @param app      The main game application (to get player info).
 */
void PlayerHUD::Draw(Renderer& renderer, GameApp& app) {
    if (!initialized) Init();

    DrawInteractionHints(renderer, app);

    // 1. Draw Health Bar at default position (top-left)
    DrawHealthBar(renderer, app, position);

    // 2. Draw Mutation Bar at top-right
    HUDContext ctxForMut = PrepareHUD(renderer);
    if (ctxForMut.valid) {
        float virtualWidth = (float)ctxForMut.fbw / ctxForMut.guiScale;
        Vector2 mutPos(virtualWidth - mutationSize.x - 20.0f, 20.0f);
        DrawMutationBar(renderer, app, mutPos);
    }

    // 2.5 Draw Ability Inventory right below health bar
    // Health bar is at 'position' (20, 20), its height is 'size.y' (40)
    // We add some padding (e.g. 10 units)
    Vector2 invPos(position.x, position.y + size.y + 10.0f);
    DrawAbilityInventory(renderer, app, invPos);

    // 3. Draw Inventory (Keys)
    HUDContext ctx = PrepareHUD(renderer);
    if (!ctx.valid) {
        renderer.setCamera(ctx.prevCam);
        return;
    }

    Entity player = app.GetPlayerEntity();
    if (player == INVALID_ENTITY) {
        renderer.setCamera(ctx.prevCam);
        return;
    }
    PlayerController* pc = app.GetController(player);
    if (!pc) {
        renderer.setCamera(ctx.prevCam);
        return;
    }

    int keyCount = pc->GetKeyCount();
    float boxSize = 60.0f * ctx.guiScale;
    float padding = 10.0f * ctx.guiScale;
    float startX = 20.0f * ctx.guiScale;
    // Position below ability inventory: position.y + size.y + padding + abilityInventorySize.y + padding
    float startY = (position.y + size.y + 10.0f + abilityInventorySize.y + 10.0f) * ctx.guiScale; 

    for (int i = 0; i < 3; ++i) {
        float bx = startX + (boxSize + padding) * i;
        float by = startY;

        Matrix3x3 modelBox = Matrix3x3::BuildTranslation(bx, by) * Matrix3x3::BuildScaling(boxSize, boxSize);
        renderer.DrawMesh(quadMesh, ctx.ortho * modelBox, Vector3(0.2f, 0.2f, 0.2f), 0);

        if (i < keyCount) {
             PlayerAbility keyType = pc->GetKeyAt(i);
             Vector3 keyColor = { 1.0f, 1.0f, 1.0f };
             GLuint currentKeyTex = 0;
             if (keyType == PlayerAbility::BURROW) currentKeyTex = ResourceManager::GetTexture("BlackKey");
             else if (keyType == PlayerAbility::PROJECTILE) currentKeyTex = ResourceManager::GetTexture("RedKey");
             else if (keyType == PlayerAbility::HEAL) currentKeyTex = ResourceManager::GetTexture("GreenKey");
             else currentKeyTex = ResourceManager::GetTexture("YellowKey");

             float keySize = boxSize * 0.8f;
             float keyOffset = (boxSize - keySize) * 0.5f;
             Matrix3x3 modelKey = Matrix3x3::BuildTranslation(bx + keyOffset, by + keyOffset) * Matrix3x3::BuildScaling(keySize, keySize);
             renderer.DrawMesh(quadMesh, ctx.ortho * modelKey, keyColor, currentKeyTex); 
        }
    }
    renderer.setCamera(ctx.prevCam);
}

/**
 * @brief Draws the thinking bubble above the player and a bottom prompt at screen bottom when near supported entities.
 * @param renderer The renderer used to issue mesh and text draw calls.
 * @param app The game application used to access player state and world entities.
 */
void PlayerHUD::DrawInteractionHints(Renderer& renderer, GameApp& app) {
    Entity player = app.GetPlayerEntity();
    if (player == INVALID_ENTITY) return;
    PlayerController* pc = app.GetController(player);
    Transform* pt = app.GetTransform(player);
    if (!pc || !pt) return;

    const Vector2 playerPos = pt->GetPosition();
    Vector2 playerSize = pt->GetScale();
   
    UI::ProximityPromptResult prompt = UI::GetActivePromptForPlayer(app, player);
    if (!prompt.show) return;

    const bool facingRight = pc->IsFacingRight();
    Vector2 bubbleSize{40.0f, 30.0f};
    float padX = 8.0f;
    float padY = 8.0f;

    float offX = facingRight
        ? (playerPos.x - playerSize.x / 2 - padX)
        : (playerPos.x + playerSize.x / 2 + padX);
    float offY = playerPos.y + playerSize.y + padY;           

    Matrix3x3 modelBubble =
        Matrix3x3::BuildTranslation(offX, offY) *
        Matrix3x3::BuildScaling(bubbleSize.x, bubbleSize.y);

    unsigned int bubTex = facingRight ? thinkingBubbleTextureIDR : thinkingBubbleTextureIDL;
    renderer.DrawMesh(quadMesh, modelBubble, Vector3(1, 1, 1), bubTex);

    HUDContext ctx = PrepareHUD(renderer);
    if (!ctx.valid) {
        renderer.setCamera(ctx.prevCam);
        return;
    }
    float x = (float)ctx.fbw * 0.5f;
    float y = 80.0f * ctx.guiScale;
    float fontSize = (prompt.sizePx > 0.0f ? prompt.sizePx : 40.0f) * ctx.guiScale;
    if (!prompt.text.empty()) {
        EngineCore::FontRenderer::DrawText(
            renderer,
            prompt.font.empty() ? std::string("default") : prompt.font,
            fontSize,
            x,
            y,
            prompt.rgba == 0 ? 0xFFFFFFFF : prompt.rgba,
            prompt.text,
            0.0f,
            FontSys::Align::Center
        );
    }
    renderer.setCamera(ctx.prevCam);
}

/**
 * @brief Draws the ability inventory portion of the HUD.
 * 
 * @param renderer The renderer system.
 * @param app      The game application.
 * @param pos      The screen position to draw at.
 */
void PlayerHUD::DrawAbilityInventory(Renderer& renderer, GameApp& app, Vector2 pos) {
    if (!initialized) Init();
    
    Entity player = app.GetPlayerEntity();
    if (player == INVALID_ENTITY) return;
    PlayerController* pc = app.GetController(player);
    if (!pc) return;

    HUDContext ctx = PrepareHUD(renderer);
    if (!ctx.valid) {
        renderer.setCamera(ctx.prevCam);
        return;
    }

    float x = pos.x * ctx.guiScale;
    float scaledSizeX = abilityInventorySize.x * ctx.guiScale;
    float scaledSizeY = abilityInventorySize.y * ctx.guiScale;
    
    // Position is from top-left, so we subtract from fbh
    float y = (float)ctx.fbh - (pos.y * ctx.guiScale) - scaledSizeY;

    // 1. Draw background
    Matrix3x3 model = Matrix3x3::BuildTranslation(x, y) * Matrix3x3::BuildScaling(scaledSizeX, scaledSizeY);
    renderer.DrawMesh(quadMesh, ctx.ortho * model, Vector3(1.0f, 1.0f, 1.0f), abilityInventoryTextureID);

    // 2. Draw current ability icon
    PlayerAbility currentAbility = pc->GetAbility();
    unsigned int iconTex = 0;
    unsigned int emptyTex = 0;
    switch (currentAbility) {
    case PlayerAbility::BURROW:
        iconTex = burrowIconTextureID;
        break;
    case PlayerAbility::HEAL:
        iconTex = healingIconTextureID;
        break;
    case PlayerAbility::PROJECTILE:
        iconTex = projectileIconTextureID;
        break;
    default:
        break;
    }

    if (iconTex != 0) {
        float iconScale = 0.8f;
        float scaledIconSizeX = scaledSizeX * iconScale;
        float scaledIconSizeY = scaledSizeY * iconScale;
        float iconOffsetX = (scaledSizeX - scaledIconSizeX) * 0.5f;
        float iconOffsetY = (scaledSizeY - scaledIconSizeY) * 0.5f;

        Matrix3x3 modelIcon = Matrix3x3::BuildTranslation(x + iconOffsetX, y + iconOffsetY) * Matrix3x3::BuildScaling(scaledIconSizeX, scaledIconSizeY);
        renderer.DrawMesh(quadMesh, ctx.ortho * modelIcon, Vector3(1.0f, 1.0f, 1.0f), iconTex);

        /// Cooldown implementation for testing:
        float cdRatio = Interaction::GetAbilityCooldownRatio(player, currentAbility);
        if (cdRatio > 0.0f) {
            float overlayHeight = scaledIconSizeY * cdRatio;
            float overlayY = y + iconOffsetY;
            float overlayTop = overlayY + (scaledIconSizeY - overlayHeight);

            /*float intensity = 1.0f - (cdRatio * 0.6f);

            Vector3 overlayColor = Vector3(intensity, intensity, intensity);*/

            (void)cdRatio; // 1 = full cooldown (black), 0 = ready (transparent)
            Vector3 overlayColor = Vector3(0.0f, 0.0f, 0.0f); // color black

            // If your shader supports alpha, you can use it like this:
            float alpha = cdRatio * 0.6f; // 0.6 max opacity
            overlayColor *= alpha;

            Matrix3x3 overlay = Matrix3x3::BuildTranslation(x + iconOffsetX, overlayTop) * Matrix3x3::BuildScaling(scaledIconSizeX, overlayHeight);
            renderer.DrawMesh(quadMesh, ctx.ortho * overlay, overlayColor, emptyTex);
        }
    }

    renderer.setCamera(ctx.prevCam);
}

HUDContext PlayerHUD::PrepareHUD(Renderer& renderer) {
    HUDContext ctx;
    ctx.valid = false;
    ctx.prevCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    ctx.fbw = 0; ctx.fbh = 0;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &ctx.fbw, &ctx.fbh);
    if (ctx.fbw <= 0 || ctx.fbh <= 0) {
        return ctx;
    }

    float l = 0.0f, r = (float)ctx.fbw;
    float b = 0.0f, t = (float)ctx.fbh;
    
    ctx.guiScale = (float)ctx.fbh / 1080.0f;

    ctx.ortho = Matrix3x3::Identity();
    ctx.ortho(0, 0) = 2.0f / (r - l);
    ctx.ortho(1, 1) = 2.0f / (t - b);
    ctx.ortho(2, 0) = -(r + l) / (r - l);
    ctx.ortho(2, 1) = -(t + b) / (t - b);

    Shader* sh = renderer.GetShader();
    if (sh) {
        sh->Use();
        sh->SetInt("u_Frame", 0);
        sh->SetInt("u_Cols", 1);
        sh->SetVec2("u_FrameSize", Vector2(1.f, 1.f));
    }

    ctx.valid = true;
    return ctx;
}

/**
 * @brief Draws the health bar portion of the HUD.
 * 
 * Handles the specific rendering logic for the health bar, including
 * scaling based on current health and screen resolution.
 * 
 * @param renderer The renderer system.
 * @param app      The game application.
 * @param pos      The screen position to draw at.
 */
void PlayerHUD::DrawHealthBar(Renderer& renderer, GameApp& app, Vector2 pos) {
    if (!initialized) Init();
    Entity player = app.GetPlayerEntity();
    if (player == INVALID_ENTITY) return;
    PlayerController* pc = app.GetController(player);
    if (!pc) return;

    int currentHP = pc->getPlayerHp();
    int maxHP = 100;
    float hpPercent = (float)currentHP / (float)maxHP;
    hpPercent = std::clamp(hpPercent, 0.0f, 1.0f);

    std::vector<Vertex2D> fgVerts(4);
    fgVerts[0] = Vertex2D(Vector2(0, 0), Vector2(0, 0), Vector3(1, 1, 1));
    fgVerts[1] = Vertex2D(Vector2(hpPercent, 0), Vector2(hpPercent, 0), Vector3(1, 1, 1));
    fgVerts[2] = Vertex2D(Vector2(hpPercent, 1), Vector2(hpPercent, 1), Vector3(1, 1, 1));
    fgVerts[3] = Vertex2D(Vector2(0, 1), Vector2(0, 1), Vector3(1, 1, 1));
    foregroundMesh.SetVertices(fgVerts);

    HUDContext ctx = PrepareHUD(renderer);
    if (!ctx.valid) {
        renderer.setCamera(ctx.prevCam);
        return;
    }

    float x = pos.x * ctx.guiScale;
    float scaledSizeX = size.x * ctx.guiScale;
    float scaledSizeY = size.y * ctx.guiScale;
    float y = (float)ctx.fbh - (pos.y * ctx.guiScale) - scaledSizeY;

    Matrix3x3 modelBG = Matrix3x3::BuildTranslation(x, y) * Matrix3x3::BuildScaling(scaledSizeX, scaledSizeY);
    renderer.DrawMesh(quadMesh, ctx.ortho * modelBG, bgTextureID ? Vector3(1,1,1) : Vector3(0.2f,0.2f,0.2f), bgTextureID);

    float xFillOffset = 80.0f * ctx.guiScale; 
    float xFill = x + xFillOffset;
    float scaledFillSizeX = (size.x - 80.0f) * ctx.guiScale;
    Matrix3x3 modelFG = Matrix3x3::BuildTranslation(xFill, y) * Matrix3x3::BuildScaling(scaledFillSizeX, scaledSizeY);
    Vector3 fgColor(0.0f, 1.0f, 0.0f);
    if (hpPercent < 0.5f) fgColor = Vector3(1.0f, 1.0f, 0.0f);
    if (hpPercent < 0.2f) fgColor = Vector3(1.0f, 0.0f, 0.0f);
    if (fgTextureID != 0) fgColor = Vector3(1.0f, 1.0f, 1.0f);
    renderer.DrawMesh(foregroundMesh, ctx.ortho * modelFG, fgColor, fgTextureID);

    renderer.setCamera(ctx.prevCam);
}

/**
 * @brief Draws the mutation bar portion of the HUD.
 * 
 * Handles the rendering and smooth interpolation of the mutation bar
 * based on the player's current mutation level.
 * 
 * @param renderer The renderer system.
 * @param app      The game application.
 * @param pos      The screen position to draw at.
 */
void PlayerHUD::DrawMutationBar(Renderer& renderer, GameApp& app, Vector2 pos) {
    if (!initialized) Init();
    Entity player = app.GetPlayerEntity();
    if (player == INVALID_ENTITY) return;
    PlayerController* pc = app.GetController(player);
    if (!pc) return;

    int mutationlv = pc->getMutationLevel();
    int maxMutationlevel = 100;
    float targetMutationPercent = std::clamp((float)mutationlv / (float)maxMutationlevel, 0.0f, 1.0f);

    float dt = (float)eng::deltaTime();
    float lerpSpeed = 5.0f;
    smoothMutationPercent += (targetMutationPercent - smoothMutationPercent) * (1.0f - pow(0.1f, dt * lerpSpeed));
    float mutationPercent = smoothMutationPercent;

    std::vector<Vertex2D> mutationVerts(4);
    mutationVerts[0] = Vertex2D(Vector2(0, 0), Vector2(0, 0), Vector3(1, 1, 1));
    mutationVerts[1] = Vertex2D(Vector2(mutationPercent, 0), Vector2(mutationPercent, 0), Vector3(1, 1, 1));
    mutationVerts[2] = Vertex2D(Vector2(mutationPercent, 1), Vector2(mutationPercent, 1), Vector3(1, 1, 1));
    mutationVerts[3] = Vertex2D(Vector2(0, 1), Vector2(0, 1), Vector3(1, 1, 1));
    foregroundMesh.SetVertices(mutationVerts);

    HUDContext ctx = PrepareHUD(renderer);
    if (!ctx.valid) {
        renderer.setCamera(ctx.prevCam);
        return;
    }

    float scaledMutSizeX = mutationSize.x * ctx.guiScale;
    float scaledMutSizeY = mutationSize.y * ctx.guiScale;
    float mutX = pos.x * ctx.guiScale;
    float mutY = (float)ctx.fbh - (pos.y * ctx.guiScale) - scaledMutSizeY;

    Matrix3x3 modelMutBG = Matrix3x3::BuildTranslation(mutX, mutY) * Matrix3x3::BuildScaling(scaledMutSizeX, scaledMutSizeY);
    renderer.DrawMesh(quadMesh, ctx.ortho * modelMutBG, bgMutationTextureID ? Vector3(1,1,1) : Vector3(0.15f,0.05f,0.2f), bgMutationTextureID);

    float mutFillOffsetX = 170.0f * ctx.guiScale; 
    float scaledMutFillSizeX = scaledMutSizeX - (mutFillOffsetX * 2.0f);
    float scaledMutFillSizeY = scaledMutSizeY * 0.25f;
    float mutXFill = mutX + mutFillOffsetX;
    // Y axis is centralised (filler is centrally of the mutation bar for y axis+ 6 units upwards shifting)
    float mutYFill = mutY + (scaledMutSizeY - scaledMutFillSizeY) * 0.5f + (6.0f * ctx.guiScale); // 

    Matrix3x3 modelMutFG = Matrix3x3::BuildTranslation(mutXFill, mutYFill) * Matrix3x3::BuildScaling(scaledMutFillSizeX, scaledMutFillSizeY);
    Vector3 mutColor(0.7f, 0.0f, 1.0f);
    if (fgMutationTextureID != 0) mutColor = Vector3(1.0f, 1.0f, 1.0f);
    renderer.DrawMesh(foregroundMesh, ctx.ortho * modelMutFG, mutColor, fgMutationTextureID);

    renderer.setCamera(ctx.prevCam);
}
