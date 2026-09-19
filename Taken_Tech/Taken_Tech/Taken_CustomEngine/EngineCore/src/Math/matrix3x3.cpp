/**
 * @file    matrix3x3.cpp
 * @author   Lim Zhi Jie
 * @co_author Tan Wei Liang Terril
 * @email    zhijie.lim, t.weiliangterril
 * @date    2025-09-29
 *
 * @brief   Implements Matrix3x3 math functions.
 *
 * Includes constructors, multiplication, element access,
 * determinant, inverse, and projection matrix builders.
 *
 * @version 1.0
 */
#include "Math/matrix3x3.h"
#include <sstream>
#include "Input/DebugConsole.hpp"
//Constructors
Matrix3x3::Matrix3x3() {
	*this = Identity();
}

Matrix3x3::Matrix3x3(float e00, float e10, float e20,
					float e01, float e11, float e21,
					float e02, float e12, float e22) {
	m[0] = e00; m[1] = e10; m[2] = e20;
	m[3] = e01; m[4] = e11; m[5] = e21;
	m[6] = e02; m[7] = e12; m[8] = e22;
}

//Elem Access
//1st one allows for modification of the matrix elem, eg. m(1,2) = 5.f
float& Matrix3x3::operator()(unsigned int col, unsigned int row) {
	return m[col * 3 + row];
}

//for nonmodification, eg. val = m(1,2);
const float& Matrix3x3::operator()(unsigned int col, unsigned int row) const {
	return m[col * 3 + row];
}

//operators
Matrix3x3 Matrix3x3::operator*(const Matrix3x3& rhs) const {
	Matrix3x3 result;
	for (int row = 0; row < 3; ++row)
	{
		for (int col = 0; col < 3; col++)
		{
			float sum = 0.0f;
			for (int k = 0; k < 3; ++k) {
				sum += (*this)(k, row) * rhs(col, k);
			}
			result(col, row) = sum;
			//result(col, row) = (*this)(0, row) * rhs(col, 0);
		}
	}
	return result;
}

Matrix3x3 Matrix3x3::operator*=(const Matrix3x3& rhs) {
	*this = *this * rhs;
	return *this;
}

//Here we are applying a 3x3 matrix to a 2d vect
//We use this for translate, rotate, scale etc
Vector2 Matrix3x3::operator*(const Vector2& v) const {
	float xNew = (*this)(0, 0) * v.x + (*this)(1, 0) * v.y + (*this)(2, 0) * 1.f;//we use the 'z' as homogenous
	float yNew = (*this)(0, 1) * v.x + (*this)(1, 1) * v.y + (*this)(2, 1) * 1.f;
	//Since its homo, we know the third row will always be 1, thus,
	return Vector2(xNew, yNew);	
}

//Static Builders, note we do this row maj, cause its fine, as the constructor alrdy accounts for it
Matrix3x3 Matrix3x3::Identity() {
	return Matrix3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);
}

Matrix3x3 Matrix3x3::BuildTranslation(float tx, float ty) {
	return Matrix3x3(
		1, 0, 0,
		0, 1, 0,
		tx, ty, 1
	);
}

Matrix3x3 Matrix3x3::BuildRotation(float radians) {
	float c = std::cos(radians);
	float s = std::sin(radians);
	return Matrix3x3(c, -s, 0, s, c, 0, 0, 0, 1);
}

Matrix3x3 Matrix3x3::BuildScaling(float sx, float sy) {
	return Matrix3x3(sx, 0, 0, 0, sy, 0, 0, 0, 1);
}

const float* Matrix3x3::Data() const {
	return m;
}

void Matrix3x3::Print() const {
	std::stringstream ss;

	for (int row = 0; row < 3; ++row)
	{
		ss << "[ ";
		for (int col = 0; col < 3; ++col)
		{
			ss << (*this)(col, row) << " ";
		}
		ss << "]\n";
	}
	DebugConsole::Get().Info(ss.str());
}

// --- Debug print ---
std::ostream& operator<<(std::ostream& os, const Matrix3x3& m) {
	for (int row = 0; row < 3; ++row) {
		os << "[ ";
		for (int col = 0; col < 3; ++col) {
			os << m(col, row) << " ";
		}
		os << "]";
		if (row < 2) os << "\n";
	}
	return os;
}

Matrix3x3 operator*(float s, const Matrix3x3& mat) {
	Matrix3x3 result;
	for (int col = 0; col < 3; ++col) {
		for (int row = 0; row < 3; ++row) {
			result(col, row) = s * mat(col, row);
		}
	}
	return result;
}

float Matrix3x3::Determinant() const {
	return (*this)(0, 0) * ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1))
		- (*this)(1, 0) * ((*this)(0, 1) * (*this)(2, 2) - (*this)(0, 2) * (*this)(2, 1))
		+ (*this)(2, 0) * ((*this)(0, 1) * (*this)(1, 2) - (*this)(0, 2) * (*this)(1, 1));
}

Matrix3x3 Matrix3x3::Inverse() const {
	float det = Determinant();
	//Inverse is 1/det* adj M
	if (std::fabs(det)<1e-6f)
	{
		return Identity();//Singular matrix
	}

	float invDet = 1.0f / det;

	Matrix3x3 Result;

	Result(0, 0) = ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1)) * invDet; //remove row 0 and col 0 , to obtain the 2x2 det
	Result(1, 0) = -((*this)(1, 0) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 0)) * invDet;//Here we bind this to the transpose
	Result(2, 0) = ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0)) * invDet;

	Result(0, 1) = -((*this)(0, 1) * (*this)(2, 2) - (*this)(0, 2) * (*this)(2, 1)) * invDet;
	Result(1, 1) = ((*this)(0, 0) * (*this)(2, 2) - (*this)(0, 2) * (*this)(2, 0)) * invDet;
	Result(2, 1) = -((*this)(0, 0) * (*this)(2, 1) - (*this)(0, 1) * (*this)(2, 0)) * invDet;

	Result(0, 2) = ((*this)(0, 1) * (*this)(1, 2) - (*this)(0, 2) * (*this)(1, 1)) * invDet;
	Result(1, 2) = -((*this)(0, 0) * (*this)(1, 2) - (*this)(0, 2) * (*this)(1, 0)) * invDet;
	Result(2, 2) = ((*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0)) * invDet;

	return Result;
}

Matrix3x3 Matrix3x3::Ortho(float left, float right, float bottom, float top) {
	Matrix3x3 result;

	float width = right - left;
	float height = top - bottom;

	// Column-major order (OpenGL expects this)
	result.m[0] = 2.0f / width;   result.m[3] = 0.0f;            result.m[6] = -(right + left) / width;
	result.m[1] = 0.0f;           result.m[4] = 2.0f / height;   result.m[7] = -(top + bottom) / height;
	result.m[2] = 0.0f;           result.m[5] = 0.0f;            result.m[8] = 1.0f;

	return result;
}