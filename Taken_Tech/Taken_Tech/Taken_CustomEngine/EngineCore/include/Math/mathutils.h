#pragma once
/**
 * @file    mathutils.h
 * @author  Jethro Sung
 * @email   sung.h
 * @date    2025-09-29
 *
 * @brief   Provides common math constants and utility functions.
 *
 * Includes degree<->radian conversion, clamp, min/max, lerp,
 * smoothstep functions, and random number generation. Also
 * supports Vector2/Vector3 variants for interpolation.
 *
 * @version 1.0
 */
#include <cmath>
#include <cstdlib> //rand()
#include "Math/vect2.h"
#include "Math/vect3.h"

namespace Math {
	constexpr float PI = static_cast < float>(3.14159265359);
	constexpr float DegToRad = PI / 180.f;
	constexpr float RadToDeg = 180.f / PI;

	// Angle Convert Funcs
	inline float ToRad(float deg) { return deg * DegToRad; }
	inline float ToDeg(float rad) { return rad * RadToDeg; }

	//Clamp min max
	inline float Clamp(float a, float low, float high) { return (a < low) ? low : (a > high) ? high : a; }//This is to restrict a to min low and max high
	inline float min(float a, float b) { return (a < b) ? a : b; }
	inline float max(float a, float b) { return (a > b) ? a : b; }

	//linear interpolate, finds a val controlled by t in [a,b], if t = 0, ret a, if t = 1, ret b, if t = 0.5, midpt between a and b, t is weight
	inline float lerp(float a, float b, float t) { return a + t * (b - a); }

	//vect2 Lerp
	inline Vector2 lerp(const Vector2& a, const Vector2& b, float t) {
		return a + t * (b - a);
	}

	//vect 3 lerp
	inline Vector3 lerp(const Vector3& a, const Vector3& b, float t) {
		return a + t * (b - a);
	}

	//smoothstep, like lerp, but starts slow, speeds up, and slows again, t is time
	inline float SmoothStep(float a, float b, float t) {
		t = Clamp(t, 0.0f, 1.0f);
		t = t * t * (3 - 2 * t);
		return lerp(a, b, t);
	}

	inline Vector2 SmoothStep(const Vector2& a, const Vector2& b, float t) {
		t = Clamp(t, 0.0f, 1.0f);
		t = t * t * (3 - 2 * t);
		return lerp(a, b, t);
	}

	inline Vector3 SmoothStep(const Vector3& a, const Vector3& b, float t) {
		t = Clamp(t, 0.0f, 1.0f);
		t = t * t * (3 - 2 * t);
		return lerp(a, b, t);
	}

	inline float SmootherStep(float a, float b, float t) {
		t = Clamp(t, 0.0f, 1.0f);
		t = t * t * t * (t * (t * 6 - 15) + 10); // 6t^5 - 15t^4 + 10t^3
		return lerp(a, b, t);
	}

	inline Vector2 SmootherStep(const Vector2& a, const Vector2& b, float t) {
		t = Clamp(t, 0.0f, 1.0f);
		t = t * t * t * (t * (t * 6 - 15) + 10);
		return lerp(a, b, t);
	}

	inline Vector3 SmootherStep(const Vector3& a, const Vector3& b, float t) {
		t = Clamp(t, 0.0f, 1.0f);
		t = t * t * t * (t * (t * 6 - 15) + 10);
		return lerp(a, b, t);
	}

	//Random 0,1
	float Rand01();

	//rand min, max
	float RandRange(float min, float max);
}