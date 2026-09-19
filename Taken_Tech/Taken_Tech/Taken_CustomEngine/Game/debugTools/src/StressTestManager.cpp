#pragma once
/**
* @file     StressTestManager.cpp
* @author   Woh Kye Le
* @email    w.kyele,t.weiliangterril
* @co-author Tan Wei Liang Terril
* @date     2025-11-01
*
* @brief This source file implements function definition to header file StressTestManager 
*
* This file defines the StressTestManager responsible for creating,
* updating, and rendering instanced 2D objects using OpenGL instancing
* 
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "StressTestManager.h"
#include "Core/resourceManager.h"
#include "Core/gameApp.h"
#include "Graphics/camera2d.h"
#include <cstdlib>  //rand
#include "Input/DebugConsole.hpp"

/**
 * @brief Destroys the StressTestManager and releases GPU/CPU resources.
 *
 * Deletes the instance vertex buffer object (VBO), clears simulation data,
 * and resets all associated pointers to prevent dangling references.
 */
StressTestManager::~StressTestManager() {
    if (instanceVBO) {
        glDeleteBuffers(1, &instanceVBO);
        instanceVBO = 0;
    }

    stressMesh = nullptr;
    shaderInstanced = nullptr;
    stressTex = 0;

    sims.clear();
    gpuBufferCPU.clear();

    DebugConsole::Get().Warning("[StressTest] Manager destroyed, resources released.");
}


/**
 * @brief Initializes the stress test simulation and GPU buffers.
 *
 * Sets up the instanced mesh, shader, and shared texture, defines the world
 * boundaries, and creates simulation data for all instances, Called before entering stress mode.
 *
 * @param app             Reference to the active GameApp instance.
 * @param instancedShader Pointer to the instanced rendering shader.
 * @param count           Number of cube instances to simulate.
 */
void StressTestManager::Init(GameApp& app,  Shader* instancedShader, size_t count){
    shaderInstanced = instancedShader;

    // get mesh from gameApp
    stressMesh = app.GetMesh(1);

    // choose a texture 
    stressTex = ResourceManager::GetTexture("crate"); 
    if (!stressTex) {
        stressTex = 0;
    }

    // cache world bounds from current labyrinth
    worldMinBound = app.GetWorldMinBound();
    worldMaxBound = app.GetWorldMaxBound();

    // fallback in case not set yet
    if (worldMaxBound.x <= worldMinBound.x ||
        worldMaxBound.y <= worldMinBound.y)
    {
        Camera2D& cam = app.GetCamera();
        worldMinBound = { 0.f, 0.f };
        worldMaxBound = { cam.viewportWidth, cam.viewportHeight };
    }

    innerMinBound = {
    worldMinBound.x + wallThickness,
    worldMinBound.y + wallThickness
    };
    innerMaxBound = {
        worldMaxBound.x - wallThickness,
        worldMaxBound.y - wallThickness
    };

    // prepare sim data for N cubes
    CreateCPUData(count, worldMinBound, worldMaxBound);

    // prepare GL
    CreateGPUBuffer();
    ConfigureMeshInstancing();

    stressActive = false;
    inStressWorld = false;
}


/**
 * @brief Updates all simulated instances and uploads new data to the GPU.
 *
 * Integrates physics for each simulated cube, rebuilds per-instance
 * transformation data on the CPU, and sends it to the GPU.
 *
 * @param dt Delta time (in seconds).
 */
void StressTestManager::Update(float dt){
    if (!stressActive) return;

    for (auto& s : sims) {
        IntegrateOne(s, dt);
    }

    // rebuild per-instance GPU data
    BuildInstanceBufferCPU();
    UploadInstanceBufferGPU();
}


/**
 * @brief Renders all active instances using GPU instancing.
 *
 * Binds the instancing attributes, applies the shader and texture,
 * and issues a single instanced draw call for all cubes.
 *
 * @param renderer Reference to the Renderer system.
 */
void StressTestManager::Draw(Renderer& renderer) {
    if (!stressActive || !stressMesh || !shaderInstanced)
        return;

    // Rebind instanced attributes every frame,
    // because RenderSystem will reset the VAO state.
    ConfigureMeshInstancing();

    // Prepare shader
    renderer.SetShader(shaderInstanced);
    renderer.SetVPForInstanced();

    // Bind texture (shared crate)
    renderer.BindTexture(stressTex);

    // Draw N instances
    stressMesh->DrawInstanced(*shaderInstanced, static_cast<GLsizei>(sims.size()));
}


/**
 * @brief Enters stress test mode by clearing the current world and spawning a new arena.
 *
 * Removes all non-persistent entities, builds the stress-test arena walls,
 * and enables simulation and rendering of the 1000+ cube instances.
 *
 * @param app Reference to the active GameApp instance.
 */
void StressTestManager::EnterStressMode(GameApp& app){
    if (inStressWorld) return; // already inside

    if (instanceVBO) {
        glDeleteBuffers(1, &instanceVBO);
        instanceVBO = 0;
    }

    // Recreate fresh GPU buffer and binding state
    CreateGPUBuffer();
    ConfigureMeshInstancing();

    // 1. wipe everything except persistent-tag entities (player etc.)
    ClearNonPersistentEntities(app);

    // 2. rebuild arena: just border walls for collision/visual
    BuildStressArenaGeometry(app);

    // 3. ensure that gameApp knows the current world bound 
    app.SetWorldBounds(worldMinBound, worldMaxBound);

    // 4. activate simulation/render of stress cubes
    stressActive = true;
    inStressWorld = true;

    DebugConsole::Get().Warning("[StressTest] Entered stress mode\n");
}


/**
 * @brief Exits stress mode and reloads the normal labyrinth scene.
 *
 * Turns off instancing and resets the active scene by reloading the JSON level.   
 *
 * @param app Reference to the active GameApp instance.
 */
void StressTestManager::ExitStressMode(GameApp& /*app*/){
    // Empty for now
}


/**
 * @brief Generates random initial positions and velocities for all cubes.
 *
 * Spawns each cube inside the specified world bounds with random positions,
 * directions, and rotation speeds.
 *
 * @param count Number of instances to create.
 * @param wmin  Minimum world coordinate (bottom-left).
 * @param wmax  Maximum world coordinate (top-right).
 */
void StressTestManager::CreateCPUData(size_t count,
    const Vector2& wmin,
    const Vector2& wmax)
{
    sims.resize(count);
    gpuBufferCPU.resize(count);

    float xmin = wmin.x;
    float xmax = wmax.x;
    float ymin = wmin.y;
    float ymax = wmax.y;

    // safe guard against inverted bound 
    if (xmax < xmin) std::swap(xmin, xmax);
    if (ymax < ymin) std::swap(ymin, ymax);

    for (auto& s : sims) {
        // random spawn inside world bounds
        float rx = static_cast<float>(rand()) / RAND_MAX;
        float ry = static_cast<float>(rand()) / RAND_MAX;

        s.pos.x = xmin + rx * (xmax - xmin);
        s.pos.y = ymin + ry * (ymax - ymin);

        // random velocity
        float vx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 300.f;
        float vy = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 300.f;
        s.vel = { vx, vy };

        s.rot = 0.f;
        s.rotVel = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.f;
        s.scale = 50.f;
        s.tint = { 1.f, 1.f, 1.f };
        s.age01 = 0.f;
    }
}


/**
 * @brief Allocates a GPU buffer for per-instance data (transform, color, etc.).
 */
void StressTestManager::CreateGPUBuffer() {
    // Create VBO for per-instance data (mat3 + color + age)
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
        gpuBufferCPU.size() * sizeof(StressGPU),
        nullptr,
        GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


/**
 * @brief Configures instanced vertex attributes for the shared mesh.
 *
 * Binds the per-instance attributes (mat3 transform, colors, age) to
 * attribute locations 3�8.
 */
void StressTestManager::ConfigureMeshInstancing() {
    if (!stressMesh) {
        DebugConsole::Get().Error("[StressTest] stressMesh not assigned.\n");
        return;
    }
    GLuint vao = stressMesh->GetVAO();
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    GLsizei stride = sizeof(StressGPU);
    const size_t base = 0;

    // mat3 (3 vec3 columns)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(base + offsetof(StressGPU, m0)));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(base + offsetof(StressGPU, m1)));
    glVertexAttribDivisor(4, 1);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, stride, (void*)(base + offsetof(StressGPU, m2)));
    glVertexAttribDivisor(5, 1);

    // colorStart / colorEnd / age01
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, stride, (void*)(base + offsetof(StressGPU, colorStart)));
    glVertexAttribDivisor(6, 1);

    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, stride, (void*)(base + offsetof(StressGPU, colorEnd)));
    glVertexAttribDivisor(7, 1);

    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, stride, (void*)(base + offsetof(StressGPU, age01)));
    glVertexAttribDivisor(8, 1);

    glBindVertexArray(0);
}


/**
 * @brief Builds a rectangular wall enclosure for the stress arena.
 *
 * Generates border wall entities around the defined world bounds using
 * prefab tiles of size 100�100.
 *
 * @param app Reference to the active GameApp instance.
 */
void StressTestManager::BuildStressArenaGeometry(GameApp& /*app*/) {

    // Empty for now
}


/**
 * @brief Removes all non-persistent entities from the world.
 *
 * Keeps only entities tagged as persistent (e.g., player) while
 * clearing all others before entering stress mode.
 *
 * @param app Reference to the active GameApp instance.
 */
void StressTestManager::ClearNonPersistentEntities(GameApp& /*app*/){
    // Empty for now
}



/**
 * @brief Builds per-instance transformation and color data for GPU upload.
 */
void StressTestManager::BuildInstanceBufferCPU() {
    for (size_t i = 0; i < sims.size(); ++i) {
        const StressSim& s = sims[i];
        StressGPU& gpu = gpuBufferCPU[i];

        // Build model matrix (column-major 3x3)
        float c = cosf(s.rot);
        float sn = sinf(s.rot);
        float sx = s.scale, sy = s.scale;

        // Column 0
        gpu.m0[0] = c * sx;
        gpu.m0[1] = sn * sx;
        gpu.m0[2] = 0.f;

        // Column 1
        gpu.m1[0] = -sn * sy;
        gpu.m1[1] = c * sy;
        gpu.m1[2] = 0.f;

        // Column 2 (translation)
        gpu.m2[0] = s.pos.x;
        gpu.m2[1] = s.pos.y;
        gpu.m2[2] = 1.f;

        // Colors
        gpu.colorStart[0] = 1.0f;
        gpu.colorStart[1] = 0.3f;
        gpu.colorStart[2] = 0.2f;

        gpu.colorEnd[0] = 0.2f;
        gpu.colorEnd[1] = 0.8f;
        gpu.colorEnd[2] = 1.0f;

        gpu.age01 = s.age01;
    }
}


/**
 * @brief Uploads the current per-instance data to the GPU buffer.
 */
void StressTestManager::UploadInstanceBufferGPU() {
    if (!instanceVBO)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        gpuBufferCPU.size() * sizeof(StressGPU),
        gpuBufferCPU.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


/**
 * @brief Updates a single instance's position, rotation, and boundary collisions.
 *
 * Performs simple Euler integration and bounces objects off the inner bounds
 * of the arena, inverting velocity upon collision.
 *
 * @param s  Reference to a single StressSim instance.
 * @param dt Delta time (in seconds).
 */
void StressTestManager::IntegrateOne(StressSim& s, float dt) {
    // advance linear + angular
    s.pos = s.pos + s.vel * dt;
    s.rot += s.rotVel * dt;
    s.age01 += dt * 0.1f;
    if (s.age01 > 1.f) s.age01 -= 1.f;

    // bounce against the inner walls
    const float minX = innerMinBound.x;
    const float maxX = innerMaxBound.x - s.scale; 
    const float minY = innerMinBound.y;
    const float maxY = innerMaxBound.y - s.scale;

    if (s.pos.x < minX) { s.pos.x = minX; s.vel.x *= -1.f; }
    if (s.pos.x > maxX) { s.pos.x = maxX; s.vel.x *= -1.f; }
    if (s.pos.y < minY) { s.pos.y = minY; s.vel.y *= -1.f; }
    if (s.pos.y > maxY) { s.pos.y = maxY; s.vel.y *= -1.f; }
}