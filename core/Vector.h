#ifndef _CORE_VECTOR_
#define _CORE_VECTOR_
#pragma once

#include <math.h>
#include <stdio.h>

//\brief Basic vector class, W component is only represented in matrices
class Vector
{
public:

	// Constructors
	Vector() { x = 0; y = 0; z = 0; }
	Vector(float a_val) { x = a_val; y = a_val; z = a_val; }
	Vector(float a_x, float a_y, float a_z) { x = a_x; y = a_y; z = a_z; }

	// Mutators
	inline void SetX(float a_x) { x = a_x; }
	inline void SetY(float a_y) { y = a_y; }
	inline void SetZ(float a_z) { z = a_z; }
	inline float GetX() const { return x; }
	inline float GetY() const { return y; }
	inline float GetZ() const { return z; }
	inline float * GetValues() { return &x; }
	inline void GetString(char * a_buf_OUT) const { sprintf(a_buf_OUT, "%f, %f, %f", x, y, z); }

	// Utility functions
	const float LengthSquared() const { return x*x + y*y + z*z; }
	float Length() const { return sqrt(LengthSquared()); }
	bool IsEqualZero() const { return x + y + z == 0.0f; }
	float Dot(const Vector & a_vec) const { return x * a_vec.x + y * a_vec.y + z * a_vec.z; }
	Vector Cross(const Vector & a_vec) const { return Vector(((y * a_vec.z) - (z * a_vec.y)),  ((z * a_vec.x) - (x * a_vec.z)), ((x * a_vec.y) - (y * a_vec.x))); }
	void Normalise() { float fLen = Length(); if (fLen > 0.0f) { x = x / fLen; y = y / fLen; z = z / fLen; } }
	static Vector Zero() { return Vector(0.0f, 0.0f, 0.0f); }
	static Vector Up() { return Vector(0.0f, 0.0f, 1.0f); }
	bool IsSmallerMagnitude (const Vector & a_compare) const { return LengthSquared() < a_compare.LengthSquared(); }
	bool IsGreaterMagnitude (const Vector & a_compare) const { return LengthSquared() > a_compare.LengthSquared(); }

	// Operator overloads
	Vector operator + (const Vector & a_val) const { return Vector(x + a_val.x, y + a_val.y, z + a_val.z); }
	Vector operator + (const float & a_val) const { return Vector(x + a_val, y + a_val, z + a_val); }
	Vector operator - (const Vector & a_val) const { return Vector(x - a_val.x, y - a_val.y, z - a_val.z); }
	Vector operator - (const float & a_val) const { return Vector(x - a_val, y - a_val, z - a_val); }
	Vector operator * (const float & a_scale) const { return Vector(x * a_scale, y * a_scale, z * a_scale); }
	Vector operator / (const float & a_scale) const { return Vector(x / a_scale, y / a_scale, z / a_scale); }
	Vector operator * (const Vector & a_val) const { return Vector(x * a_val.x, y * a_val.y, z * a_val.z); }
	Vector operator - () const { return Vector(-x, -y, -z); }
	bool operator == (const Vector & a_compare) const { return x == a_compare.x && y == a_compare.y && z == a_compare.z; }
	void operator += (const Vector & a_val) { x += a_val.x; y += a_val.y; z += a_val.z; }
	void operator -= (const Vector & a_val) { x -= a_val.x; y -= a_val.y; z -= a_val.z; }
	

private:
	float x, y, z;
};

//\brief Simple 2 component vector class
class Vector2
{
public:

	// Constructors
	Vector2() { x = 0; y = 0; }
	Vector2(float a_val) { x = a_val; y = a_val; }
	Vector2(float a_x, float a_y) { x = a_x; y = a_y; }

	// Mutators
	inline void SetX(float a_x) { x = a_x; }
	inline void SetY(float a_y) { y = a_y; }
	inline float GetX() const { return x; }
	inline float GetY() const { return y; }
	inline void GetString(char * a_buf_OUT) const { sprintf(a_buf_OUT, "%f, %f", x, y); }
	static Vector2 Vector2Zero() { return Vector2(0.0f, 0.0f); }
	void Normalise() { float fLen = Length(); if (fLen > 0.0f) { x = x / fLen; y = y / fLen; } }

	// Utility functions
	const float LengthSquared() const { return x*x + y*y; }
	float Length() const { return sqrt(LengthSquared()); }

	// Operator overloads
	Vector2 operator * (const Vector2 & a_val) const { return Vector2(x * a_val.x, y * a_val.y); }
	Vector2 operator * (const float & a_scale) const { return Vector2(x * a_scale, y * a_scale); }
	Vector2 operator + (const Vector2 & a_val) const { return Vector2(x + a_val.x, y + a_val.y); }
	Vector2 operator - (const Vector2 & a_val) const { return Vector2(x - a_val.x, y - a_val.y); }
	Vector2 operator - () const { return Vector2(-x, -y); }
	Vector2 operator *= (const float & a_scale) const { return Vector2(x + x*a_scale, y + y*a_scale); }
	void  operator += (const Vector2 & a_val) { x += a_val.x; y += a_val.y; }
	void  operator -= (const Vector2 & a_val) { x -= a_val.x; y -= a_val.y; }

protected:
	float x, y;
};

//\ brief Simple 2D Vector specialised for use in texture coordinates
class TexCoord : public Vector2
{
public:

	TexCoord() { x = 0.0f; y = 0.0f; }
	TexCoord(float a_x, float a_y) { x = a_x; y = a_y; }

	inline float CalcBounds() { return x * y; }

	// Operator overloads are not inherited by default
	using Vector2::operator=;
	using Vector2::operator*;
	using Vector2::operator+;
	using Vector2::operator-;
};

#endif //_CORE_VECTOR_