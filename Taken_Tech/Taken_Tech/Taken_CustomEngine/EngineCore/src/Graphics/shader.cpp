/**
 * @file    shader.cpp
 * @author  Jethro
 * @email   sung.h,t.weiliangterril
 * @co-author Tan Wei Liang Terril
 * @date    2025-09-29
 *
 * @brief   Implements the Shader class for GLSL programs.
 *
 * Provides compilation from source strings, file loading,
 * linking, and uniform uploads. Includes debugging helpers
 * to list active attributes/uniforms for validation.
 *
 * @version 1.0
 */
#include "Graphics/shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>
#include "Input/DebugConsole.hpp"

/**
	* @brief Constructs an empty Shader object.
	*
	* Initializes the OpenGL program ID to 0. No GPU resources are allocated
	* until Compile() or LoadFromFile() is invoked.
*/
Shader::Shader() : ID(0) {} // Initialize ID to 0, Id is the openGl handle for the shader program

/**
	* @brief Destroys the shader program if one exists.
	*
	* Deletes the linked GLSL program stored in @ref ID. Does not throw on
	* invalid or already-deleted IDs.
*/
Shader::~Shader() {
	if (ID) glDeleteProgram(ID); // Delete the shader program
}

// Create shader from source code strings
// We create a vertex shader and a fragment shader, compile them, link them into a shader program represented by ID, if compilation fails, we print the error and return false
// If successful, we delete the individual shaders as they are no longer needed after linking
/**
	* @brief Compiles vertex and fragment shader source into an OpenGL program.
	*
	* Steps performed:
	*  1. Creates individual vertex and fragment shader objects.
	*  2. Uploads and compiles GLSL source for each.
	*  3. Links both into a final shader program.
	*  4. Deletes temporary shader objects after linking.
	*
	* On any compilation or link failure, the function prints detailed
	* debug logs and returns false.
	*
	* @param vertSrc Null-terminated vertex shader GLSL source.
	* @param fragSrc Null-terminated fragment shader GLSL source.
	*
	* @return true on success; false on compilation or linking failure.
*/
bool Shader::Compile(const char* vertSrc, const char* fragSrc) {
	GLuint vertex, fragment;
	// Vertex Shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertSrc, NULL); // Attach the shader source code to the shader object
	glCompileShader(vertex);// Compile the shader
	if (!CheckCompileErrors(vertex, "VERTEX")) { // Check for compilation errors
		glDeleteShader(vertex);
		return false;
	}
	// Fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragSrc, NULL);
	glCompileShader(fragment);
	if (!CheckCompileErrors(fragment, "FRAGMENT")) {
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		return false;
	}
	// Shader Program
	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	if (!CheckCompileErrors(ID, "PROGRAM")) {
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		glDeleteProgram(ID);
		ID = 0;
		return false;
	}
	// Delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	return true;
}

/**
	* @brief Loads vertex and fragment shader files from disk and compiles them.
	*
	* Reads both GLSL files into strings, then calls Compile(). On success,
	* logs active shader attributes and uniforms for debugging and validation.
	*
	* @param vertPath Path to the vertex shader file.
	* @param fragPath Path to the fragment shader file.
	*
	* @return true if both files loaded and compiled successfully.
*/
bool Shader::LoadFromFile(const std::string& vertPath, const std::string& fragPath) {
	std::string vertCode = LoadFile(vertPath);
	std::string fragCode = LoadFile(fragPath);
	if (vertCode.empty() || fragCode.empty()) {
		DebugConsole::Get().Error("SHADER::EMPTY_SOURCE\n");
		return false;
	}

	bool result = Compile(vertCode.c_str(), fragCode.c_str());

	if (!result) {
		DebugConsole::Get().Error("SHADER::COMPILATION_OR_LINKING_FAILED\n");
	}
	else {
		DebugConsole::Get().Success("SHADER::PROGRAM_CREATED ID=" + std::to_string(ID));
		PrintActiveInfo();
		PrintActiveUniforms();
	}

	return result;
}

/**
	* @brief Reads a text file into a std::string.
	*
	* Opens the file using std::ifstream, streams its entire contents into
	* a std::stringstream, and returns the resulting string.
	*
	* @param path Filesystem path to a UTF-8 text file.
	*
	* @return File contents as a string. Returns an empty string if the file
	*         cannot be opened.
*/
std::string Shader::LoadFile(const std::string& path) const {
	std::ifstream file;
	std::stringstream buffer;
	file.open(path);
	if (!file.is_open()) {
		DebugConsole::Get().Error("SHADER::FILE_NOT_SUCCESFULLY_READ: " + path + "\n");
		return "";
	}
	buffer << file.rdbuf();
	file.close();
	return buffer.str();
}

/**
	* @brief Checks for shader compilation or program linking errors.
	*
	* If @p type is "PROGRAM", inspects the link status via glGetProgramiv().
	* Otherwise, inspects shader compilation status via glGetShaderiv().
	*
	* Prints full GLSL error logs using DebugConsole.
	*
	* @param shader OpenGL shader or program ID.
	* @param type   Either "VERTEX", "FRAGMENT", or "PROGRAM".
	*
	* @return true if compilation/linking succeeded, false otherwise.
*/
bool Shader::CheckCompileErrors(GLuint shader, const std::string& type) const {
	GLint success;
	GLchar infoLog[1024];
	if (type != "PROGRAM") {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "ERROR::SHADER_COMPILATION_ERROR of type: ", type, "\n", infoLog, "\n -- --------------------------------------------------- -- \n");
			return false;
		}
	} else {
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "ERROR::PROGRAM_LINKING_ERROR of type: ", type, "\n", infoLog, "\n -- --------------------------------------------------- -- \n");
			return false;
		}
	}
	return true;
}

// Activate the shader program for drawing, must be called before rendering or setting uniforms
void Shader::Use() const {
	glUseProgram(ID);
}

/**
	* @brief Logs all active vertex attributes in the shader program.
	*
	* Prints:
	*  - Attribute index
	*  - GLSL type enum
	*  - Name
	*  - Location
	*
	* Useful for debugging mismatches between VAO layouts and shader inputs.
*/
void Shader::PrintActiveInfo() const {
	GLint numAttribs = 0;
	glGetProgramiv(ID, GL_ACTIVE_ATTRIBUTES, &numAttribs);
	DebugConsole::Get().Info("ACTIVE_ATTRIBUTES: " + std::to_string(numAttribs));
	for (GLint i = 0; i < numAttribs; ++i) {
		GLchar name[256];
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveAttrib(ID, i, sizeof(name), &length, &size, &type, name);
		GLint location = glGetAttribLocation(ID, name);
		DebugConsole::Get().Info("ATTRIBUTE #" + std::to_string(i) + ": TYPE: " + std::to_string(type) + " NAME: " + std::string(name) + " LOCATION: " + std::to_string(location));
	}
}

/**
	* @brief Logs all active uniform variables in the shader program.
	*
	* Prints:
	*  - Uniform index
	*  - GLSL type enum
	*  - Name
	*  - Location
	*
	* Helps validate uniform bindings during engine development.
*/
void Shader::PrintActiveUniforms() const {
	GLint numUniforms = 0;
	glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &numUniforms);
	DebugConsole::Get().Info("ACTIVE_UNIFORMS: " + std::to_string(numUniforms));
	for (GLint i = 0; i < numUniforms; ++i) {
		GLchar name[256];
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveUniform(ID, i, sizeof(name), &length, &size, &type, name);
		GLint location = glGetUniformLocation(ID, name);
		DebugConsole::Get().Info("ATTRIBUTE #" + std::to_string(i) + ": TYPE: " + std::to_string(type) + " NAME: " + std::string(name) + " LOCATION: " + std::to_string(location));
	}
}

//Uniform Setters for math
//These send data from CPU to GPU, we get the location of the uniform variable in the shader program using glGetUniformLocation and then set its value using the appropriate glUniform* function
void Shader::SetBool(const std::string& name, bool val) const {
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)val);
}

void Shader::SetInt(const std::string& name, int val) const {
	glUniform1i(glGetUniformLocation(ID, name.c_str()), val);
}

void Shader::SetFloat(const std::string& name, float val) const {
	glUniform1f(glGetUniformLocation(ID, name.c_str()), val);
}

void Shader::SetVec2(const std::string& name, const Vector2& v) const {
	glUniform2f(glGetUniformLocation(ID, name.c_str()), v.x, v.y);
}

void Shader::SetVec3(const std::string& name, const Vector3& v) const {
	glUniform3f(glGetUniformLocation(ID, name.c_str()), v.x, v.y, v.z);
}

void Shader::SetMat3(const std::string& name, const Matrix3x3& m) const {
	glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, m.Data());
}