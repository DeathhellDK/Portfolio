#include "Math/vect3.h"
//Constructors

Vector3::Vector3() : mVec{ 0,0,0 }, x(mVec[0]), y(mVec[1]), z(mVec[2]) {}
Vector3::Vector3(float nx, float ny, float nz) : mVec{nx,ny,nz}, x(mVec[0]), y(mVec[1]), z(mVec[2]) {}
Vector3::Vector3(const Vector3& other) : mVec{other.x, other.y, other.z} , x(mVec[0]), y(mVec[1]), z(mVec[2]) {}

//assignments
Vector3& Vector3::operator=(const Vector3& rhs)
{
	if (this != &rhs)
	{
		x = rhs.x, y = rhs.y, z = rhs.z;
	}
	return *this;
}

Vector3 Vector3::operator+(const Vector3& rhs) const {
	return { x + rhs.x, y + rhs.y, z + rhs.z };
}
Vector3 Vector3::operator-(const Vector3& rhs) const {
	return { x - rhs.x, y - rhs.y, z - rhs.z };
}
Vector3 Vector3::operator*(float scalar) const {
	return { x*scalar, y* scalar, z* scalar };
}
Vector3 Vector3::operator/(float scalar) const {
	return { x/scalar, y / scalar, z / scalar };
}

Vector3& Vector3::operator+=(const Vector3& rhs) {
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	return *this;
}
Vector3& Vector3::operator-=(const Vector3& rhs) {
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	return *this;
}
Vector3& Vector3::operator*=(float scalar) {
	x *= scalar;
	y *= scalar;
	z *= scalar;
	return *this;
}
Vector3& Vector3::operator/=(float scalar) {
	x /= scalar;
	y /= scalar;
	z /= scalar;
	return *this;
}

//Equal
bool Vector3::operator==(const Vector3& rhs) const
{
	return rhs.x == x && rhs.y == y && rhs.z == z;
}

bool Vector3::operator!=(const Vector3& rhs) const
{
	return !(*this == rhs);
}

//Vect Operations
float Vector3::Length() const{
	return std::sqrt(x * x + y * y + z * z);
}

float Vector3::LengthSqd() const {
	return (x * x + y * y + z * z);
}

void Vector3::Normalize() {
	float len = Length();
	if (len > 0.f)
	{
		x /= len;
		y /= len;
		z /= len;
	}
}

Vector3 Vector3::Normalized() const{
	float len = Length();
	return (len > 0.f) ? Vector3(x / len, y / len, z / len) : Vector3();
}

float Vector3::DotProduct(const Vector3& rhs) const {
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

Vector3 Vector3::CrossProduct(const Vector3& rhs) const {
	return { y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x };
}

//Idx
float& Vector3::operator[](int i) {
	if (i<0 || i>=3)
	{
		throw std::out_of_range("Vector2 index out of range");
	}
	return mVec[i];
}

const float& Vector3::operator[](int i) const {
	if (i < 0 || i>=3)
	{
		throw std::out_of_range("Vector2 index out of range");
	}
	return mVec[i];
}

//Util
void Vector3::Set(float nx, float ny, float nz) {
	x = nx;
	y = ny;
	z = nz;
}

void Vector3::Zero() {
	x = y = z = 0.f;
}

// Debug print
std::ostream& operator<<(std::ostream& os, const Vector3& v) {
	os << "(" << v.x << "," << v.y << "," << v.z << ")";
	return os;
}

// Free scalar ops
Vector3 operator+(float s, const Vector3& v) { return { v.x + s, v.y + s, v.z + s }; }
Vector3 operator*(float s, const Vector3& v) { return { v.x * s, v.y * s, v.z * s }; }