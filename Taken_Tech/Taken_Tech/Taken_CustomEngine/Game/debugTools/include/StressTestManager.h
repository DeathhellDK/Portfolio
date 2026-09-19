#pragma once
/**
* @file     StressTestManager.h
* @author   Woh Kye Le
* @email    w.kyele
* @co-author
* @date     2025-11-01
*
* @brief This Header file is specifically designed for any stress test scenario , this header prevents
*        GameApp from implementing the logic for any stress test, rather GameApp just calls function from
*		 here
* 
* Current TestCase : NiL
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Math/vect2.h"
#include "Math/vect3.h"
#include "Math/matrix3x3.h"
#include "glad/glad.h"
#include <vector>

class GameApp;
class Renderer;
class Mesh2D;
class Shader;

/**
 * @brief Per-instance simulation state (CPU side).
 */
struct StressSim {  
    Vector2 pos    { 0.f, 0.f };          // world-space position (bottom-left of quad)
    Vector2 vel    { 0.f, 0.f };          // velocity in world units/sec
    float   rot    { 0.f };               // rotation in radians
    float   rotVel { 0.f };               // angular velocity (rad/sec)
    float   scale  { 1.f };               // uniform scale
    Vector3 tint   { 1.f, 1.f, 1.f };     // base color
    float   age01  { 0.f };               // 0..1 normalized "lifetime" (used for color lerp)
};

/**
 * @brief Per-instance GPU data layout. Matches your instanced vertex shader:
 *
 * layout (location=3) in vec3 a_m0;
 * layout (location=4) in vec3 a_m1;
 * layout (location=5) in vec3 a_m2;
 * layout (location=6) in vec3 a_ColorStart;
 * layout (location=7) in vec3 a_ColorEnd;
 * layout (location=8) in float a_Age01;
 */
struct StressGPU {
    float m0[3];
    float m1[3];
    float m2[3];
    float colorStart[3];
    float colorEnd[3];
    float age01;
};

class StressTestManager {
public:
    StressTestManager() = default;
    ~StressTestManager();

    // Call once during GameApp::Initialize
    void Init(GameApp& app, Shader* instancedShader, size_t count); 

    // Called every frame from GameApp::Update
    void Update(float dt);

    // Called every frame from GameApp::Draw
    void Draw(Renderer& renderer);

    // Switch world <-> stress mode
    void EnterStressMode(GameApp& app);
    void ExitStressMode(GameApp& app);

    bool IsActive() const { return stressActive; }
    bool InStressWorld() const { return inStressWorld; }

private:
    // --- setup helpers ---
    void CreateCPUData(size_t count, const Vector2& worldMin, const Vector2& worldMax);
    void CreateGPUBuffer();
    void ConfigureMeshInstancing();
    void BuildStressArenaGeometry(GameApp& app);
    void ClearNonPersistentEntities(GameApp& app);

    // --- simulation helpers ---
    void BuildInstanceBufferCPU();
    void UploadInstanceBufferGPU();
    void IntegrateOne(StressSim& s, float dt);

private:
    // state flags
    bool stressActive = false; 
    bool inStressWorld = false; 

    // sim data
    std::vector<StressSim> sims;
    std::vector<StressGPU> gpuBufferCPU;

    // rendering hooks
    Shader* shaderInstanced = nullptr;
    Mesh2D* stressMesh = nullptr;
    GLuint  stressTex = 0;
    GLuint  instanceVBO = 0;

    // cached bounds
    Vector2 worldMinBound{ 0.f,0.f };
    Vector2 worldMaxBound{ 0.f,0.f };

    Vector2 innerMinBound{ 0.f, 0.f };
    Vector2 innerMaxBound{ 0.f, 0.f };
    float wallThickness = 100.f;
};