#include "Math/vect2.h"
//Constructors
Vector2::Vector2() : mVec{0,0}, x(mVec[0]), y(mVec[1]) {}
Vector2::Vector2(float nx, float ny) : mVec{nx, ny}, x(mVec[0]), y(mVec[1]) {}
Vector2::Vector2(const Vector2& other) : mVec{ other.x , other.y }, x(mVec[0]), y(mVec[1]) {}

//Assignment
Vector2& Vector2::operator=(const Vector2& rhs) {
	if (this != &rhs)
	{
		x = rhs.x;
		y = rhs.y;
	}
	return *this;
}

//Arith
Vector2 Vector2::operator+(const Vector2& rhs) const {
	return { x + rhs.x, y + rhs.y };
}
Vector2 Vector2::operator-(const Vector2& rhs) const {
	return { x - rhs.x, y - rhs.y };
}
Vector2 Vector2::operator+(float a) const {
	return { x + a, y + a };
}
Vector2 Vector2::operator-(float a) const {
	return { x - a, y - a };
}

Vector2 Vector2::operator*(float a) const {
	return { x * a, y * a };
}
Vector2 Vector2::operator/(float a) const {
	return { x / a, y / a };
}

bool Vector2::operator==(const Vector2& rhs) const {
	return (x == rhs.x && y == rhs.y);
}

bool Vector2::operator!=(const Vector2& rhs) const {
	return (x != rhs.x && y != rhs.y);
}

Vector2& Vector2::operator+=(const Vector2& rhs) {
	x += rhs.x;
	y += rhs.y;
	return *this;
}

Vector2& Vector2::operator-=(const Vector2& rhs) {
	x -= rhs.x;
	y -= rhs.y;
	return *this;
}

Vector2& Vector2::operator*=(float scalar) {
	x *= scalar;
	y *= scalar;
	return *this;
}

Vector2& Vector2::operator/=(float scalar) {
	x /= scalar;
	y /= scalar;
	return *this;
}

Vector2& Vector2::operator+=(float scalar) {
	x += scalar;
	y += scalar;
	return *this;
}

Vector2& Vector2::operator-=(float scalar) {
	x -= scalar;
	y -= scalar;
	return *this;
}

float Vector2::Length() const {
	return std::sqrt(x * x + y * y);
}

float Vector2::LengthSqd() const {
	return (x * x + y * y);
}

void Vector2::Normalize() {
	float len = Length();
	if (len > 0.f)
	{
		x /= len;
		y /= len;
	}
}

Vector2 Vector2::Normalized() const {
	float len = Length();
	return (len > 0) ? Vector2(x / len, y / len) : Vector2();
}

float Vector2::DotProduct(const Vector2& other) const {
	return { x * other.x + y * other.y };
}

float Vector2::CrossProduct(const Vector2& other) const {
	return { x * other.y - y * other.x };
}

Vector2 Vector2::Perpendicular() const {
	return { -y, x };
}

Vector2 Vector2::Rotate(float angle) const {
	float cosA = std::cos(angle);
	float sinA = std::sin(angle);
	return { x * cosA - y * sinA, x * sinA + y * cosA };
}

//Index Op
//This is so that we can have v[0] = 10;
float& Vector2::operator[](int i) {
	if (i< 0 || i>=2)
	{
		throw std::out_of_range("Vector2 index out of range");
	}
	return mVec[i];
}

//this is for scenarios where we want like a = v[0];
const float& Vector2::operator[](int i) const {
	if (i < 0 || i>=2)
	{
		throw std::out_of_range("Vector2 index out of range");
	}
	return mVec[i];
}

//Utils
void Vector2::Set(float nx, float ny) {
	x = nx;
	y = ny;
}

void Vector2::Zero() {
	x = 0.f;
	y = 0.f;
}

std::ostream& operator<<(std::ostream& os, const Vector2& v) {
	os << '(' << v.x << ',' << v.y << ')';
	return os;
}

Vector2 operator+(float scalar, const Vector2& v) {
	return { v.x + scalar, v.y + scalar };
}

Vector2 operator-(float scalar, const Vector2& v) {
	return { scalar - v.x, scalar - v.y };
}

Vector2 operator*(float scalar, const Vector2& v) {
	return { v.x * scalar, v.y * scalar };
}