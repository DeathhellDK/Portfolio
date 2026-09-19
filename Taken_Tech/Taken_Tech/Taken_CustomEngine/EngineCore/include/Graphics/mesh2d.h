#pragma once
/**
* @file		mesh2d.h
* @author	Jethro Sung
* @email	sung.h
* @co-author Woh Kye Le
* @email     w.kyele
* @date      2025-09-11
*
* @brief   Declares the Mesh2D class for managing 2D vertex/index buffers.
*
* Mesh2D encapsulates an OpenGL VAO, VBO, and EBO to handle dynamic
* 2D meshes. It provides methods to reserve GPU memory, upload vertices
* and indices, and issue draw calls with an active Shader.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/


#include <glad/glad.h>
#include <vector>
#include "Graphics/shader.h"
#include "Graphics/vertex2d.h"

/**
 * @class Mesh2D
 * @brief Wraps OpenGL buffers for drawing 2D geometry.
 *
 * Provides safe RAII management of VAO, VBO, and EBO resources.
 * Supports move semantics but forbids copying. Before use, GPU
 * buffers must be reserved via `Reserve()`, then filled using
 * `SetVertices()` and `SetIndices()`. Rendering is done through
 * `Draw()`, assuming the correct Shader is already active.
 *
 * Typical usage:
 * Mesh2D mesh;
 * mesh.Reserve(100, 300); // reserve capacity
 * mesh.SetVertices(verts);
 * mesh.SetIndices(indices);
 * mesh.Draw(shader);
 */
class Mesh2D {
	private:
		GLuint VAO, VBO, EBO; // Vertex Array Object (store raw vertex data eg. pos, texcoord), Vertex Buffer Object(records how verts are laid out in the vbo and which ebo to use), Element Buffer Object (store indices for indexed drawing, eg. draw vert 0, draw vert 1)
		size_t indexCount;    // Number of indices currently uploaded
		bool initialized;     // if buffers have been created
		size_t vertCapacity, indexCapacity;  // Maximum number of vertices and indices that can be uploaded without reallocation
		GLenum primitiveType = GL_TRIANGLES; // default
		float lineWidth = 2.0f;  // only used if primitive = GL_LINES
		float pointSize = 5.0f;  // only used if primitive = GL_POINTS

	public:
		// constructors and destructors
		/** @brief Default constructor creating an empty mesh. */
		Mesh2D();
		
		/** 
		 * @brief Constructs a mesh from provided vertices and indices.
		 * @param vertices The vertex data to initialize with.
		 * @param indices The index data to initialize with.
		 */
		Mesh2D(const std::vector<Vertex2D>& vertices, const std::vector<unsigned int>& indices);
		
		/** @brief Destructor that releases GPU resources. */
		~Mesh2D();

		// forbid copy
		Mesh2D(const Mesh2D&) = delete;
		Mesh2D& operator=(const Mesh2D&) = delete;

		// allow move
		/**
		 * @brief Move constructor. Transfers ownership of OpenGL buffers.
		 * @param other The mesh to move from.
		 */
		Mesh2D(Mesh2D&& other) noexcept;
		
		/**
		 * @brief Move assignment operator. Replaces current mesh and transfers ownership.
		 * @param other The mesh to move from.
		 * @return Reference to this mesh.
		 */
		Mesh2D& operator=(Mesh2D&& other) noexcept;

		/**
		 * @brief Allocates GPU buffers with a fixed capacity.
		 * @param maxVerts Maximum number of vertices to reserve.
		 * @param maxIndices Maximum number of indices to reserve.
		 */
		void Reserve(size_t maxVerts, size_t maxIndices);

		/**
		 * @brief Uploads vertex data to GPU (must not exceed reserved capacity).
		 * @param verts Vertex array to upload.
		 */
		void SetVertices(const std::vector<Vertex2D>& verts);

		/**
		 * @brief Uploads index data to GPU (must not exceed reserved capacity).
		 * @param inds Index array to upload.
		 */
		void SetIndices(const std::vector<unsigned int>& inds);

		/**
		 * @brief Sets the primitive type for drawing.
		 * @param t OpenGL primitive mode.
		 */
		void SetPrimitive(GLenum t) { primitiveType = t; }

		/**
		 * @brief Sets line width for GL_LINES mode.
		 * @param t Width in pixels.
		 */
		void SetLineWidth(float t) { lineWidth = t; }

		/**
		* @brief Sets point size for GL_POINTS mode.
		* @param t Size in pixels.
		*/
		void SetPointSize(float t) { pointSize = t; }

		
		GLenum GetPrimitive()  const { return primitiveType; } /// @return Current primitive mode 
		size_t GetIndexCount() const { return indexCount; }    /// @return Number of indices currently uploaded
		GLuint GetVAO()        const { return VAO; }           /// @return The VAO handle for this mesh

		/**
		* @brief Issues a draw call using the uploaded data.
		* @param shader Shader program to use (must already be bound).
		*/
		void Draw(const Shader& shader) const;
		
		/**
		 * @brief Issues an instanced draw call using uploaded data.
		 * @param shader Shader program to use for drawing.
		 * @param instanceCount Number of instances to draw.
		 */
		void DrawInstanced(const Shader& shader, GLsizei instanceCount) const;
};