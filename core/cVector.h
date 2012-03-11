#ifndef _CORE_VECTOR_
#define _CORE_VECTOR_
#pragma once

#include <math.h>

//\brief Basic vector class
class cVector
{
public:

	// Constructors
	cVector() { x = 0; y = 0; z = 0; }
	cVector(float a_val) { x = a_val; y = a_val; z = a_val; }
	cVector(float a_x, float a_y, float a_z) { x = a_x; y = a_y; z = a_z; }

	// Mutators
	inline void SetX(float a_x) { x = a_x; }
	inline void SetY(float a_y) { y = a_y; }
	inline void SetZ(float a_z) { z = a_z; }
	inline float GetX() { return x; }
	inline float GetY() { return y; }
	inline float GetZ() { return z; }

	// Utility functions
	const float LengthSquared() const { return x*x + y*y + z*z; }
	float Length() const { return sqrt(LengthSquared()); }
	bool IsEqualZero() const { return x + y + z == 0.0f; }
	float Dot(const cVector & a_vec) const { return x * a_vec.x + y * a_vec.y + z * a_vec.z; }
	cVector Cross(const cVector & a_vec) const { return cVector(((y * a_vec.z) - (z * a_vec.y)),  ((z * a_vec.x) - (x * a_vec.z)), ((x * a_vec.y) - (y * a_vec.x))); }

	// Operator overloads
	cVector operator + (const cVector & a_val) const { return cVector(x + a_val.x, y + a_val.y, z + a_val.z); }
	cVector operator - (const cVector & a_val) const { return cVector(x - a_val.x, y - a_val.y, z - a_val.z); }
	cVector operator * (float a_scale) const { return cVector(x * a_scale, y * a_scale, z * a_scale); }
	cVector operator * (const cVector & a_val) const { return cVector(x * a_val.x, y * a_val.y, z * a_val.z); }
	bool operator < (const cVector & a_compare) const { return LengthSquared() < a_compare.LengthSquared(); }
	bool operator > (const cVector & a_compare) const { return LengthSquared() > a_compare.LengthSquared(); }
	bool operator == (const cVector & a_compare) const { return x == a_compare.x && y == a_compare.y && z == a_compare.z; }

private:
	float x, y, z;

};

#endif //_CORE_VECTOR_