/**
 * @file    mathutils.cpp
 * @author  Jethro Sung
 * @email   sung.h
 * @date    2025-09-29
 *
 * @brief   Implements random utility functions in Math namespace.
 *
 * Includes uniform random float in [0,1] and in [min,max].
 *
 * @version 1.0
 */
#include "Math/mathutils.h"

namespace Math {
	float Rand01() {
		return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	}

	float RandRange(float min, float max)
	{
		return min + (max - min) * Rand01();
	}
}