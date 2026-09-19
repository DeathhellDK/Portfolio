/**
* @file     mesh2d.cpp
* @author   Jethro Sung
* @email	sung.h,t.weiliangterril
* @co-author Woh Kye Le, Tan Wei Liang Terril
* @email     w.kyele
* @date      2025-09-11
* @brief   Implements the Mesh2D class for GPU-managed 2D geometry.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Graphics/mesh2d.h"
#include <iostream>
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include "Input/DebugConsole.hpp"

/**
 * @brief Default constructor.
 *
 * Initializes member variables to zero or false.
 */
Mesh2D::Mesh2D()
	: VAO(0), VBO(0), EBO(0), indexCount(0), initialized(false), vertCapacity(0), indexCapacity(0), primitiveType(GL_TRIANGLES), lineWidth(2.0f) , pointSize(5.0f) { // Initialize members to zero or false, no GPU resources allocated yet
}

/**
 * @brief Constructs a mesh from given vertices and indices.
 *
 * Calls Reserve(), SetVertices(), and SetIndices() internally to fully
 * initialize the mesh and upload its geometry to the GPU.
 *
 * @param vertices Vertex array to upload.
 * @param indices Index array to upload.
 */
Mesh2D::Mesh2D(const std::vector<Vertex2D>& vertices, const std::vector<unsigned int>& indices)
	: Mesh2D() { // Delegate to default constructor to initialize members
	Reserve(vertices.size(), indices.size()); // Reserve GPU buffers based on input sizes
	SetVertices(vertices); // Upload vertex data to GPU
	SetIndices(indices);   // Upload index data to GPU
}

/**
 * @brief Destructor that releases all GPU resources.
 *
 * Deletes VAO, VBO, and EBO if they were allocated.
 * Safe to call even if no resources exist.
 */
Mesh2D::~Mesh2D() {
	if (VAO) {
		glDeleteVertexArrays(1, &VAO);// Delete the VAO to free GPU resources
	}
	if (VBO) {
		glDeleteBuffers(1, &VBO);// Delete the VBO to free GPU resources
	}
	if (EBO) {
		glDeleteBuffers(1, &EBO);// Delete the EBO to free GPU resources
	}
}

/**
 * @brief Allocates GPU buffers for vertices and indices.
 *
 * Sets up VAO attributes:
 * - Location 0 -> position (vec2)
 * - Location 1 -> texCoord (vec2)
 * - Location 2 -> color (vec3)
 *
 * @param maxVerts Maximum number of vertices to reserve.
 * @param maxIndices Maximum number of indices to reserve.
 */
void Mesh2D::Reserve(size_t maxVerts, size_t maxIndices) {

	if (initialized) {
		DebugConsole::Get().Error("Mesh2D::Reserve called on already initialized mesh.\n");
		return;
	}

	// Generate objects
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);

	// Each vertex = 7 floats (2 pos + 2 uv + 3 color)
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, maxVerts * sizeof(Vertex2D), nullptr, GL_DYNAMIC_DRAW);

	GLsizei stride = sizeof(Vertex2D);

	// Position (2 floats)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex2D, pos));
	glEnableVertexAttribArray(0);

	// TexCoord (2 floats)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex2D, texCoord));
	glEnableVertexAttribArray(1);

	// Color (3 floats)
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex2D, color));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);

	GLint attribVBO = 0, enabled = 0;
	glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &attribVBO);
	glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);

	// ---- DEBUG PRINTS ----
	/*DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "sizeof(Vertex2D) = " + sizeof(Vertex2D),
		"offsetof(pos)     = " + offsetof(Vertex2D, pos),
		"offsetof(texCoord)= " + offsetof(Vertex2D, texCoord),
		"offsetof(color)   = " + offsetof(Vertex2D, color), "\n");*/

	glBindVertexArray(0);

	vertCapacity = maxVerts;
	indexCapacity = maxIndices;
	initialized = true;


}

/**
 * @brief Uploads vertex data to GPU memory.
 *
 * Must not exceed the capacity reserved with Reserve().
 *
 * @param verts Vector of Vertex2D structs containing vertex data.
 */
void Mesh2D::SetVertices(const std::vector<Vertex2D>& verts) {
	if (!initialized) {
		Reserve(verts.size(), 0);
	}
	if (verts.size() > vertCapacity) {
		DebugConsole::Get().Error("Mesh2D::SetVertices - too many verts for the capacity provided\n");
		return;
	}
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(Vertex2D), verts.data());
}

/**
 * @brief Uploads index data to GPU memory.
 *
 * Must not exceed the capacity reserved with Reserve().
 * Also updates `indexCount` for subsequent draw calls.
 *
 * @param inds Vector of index values.
 */
void Mesh2D::SetIndices(const std::vector<unsigned int>& inds) {
	if (!initialized) {
		Reserve(0, inds.size());
	}
	if (inds.size() > indexCapacity) {
		DebugConsole::Get().Error("Mesh2D::SetVertices - too many indices for the capacity provided\n");
		return;
	}
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // Bind the EBO for updating
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, inds.size() * sizeof(unsigned int), inds.data()); // Update index data in the EBO
	indexCount = inds.size();
}

/**
 * @brief Draws the mesh using indexed rendering.
 *
 * Requires that a shader is already active and that
 * vertices/indices have been uploaded.
 *
 * @param shader Shader program to use.
 */
void Mesh2D::Draw(const Shader& shader) const {
	if (!initialized || indexCount == 0) {
		DebugConsole::Get().Error("Mesh2D::Draw called on uninitialized mesh or with zero indices.\n");
		return;
	}
	shader.Use();
	glBindVertexArray(VAO); // Bind the VAO for drawing
	//DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "Mesh2D::Draw - Drawing " + indexCount, " indices." + VAO, " VBO=" + VBO, " EBO=" + EBO, "\n");
	glDrawElements(primitiveType, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, 0); // Draw the mesh using indexed drawing
}

/**
 * @brief Draws multiple instances of the same mesh geometry.
 *
 * Uses glDrawElementsInstanced() to efficiently render many identical
 * meshes with different transforms or colors (set via instancing attributes).
 *
 * @param shader Shader to use for rendering.
 * @param instanceCount Number of instances to draw.
 */
void Mesh2D::DrawInstanced(const Shader& shader, GLsizei instanceCount) const {
	if (!initialized || indexCount == 0) {
		DebugConsole::Get().Error("Mesh2D::DrawInstanced called on uninitialized mesh or with zero indices.\n");
		return;
	}
	shader.Use();
	glBindVertexArray(VAO); // Bind the VAO for drawing
	glDrawElementsInstanced(primitiveType, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, 0, instanceCount);
}

/**
 * @brief Move constructor: transfers ownership of OpenGL buffers.
 *
 * After the move, the source mesh becomes empty and safe to destroy.
 *
 * @param other Mesh2D instance to move from.
 */
Mesh2D::Mesh2D(Mesh2D&& other) noexcept
	: VAO(other.VAO), VBO(other.VBO), EBO(other.EBO),
	indexCount(other.indexCount),
	initialized(other.initialized),
	vertCapacity(other.vertCapacity),
	indexCapacity(other.indexCapacity),
	primitiveType(other.primitiveType),
	lineWidth(other.lineWidth),
	pointSize(other.pointSize)
{
	other.VAO = 0; other.VBO = 0; other.EBO = 0;
	other.initialized = false;
	other.indexCount = 0;
	other.vertCapacity = 0;
	other.indexCapacity = 0;
	other.primitiveType = GL_TRIANGLES;
	other.lineWidth = 1.0f;
	other.pointSize = 1.0f;
}

/**
 * @brief Move assignment operator: releases current buffers and steals another's.
 *
 * Frees existing GPU buffers, then transfers ownership from @p other.
 * Source object is reset to a safe, uninitialized state.
 *
 * @param other Mesh2D instance to move from.
 * @return Reference to this object.
 */
Mesh2D& Mesh2D::operator=(Mesh2D&& other) noexcept {
	if (this != &other) {
		// cleanup self first
		if (VAO) glDeleteVertexArrays(1, &VAO);
		if (VBO) glDeleteBuffers(1, &VBO);
		if (EBO) glDeleteBuffers(1, &EBO);

		VAO = other.VAO; VBO = other.VBO; EBO = other.EBO;
		indexCount = other.indexCount;
		initialized = other.initialized;
		vertCapacity = other.vertCapacity;
		indexCapacity = other.indexCapacity;
		primitiveType = other.primitiveType;
		lineWidth = other.lineWidth;
		pointSize = other.pointSize;

		other.VAO = 0;
		other.VBO = 0;
		other.EBO = 0;
		other.initialized = false;
		other.indexCount = 0;
		other.vertCapacity = 0;
		other.indexCapacity = 0;
		other.primitiveType = GL_TRIANGLES;
		other.lineWidth = 1.0f;
		other.pointSize = 1.0f;
	}
	return *this;
}

