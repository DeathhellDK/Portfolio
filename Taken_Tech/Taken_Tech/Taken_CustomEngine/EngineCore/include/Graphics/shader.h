#pragma once
/**
 * @file    shader.h
 * @author  Jethro
 * @email   sung.h
 * @date    2025-09-29
 *
 * @brief   Declares the Shader class for OpenGL GLSL programs.
 *
 * The Shader class encapsulates an OpenGL program object (vertex +
 * fragment shader). It provides loading, compilation, linking, and
 * uniform setters for CPU-> GPU communication.
 *
 * @version 1.0
 */
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "Math/matrix3x3.h"
#include "Math/vect2.h"
#include "Math/vect3.h"

/**
* @class Shader
* @brief Encapsulates an OpenGL shader program.
*
* Provides methods for:
* - Compiling shaders from source or files.
* - Binding the program for rendering.
* - Setting uniforms (bool, int, float, vec2, vec3, mat3).
* - Debugging active attributes and uniforms.
*
* Usage:
* @code
* Shader shader;
* shader.LoadFromFile("basic.vert", "basic.frag");
* shader.Use();
* shader.SetVec3("u_Tint", {1,0,0});
* @endcode
*/
class Shader {
	public:
		/** @brief Default constructor creating an empty shader. */
		Shader();
		~Shader();

		/**
		* @brief Compiles a shader program from source strings.
		* @param vertSrc GLSL source for vertex shader.
		* @param fragSrc GLSL source for fragment shader.
		* @return True if compilation + linking succeeded.
		*/
		bool Compile(const char* vertSrc, const char* fragSrc);

		/**
		* @brief Loads shader code from files and compiles program.
		* @param vertPath Path to vertex shader source file.
		* @param fragPath Path to fragment shader source file.
		* @return True if compilation + linking succeeded.
		*/
		bool LoadFromFile(const std::string& vertPath, const std::string& fragPath);

		// Activates this shader program for subsequent rendering.
		void Use() const;

		//debug info
		void PrintActiveInfo() const;
		void PrintActiveUniforms() const;

		//Uniform Setters for math
		void SetBool(const std::string& name, bool val) const;
		void SetInt(const std::string& name, int val) const;
		void SetFloat(const std::string& name, float val) const;
		void SetVec2(const std::string& name, const Vector2& v) const;
		void SetVec3(const std::string& name, const Vector3& v) const;
		void SetMat3(const std::string& name, const Matrix3x3& m) const;

		// @return Raw OpenGL program ID.
		unsigned int GetID() const { return ID; }

	private:
		GLuint ID;// OpenGL shader program ID
		std::string LoadFile(const std::string& path) const;//helper to load file
		bool CheckCompileErrors(GLuint shader, const std::string& type) const;//helper to check errors
};
