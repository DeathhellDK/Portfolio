/**
 * @file      vect3.h
 * @author	  Jethro Sung
 * @email	  sung.h
 * @date	  04/04/26
 *
 * @brief     Declares the Vector3 class used for 3D-style math utilities in
 *            the engine.
 *
 *            This class provides vector arithmetic, normalization, length,
 *            dot and cross products, indexed access, and helper utilities for
 *            systems that require 3-component data such as lighting, color,
 *            and extended spatial calculations.
*/

#pragma once
#include <iostream>


class Vector3 {
private:
	float mVec[3];
	public: 
		float& x;
		float& y;
		float& z;

		//Constructors
		/** @brief Default constructor initializing to (0,0,0). */
		Vector3();
		
		/**
		 * @brief Constructs a vector with explicit coordinates.
		 * @param x X coordinate.
		 * @param y Y coordinate.
		 * @param z Z coordinate.
		 */
		Vector3(float x, float y, float z);
		
		/**
		 * @brief Copy constructor.
		 * @param otherVect3 The vector to copy from.
		 */
		Vector3(const Vector3& otherVect3);

		//Base Operators

		// Assignment 
		/**
		 * @brief Assignment operator.
		 * @param rhs The right-hand side vector.
		 * @return Reference to this modified vector.
		 */
		Vector3& operator=(const Vector3& rhs);

		//Artihmetic
		/**
		 * @brief Vector addition.
		 * @param rhs The right-hand side vector.
		 * @return A new added vector.
		 */
		Vector3 operator+(const Vector3& rhs) const;
		
		/**
		 * @brief Vector subtraction.
		 * @param rhs The right-hand side vector.
		 * @return A new subtracted vector.
		 */
		Vector3 operator-(const Vector3& rhs) const;
		/**
		 * @brief Scalar multiplication.
		 * @param scalar The value to multiply by.
		 * @return A new modified vector.
		 */
		Vector3 operator*(float scalar) const;
		
		/**
		 * @brief Scalar division.
		 * @param scalar The value to divide by.
		 * @return A new modified vector.
		 */
		Vector3 operator/(float scalar) const;

		/** @brief Compound vector addition. */
		Vector3& operator+=(const Vector3& rhs);
		/** @brief Compound vector subtraction. */
		Vector3& operator-=(const Vector3& rhs);
		/** @brief Compound scalar multiplication. */
		Vector3& operator*=(float scalar);
		/** @brief Compound scalar division. */
		Vector3& operator/=(float scalar);

		/** @brief Equality check. */
		bool operator==(const Vector3& rhs) const;
		/** @brief Inequality check. */
		bool operator!=(const Vector3& rhs) const;

		//Vector Operations
		/** @brief Computes the length of the vector. */
		float Length() const;
		/** @brief Computes the squared length of the vector. */
		float LengthSqd() const;
		/** @brief Normalizes the vector in-place. */
		void Normalize();
		/** @brief Returns a normalized copy of the vector. */
		Vector3 Normalized() const;

		/** @brief Computes the dot product with another vector. */
		float DotProduct(const Vector3& rhs) const;
		/** @brief Computes the 3D cross product with another vector. */
		Vector3 CrossProduct(const Vector3& rhs) const; 

		//Index Access
		/** @brief Mutable index access. */
		float& operator[](int i);
		/** @brief Const index access. */
		const float& operator[](int i) const;

		//Util
		/**
		 * @brief Set the value of an already created vector.
		 * @param nx New X value.
		 * @param ny New Y value.
		 * @param nz New Z value.
		 */
		void Set(float nx, float ny, float nz);
		
		/** @brief Sets all components to 0. */
		void Zero();

		//For debug Purposes
		//THis lets us print the vect with cout, however, given the circumstances
		//it must be a free func that takes in both os and v as args. Else it would be v << os not os <<v
		//however, if we want to print private members of the class, and since this is inside the class, we have to use friend.
		//But if dw friend, just put it outside the class. eg. std::ostream& operator<<(std::ostream& os, const Vector3& v);
		friend std::ostream& operator<<(std::ostream& os, const Vector3& v);

		// --- Free scalar operators ---
		friend Vector3 operator+(float scalar, const Vector3& v);
		friend Vector3 operator*(float scalar, const Vector3& v);
};