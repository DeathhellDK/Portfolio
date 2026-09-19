/**
* @file     renderer.cpp
* @author   Jethro Sung
* @email    sung.h
* @co-author Woh Kye Le, Tan Wei Liang Terril
* @email     w.kyele,t.weiliangterril
* @date      2025-09-11
* @brief     Binds shader/camera, uploads uniforms, and draws Mesh2D
* 
* Provides shader initialization, clearing, camera assignment, and
* mesh drawing with color and optional textures. Handles uploading
* of MVP matrices and shader uniforms before issuing draw calls.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "Graphics/renderer.h"
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include "Input/DebugConsole.hpp"

/**
    * @brief Constructs an empty renderer with no active camera.
    *
    * Initializes internal FBO/texture IDs to zero. The renderer does not
    * alloc any GPU resources until Initialize() or CreateOrResizeSceneTarget()
    * is called.
*/
Renderer::Renderer() : camera(nullptr) {}

/**
    * @brief Releases internally owned GPU resources.
    *
    * Destroys:
    *  - Scene color texture
    *  - Scene depth-stencil renderbuffer
    *  - Scene framebuffer object
    *
    * Does not destroy external textures or shaders.
*/
Renderer::~Renderer() {
    if (sceneColorTex) {
        glDeleteTextures(1, &sceneColorTex);
        sceneColorTex = 0;
    }
    if (sceneDepthRbo) {
        glDeleteRenderbuffers(1, &sceneDepthRbo);
        sceneDepthRbo = 0;
    }
    if (sceneFBO) {
        glDeleteFramebuffers(1, &sceneFBO);
        sceneFBO = 0;
    }
}

/**
 * @brief Initializes the renderer's internal shader from source files.
 *
 * This loads and links a shader into the Renderer's owned `shader` member.
 * After this succeeds, that shader can be used for DrawMesh() if no
 * alternate shader is provided via SetShader().
 *
 * @param vertPath Filesystem path to the vertex shader GLSL file.
 * @param fragPath Filesystem path to the fragment shader GLSL file.
 * @return true if compilation+linking succeeded, false on failure.
 */
//bool Renderer::Initialize(const std::string& vertPath, const std::string& fragPath) {
//	// Compile and load the shader program from specified vertex and fragment shader files
//	if (!shader.LoadFromFile(vertPath, fragPath)) {
//        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "Failed to load shader from files: " + vertPath + ", " + fragPath);
//		return false;
//	}
//	return true;
//}

bool Renderer::Initialize(const std::string& vertPath, const std::string& fragPath,
    const std::string& particleVertPath, const std::string& particleFragPath)
{
    // Default shader
    if (!shaderDefault.LoadFromFile(vertPath, fragPath)) {
        DebugConsole::Get().Error("[Shader] Default shader failed to load\n");
        return false;
    }

    // Particle(instanced) shader
    if (!shaderParticles.LoadFromFile(particleVertPath, particleFragPath)) {
        DebugConsole::Get().Error("[Shader] Particle instanced shader failed to load\n");
        return false;
    }

    // Start with default shader active
    SetShader(&shaderDefault);
    return true;
}

/**
 * @brief Clear the framebuffer to a solid RGBA color.
 *
 *
 * @param r Red channel (0.0f - 1.0f).
 * @param g Green channel (0.0f - 1.0f).
 * @param b Blue channel (0.0f - 1.0f).
 * @param a Alpha channel (0.0f - 1.0f).
 */
void Renderer::Clear(float r, float g, float b, float a) const {
	glClearColor(r, g, b, a); // Set the clear color
	glClear(GL_COLOR_BUFFER_BIT); // Clear the color buffer
}

/**
 * @brief Binds or unbinds a texture for subsequent draw calls.
 *
 * Also sets the shader uniforms `u_Tex` (sampler index) and
 * `u_UseTexture` (bool) so the fragment shader knows whether
 * to sample a texture or just use tint color.
 *
 * @param texId OpenGL texture ID to bind. Pass 0 to indicate "no texture".
 */
void Renderer::BindTexture(GLuint texId) {
    Shader* s = activeShader ? activeShader : &shaderDefault;
    s->Use();
    if (texId) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        s->SetInt("u_Tex", 0);
        s->SetBool("u_UseTexture", true);
    }
    else {
        s->SetBool("u_UseTexture", false);
    }
}

/**
 * @brief Uploads the shared ViewProjection matrix for instanced draws.
 *
 * Instanced rendering shaders typically multiply `u_VP` by each
 * instance's per-object model matrix (stored in per-instance attribs),
 * instead of receiving a full `u_MVP` uniform per draw.
 *
 * This function:
 *  - Computes the camera view-projection transform, or identity if no camera.
 *  - Uploads that matrix to the active shader uniform `u_VP`.
 */
void Renderer::SetVPForInstanced() {
    Shader* s = &shaderParticles;
    s->Use();

    // MVP = VP * M. 
    Matrix3x3 I;
    Matrix3x3 vp = camera ? camera->GetMVP(I) : I;
    s->SetMat3("u_VP", vp);  // instanced vertex shader that uses u_VP * perInstanceModel

    s->SetFloat("u_Alpha", globalAlpha);
}

/**
 * @brief Perform an instanced draw call using glDrawElementsInstanced().
 *
 * Assumes:
 *  - mesh VAO is configured with both vertex data and per-instance attributes.
 *  - The correct shader is already active (or handled here).
 *  - SetVPForInstanced() has been called to upload `u_VP`.
 *
 * @param mesh           Reference to the mesh to render.
 * @param instanceCount  Number of instances to draw in this call.
 */
void Renderer::DrawMeshInstanced(const Mesh2D& mesh, GLsizei instanceCount) {
    extern GLuint GetMeshVAO(const Mesh2D&);
    extern GLsizei GetMeshIndexCount(const Mesh2D&);

    Shader* s = &shaderParticles;
    s->Use();

    s->SetFloat("u_Alpha", globalAlpha);

    GLuint vao = mesh.GetVAO();
    GLsizei idx = static_cast<GLsizei>(mesh.GetIndexCount());

    glBindVertexArray(vao);
    glDrawElementsInstanced(GL_TRIANGLES, idx, GL_UNSIGNED_INT, nullptr, instanceCount);
}


/**
 * @brief Draws a Mesh2D with a model matrix, tint, and optional texture.
 *
 * Workflow:
 * 1. Ensures a camera is set; otherwise logs an error.
 * 2. Binds the shader and uploads:
 *    - MVP matrix (`u_MVP`).
 *    - Tint color (`u_Tint`).
 *    - Texture binding and flag (`u_Tex`, `u_UseTexture`).
 * 3. Calls Mesh2D::Draw() to issue the draw call.
 *
 * @param mesh The mesh geometry to render.
 * @param model Model transform matrix.
 * @param color RGB tint color.
 * @param texId Texture ID to bind (0 = no texture).
 */
void Renderer::DrawMesh(const Mesh2D& mesh, const Matrix3x3& model, const Vector3& color, GLuint texId) {\

    // pick the shader to use:
    Shader* s = activeShader ? activeShader : &shaderDefault;
    s->Use();
    s->SetFloat("u_Alpha", globalAlpha);

    Matrix3x3 mvp;
    if (camera) {
        mvp = camera->GetMVP(model);  // world-space draw
    }
    else {
        mvp = model;                  // screen-space draw (no camera transform)
    }

    // Upload MVP matrix
    s->SetMat3("u_MVP", mvp);

    // Upload tint color 
    s->SetVec3("u_Tint", color);

    if (texId) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        s->SetInt("u_Tex", 0);
        s->SetBool("u_UseTexture", true);
    }
    else {
        s->SetBool("u_UseTexture", false);
    }

    // Issue draw
    mesh.Draw(*s);
}

/**
 * @brief Draws a Mesh2D using a Material for tint and texture.
 *
 * This is equivalent to the other DrawMesh(), but instead of passing
 * `color` and `texId` separately, both are read from @p mat.
 *
 * @param mesh   Mesh geometry to render.
 * @param model  Model transform for this instance.
 * @param mat    Material providing tint color and texture ID.
 */
void Renderer::DrawMesh(const Mesh2D& mesh, const Matrix3x3& model, const Material& mat) {
    Shader* s = activeShader ? activeShader : &shaderDefault;
    s->Use();
    s->SetFloat("u_Alpha", globalAlpha);

    Matrix3x3 mvp = camera ? camera->GetMVP(model) : model;
    s->SetMat3("u_MVP", mvp);
    s->SetVec3("u_Tint", mat.GetColor());
    s->SetFloat("u_Alpha", 1.0f);

    const GLuint tex = mat.GetTexture();
    if (tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        s->SetInt("u_Tex", 0);
        s->SetBool("u_UseTexture", true);
    }
    else {
        s->SetBool("u_UseTexture", false);
    }

    mesh.Draw(*s);
}

/**
    * @brief Creates or resizes the offscreen framebuffer target.
    *
    * Allocates:
    *  - RGBA8 color texture (sceneColorTex)
    *  - Depth-stencil renderbuffer (sceneDepthRbo)
    *  - Framebuffer object (sceneFBO)
    *
    * If the size matches the existing target, the function does nothing.
    * On failure, all partial GPU objects are cleaned up and false is returned.
    *
    * @param width  Desired offscreen width in pixels.
    * @param height Desired offscreen height in pixels.
    *
    * @return true on successful creation, false if FBO is incomplete.
    *
    * @note Used by the EditorViewport and any system requiring offscreen rendering.
*/
bool Renderer::CreateOrResizeSceneTarget(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    // If the size is unchanged and we already have a target, nothing to do
    if (sceneFBO != 0 && width == sceneWidth && height == sceneHeight) {
        return true;
    }

    // Destroy any existing resources
    if (sceneColorTex) {
        glDeleteTextures(1, &sceneColorTex);
        sceneColorTex = 0;
    }
    if (sceneDepthRbo) {
        glDeleteRenderbuffers(1, &sceneDepthRbo);
        sceneDepthRbo = 0;
    }
    if (sceneFBO) {
        glDeleteFramebuffers(1, &sceneFBO);
        sceneFBO = 0;
    }

    sceneWidth = width;
    sceneHeight = height;

    // --- Create color texture ---
    glGenTextures(1, &sceneColorTex);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
        sceneWidth, sceneHeight,
        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // --- Create depth-stencil renderbuffer ---
    glGenRenderbuffers(1, &sceneDepthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
        sceneWidth, sceneHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // --- Create framebuffer and attach ---
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        sceneColorTex,
        0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        sceneDepthRbo);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        // Log and clean up on failure
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error,
            "Renderer: scene FBO is incomplete (status = 0x", std::hex, status, std::dec, ")");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteTextures(1, &sceneColorTex);   sceneColorTex = 0;
        glDeleteRenderbuffers(1, &sceneDepthRbo); sceneDepthRbo = 0;
        glDeleteFramebuffers(1, &sceneFBO);    sceneFBO = 0;
        sceneWidth = sceneHeight = 0;
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

/**
    * @brief Binds the offscreen scene framebuffer for rendering.
    *
    * Sets:
    *  - glBindFramebuffer(sceneFBO)
    *  - glViewport(0,0,width,height)
    *
    * Subsequent draw calls render into the scene target instead of the backbuffer.
    *
    * @note Does nothing if no scene target exists.
*/
void Renderer::BindSceneTarget() {
    if (!sceneFBO) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, sceneWidth, sceneHeight);
}

/**
    * @brief Restores rendering to the default framebuffer.
    *
    * Rebinds framebuffer 0 and restores viewport dimensions of the main window.
    *
    * @param fbWidth  Window framebuffer width.
    * @param fbHeight Window framebuffer height.
*/
void Renderer::UnbindSceneTarget(int fbWidth, int fbHeight) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);
}