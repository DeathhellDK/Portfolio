#pragma once
/**
 * @file    matrix3x3.h
 * @author   Lim Zhi Jie
 * @co-author Tan Wei Liang Terril
 * @email    zhijie.lim, t.weiliangterril
 * @date    2025-09-29
 *
 * @brief   Declares a 3�3 matrix class for 2D transformations.
 *
 * Supports translation, rotation, scaling, determinant, inverse,
 * orthographic projection, and multiplication with other matrices
 * and 2D vectors.
 *
 * @version 1.0
 */
#include <iostream>
#include <cmath>
#include <stdexcept>
#include "vect2.h"

/**
* @class Matrix3x3
* @brief Represents a 3�3 matrix with row/column operations.
*
* Typically used for 2D affine transforms:
* - Translation
* - Rotation
* - Scaling
* - Projection (orthographic)
*/
class Matrix3x3
{
	private:
		float m[9];// Column-major order for OpenGL compatibility
	public:
		/** @brief Constructs an identity matrix by default. */
		Matrix3x3(); // Identity Matrix
		
		/**
		 * @brief Constructs a matrix with explicit values.
		 * @param e00 Row 0, Col 0
		 * @param e10 Row 1, Col 0
		 * @param e20 Row 2, Col 0
		 * @param e01 Row 0, Col 1
		 * @param e11 Row 1, Col 1
		 * @param e21 Row 2, Col 1
		 * @param e02 Row 0, Col 2
		 * @param e12 Row 1, Col 2
		 * @param e22 Row 2, Col 2
		 */
		Matrix3x3(float e00, float e10, float e20,
			float e01, float e11, float e21,
			float e02, float e12, float e22);
		//~Matrix3x3();

		//Elem Access
		/**
		 * @brief Mutable element access.
		 * @param col Column index.
		 * @param row Row index.
		 * @return Reference to the element.
		 */
		float& operator()(unsigned int col, unsigned int row);
		
		/**
		 * @brief Const element access.
		 * @param col Column index.
		 * @param row Row index.
		 * @return Const reference to the element.
		 */
		const float& operator()(unsigned int col, unsigned int row) const;

		//operators
		/**
		 * @brief Matrix-Matrix multiplication.
		 * @param rhs The right-hand side matrix.
		 * @return A new multiplied matrix.
		 */
		Matrix3x3 operator*(const Matrix3x3& rhs) const; //M*M, returns new matrix
		
		/**
		 * @brief Compound matrix multiplication.
		 * @param rhs The right-hand side matrix.
		 * @return Reference to this modified matrix.
		 */
		Matrix3x3 operator*=(const Matrix3x3& rhs);//compound assignment operator, A*=B
		
		/**
		 * @brief Matrix-Vector multiplication.
		 * @param v The 2D vector.
		 * @return A transformed 2D vector.
		 */
		Vector2 operator*(const Vector2& v) const; // M*v

		//Static Builders
		/**
		 * @brief Returns an identity matrix.
		 * @return The identity matrix.
		 */
		static Matrix3x3 Identity();
		
		/**
		 * @brief Builds a 2D translation matrix.
		 * @param tx X translation.
		 * @param ty Y translation.
		 * @return The translation matrix.
		 */
		static Matrix3x3 BuildTranslation(float tx, float ty);
		static Matrix3x3 BuildRotation(float radians);
		static Matrix3x3 BuildScaling(float sx, float sy);

		float Determinant() const;
		Matrix3x3 Inverse() const;

		//Utils
		const float* Data() const; // for OpenGL (glUniformMatrix3fv), eg. glUniformMatrix3fv(location, 1, GL_FALSE, M.Data());
		void Print() const; // for debug print

		// debug print
		friend std::ostream& operator<<(std::ostream& os, const Matrix3x3& m);

		// Build an orthographic projection matrix
		static Matrix3x3 Ortho(float left, float right, float bottom, float top);
};

// Non-member operator for scalar * Matrix
Matrix3x3 operator*(float s, const Matrix3x3& m);
