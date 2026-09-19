/**
 * @file      vect2.h
 * @author	  Jethro Sung	
 * @email	  sung.h	
 * @date	  04/04/26
 *
 * @brief     Declares the Vector2 class used for 2D math operations in the
 *            engine.
 *
 *            This class provides arithmetic operators, normalization, length,
 *            dot and cross products, perpendicular-vector generation, and
 *            rotation helpers for 2D gameplay, physics, and rendering code.
*/
#pragma once
#include <iostream>
#include <cmath>
#include <stdexcept>

class Vector2 {
	private:
		float mVec[2]; //private storage
	public:
		float& x; //ref bound to mVec[0]
		float& y; //ref bound to mVec[1]
		
		//Constructors
		/** @brief Default constructor initializing to (0,0). */
		Vector2();
		
		/**
		 * @brief Constructs a vector with explicit coordinates.
		 * @param nx X coordinate.
		 * @param ny Y coordinate.
		 */
		Vector2(float nx, float ny);
		
		/**
		 * @brief Copy constructor.
		 * @param other The vector to copy from.
		 */
		Vector2(const Vector2& other);

		//Assignment
		/**
		 * @brief Assignment operator.
		 * @param rhs The right-hand side vector.
		 * @return Reference to this modified vector.
		 */
		Vector2& operator=(const Vector2& rhs);

		//Arith
		//vect with vect
		/**
		 * @brief Vector addition.
		 * @param rhs The right-hand side vector.
		 * @return A new added vector.
		 */
		Vector2 operator+(const Vector2& rhs) const;
		
		/**
		 * @brief Vector subtraction.
		 * @param rhs The right-hand side vector.
		 * @return A new subtracted vector.
		 */
		Vector2 operator-(const Vector2& rhs) const;
		
		//vect with scalar
		/**
		 * @brief Scalar addition.
		 * @param scalar The value to add.
		 * @return A new modified vector.
		 */
		Vector2 operator+(float scalar) const;
		
		/**
		 * @brief Scalar subtraction.
		 * @param scalar The value to subtract.
		 * @return A new modified vector.
		 */
		Vector2 operator-(float scalar) const;
		/**
		 * @brief Scalar multiplication.
		 * @param scalar The value to multiply by.
		 * @return A new modified vector.
		 */
		Vector2 operator*(float scalar) const;
		
		/**
		 * @brief Scalar division.
		 * @param scalar The value to divide by.
		 * @return A new modified vector.
		 */
		Vector2 operator/(float scalar) const;

		/** @brief Equality check. */
		bool operator==(const Vector2& rhs) const;
		/** @brief Inequality check. */
		bool operator!=(const Vector2& rhs) const;

		/** @brief Compound vector addition. */
		Vector2& operator+=(const Vector2& rhs);
		/** @brief Compound vector subtraction. */
		Vector2& operator-=(const Vector2& rhs);
		/** @brief Compound scalar addition. */
		Vector2& operator+=(float scalar);
		/** @brief Compound scalar subtraction. */
		Vector2& operator-=(float scalar);
		/** @brief Compound scalar multiplication. */
		Vector2& operator*=(float scalar);
		/** @brief Compound scalar division. */
		Vector2& operator/=(float scalar);

		//Vector Operations
		/** @brief Computes the length of the vector. */
		float Length() const;
		/** @brief Computes the squared length of the vector. */
		float LengthSqd() const;
		/** @brief Normalizes the vector in-place. */
		void Normalize(); // modifies the vect
		/** @brief Returns a normalized copy of the vector. */
		Vector2 Normalized() const; // creates a new normalized vect

		/** @brief Computes the dot product with another vector. */
		float DotProduct(const Vector2& rhs) const;
		/** @brief Computes the 2D cross product with another vector. */
		float CrossProduct(const Vector2& rhs) const;

		/** @brief Perpendicular Vector (90 degree CCW rotation). */
		Vector2 Perpendicular() const;
		/** @brief Rotates the vector by angle (in radians). */
		Vector2 Rotate(float angle) const;

		//Index Operator
		/** @brief Mutable index access. */
		float& operator[](int i); //non const
		/** @brief Const index access. */
		const float& operator[](int i) const; // const

		//Util
		/**
		 * @brief Set the value of an already created vector.
		 * @param nx New X value.
		 * @param ny New Y value.
		 */
		void Set(float nx, float ny); // Set the value of an alrdy created vect
		
		/** @brief Sets both components to 0. */
		void Zero();

		//For debug Purposes
		friend std::ostream& operator<<(std::ostream& os, const Vector2& v);

		// Scalar on left-hand side
		friend Vector2 operator+(float scalar, const Vector2& v);
		friend Vector2 operator-(float scalar, const Vector2& v);
		friend Vector2 operator*(float scalar, const Vector2& v);
};