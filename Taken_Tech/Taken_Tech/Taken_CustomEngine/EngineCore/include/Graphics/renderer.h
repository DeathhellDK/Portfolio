#pragma once
/**
* @file		renderer.h
* @author	Jethro Sung
* @email	sung.h, w.kyele
* @co-author Woh Kye Le
* @email     w.kyele
* @date      2025-09-11
*
* @brief   Declares the Renderer class for 2D rendering.
*
* Renderer owns a shader and a camera reference, and provides functions
* for clearing the screen, binding a camera, and drawing Mesh2D objects
* with optional color and texture. It is used by systems such as
* RenderSystem to batch draw calls to OpenGL.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "Graphics/shader.h"
#include "Graphics/mesh2d.h"
#include "Graphics/camera2d.h"
#include "Graphics/material.h"
#include "Math/matrix3x3.h"
#include "Math/vect2.h"
#include "Math/vect3.h"

#include <algorithm>

/**
* @class Renderer
* @brief Handles drawing of 2D meshes using a shader and camera.
*
* Renderer abstracts OpenGL state management for drawing entities.
* It uses:
* - A `Shader` for GPU programs.
* - A `Camera2D` for view/projection matrices.
* - `Mesh2D` geometry, with per-entity tint color and optional texture.
*
* Example usage:
* @code
* Renderer renderer;
* renderer.Initialize("shaders/vert.glsl", "shaders/frag.glsl");
* renderer.setCamera(&camera);
* renderer.Clear(0.1f, 0.1f, 0.1f, 1.0f);
* renderer.DrawMesh(mesh, modelMatrix, {1,1,1}, textureId);
* @endcode
*/
class Renderer {
public:
	/** @brief Default constructor for Renderer. */
	Renderer();
	~Renderer();

	/**
	 * @brief Initializes the renderer with a vertex and fragment shader.
	 * @param vertPath Path to default vertex shader file.
	 * @param fragPath Path to default fragment shader file.
	 * @param particleVertPath Path to particle vertex shader file.
	 * @param particleFragPath Path to particle fragment shader file.
	 * @return True if shader compilation and linking succeeded.
	 */
	bool Initialize(const std::string& vertPath, const std::string& fragPath,
					const std::string& particleVertPath, const std::string& particleFragPath);

	/**
	 * @brief Clears the screen with the given RGBA color.
	 * @param r Red channel (0�1).
	 * @param g Green channel (0�1).
	 * @param b Blue channel (0�1).
	 * @param a Alpha channel (0�1).
	 */
	void Clear(float r, float g, float b, float a) const;

	/**
	 * @brief Binds a texture for the currently active shader and sets sampler uniforms.
	 * @param texId OpenGL texture object ID to bind. Use 0 to indicate "no texture".
	 */
	void BindTexture(GLuint texId);

    /**
	 * @brief Issues an instanced draw call for a mesh.
	 * @param mesh           The mesh to draw.
	 * @param instanceCount  How many instances to render in one call.
	 */
	void DrawMeshInstanced(const Mesh2D& mesh, GLsizei instanceCount);

	/**
	 * @brief Uploads camera view-projection state for instanced rendering.
	 */
	void SetVPForInstanced();

	/**
	 * @brief Assigns the active camera for rendering.
	 * @param cam Pointer to a Camera2D (must remain valid while in use).
	 */
	void setCamera(Camera2D* cam) { camera = cam; }


	/**
	 * @brief Sets the actual shader for rendering.
	 * @param S Pointer to a Shader
	 */
	void SetShader(Shader* S) { activeShader = S; }

	/**
	 * @brief Get the currently active shader.
	 */
	Shader* GetShader() { return activeShader; }

	/**
	 * @brief Get the currently default shader.
	 */
	Shader* GetDefaultShader() { return &shaderDefault; }

	/**
	 * @brief Get the currently particle shader.
	 */
	Shader* GetParticleShader() { return &shaderParticles; }

	/**
	 * @brief Get the currently assigned camera.
	 */
	Camera2D* getCamera() const { return camera; }

	/**
	* @brief Sets a global alpha multiplier applied to all subsequent draw calls.
	*/
	void SetGlobalAlpha(float a) { globalAlpha = std::clamp(a, 0.0f, 1.0f); }

	/**
	 * @brief Returns the current global alpha multiplier.
	 */
	float GetGlobalAlpha() const { return globalAlpha; }


	/**
	 * @brief Draws a Mesh2D with given transform, color, and optional texture.
	 * @param mesh The geometry to draw.
	 * @param model Model transform matrix.
	 * @param color Tint color applied to the mesh.
	 * @param texId OpenGL texture ID (0 = no texture).
	 */
	void DrawMesh(const Mesh2D& mesh, const Matrix3x3& model, const Vector3& color, GLuint texId = 0);

	/**
	 * @brief Draws a Mesh2D using a Material object.
	 * @param mesh   The geometry to render.
	 * @param model  The model transform matrix.
	 * @param mat    Material containing tint color and texture ID
	 */
	void DrawMesh(const Mesh2D& mesh, const Matrix3x3& model, const Material& mat);

	/**
		* @brief Creates or resizes the offscreen scene framebuffer.
		*
		* If an FBO already exists with a different size, it is destroyed
		* and recreated. Passing non-positive dimensions leaves the target
		* unchanged and returns false.
		*
		* This is intended for rendering the game into a texture for use
		* in an ImGui viewport window.
		*
		* @param width   Desired width in pixels.
		* @param height  Desired height in pixels.
		* @return true if the scene target is ready for use.
	*/
	bool CreateOrResizeSceneTarget(int width, int height);

	/**
		* @brief Binds the scene framebuffer as the current render target.
		*
		* Also sets the OpenGL viewport to match the scene target size.
		* Does nothing if no scene target has been created yet.
	*/
	void BindSceneTarget();

	/**
		* @brief Restores rendering to the default framebuffer (window).
		*
		* Also restores the viewport to the given window framebuffer size.
		*
		* @param fbWidth   Window framebuffer width in pixels.
		* @param fbHeight  Window framebuffer height in pixels.
	*/
	void UnbindSceneTarget(int fbWidth, int fbHeight);

	/**
		* @brief Returns the color texture attached to the scene FBO.
		*
		* You can pass this texture ID to ImGui::Image / ImGui::ImageButton.
		*
		* @return OpenGL texture ID, or 0 if no scene target exists.
	*/
	GLuint GetSceneTexture() const { return sceneColorTex; }

	/// @brief Width of the scene render target in pixels (0 if none).
	int GetSceneWidth()  const { return sceneWidth; }

	/// @brief Height of the scene render target in pixels (0 if none).
	int GetSceneHeight() const { return sceneHeight; }

private:
	Camera2D* camera; // Pointer to the camera for view/projection matrices
	Shader shaderDefault;    // default sprite/world shader
	Shader shaderParticles;  // instanced particle shader
	Shader* activeShader = nullptr; // the actual shader used to draw

	// --- Offscreen scene render target (used by editor ImGui viewport) ---
	GLuint sceneFBO = 0;
	GLuint sceneColorTex = 0;
	GLuint sceneDepthRbo = 0;
	int    sceneWidth = 0;
	int    sceneHeight = 0;

	float globalAlpha = 1.0f;
};
