#ifndef _CORE_QUATERNION_
#define _CORE_QUATERNION_
#pragma once

#include <math.h>
#include <stdio.h>

#include "MathUtils.h"
#include "Matrix.h"
#include "Vector.h"

//\brief Quaternion class for representing unbounded rotation in 3D space
class Quaternion
{
public:

	// Constructors
	Quaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) { }
	Quaternion(float a_w, float a_x, float a_y, float a_z) : w(a_w), x(a_x), y(a_y), z(a_z) { }
	Quaternion(Vector a_axis, float a_angle) {	w = cosf(a_angle*0.5f);
												x = a_axis.GetX() * sinf(a_angle*0.5f);
												y = a_axis.GetY() * sinf(a_angle*0.5f);
												z = a_axis.GetZ() * sinf(a_angle*0.5f);  }

	// Utility functions
	float GetMagnitude() const { return sqrt((w*w) + (x*x) + (y*y) + (z*z)); }
	bool IsUnit() const { return abs(1.0f - ((w*w) + (x*x) + (y*y) + (z*z))) > EPSILON; }
	Matrix GetRotationMatrix() const { return Matrix(	(w*w)+(x*x)-(y*y)-(z*z),	(2.0f*x*y)-(2.0f*w*z),		(2.0f*x*z)+(2.0f*w*y),		0.0f,
														(2.0f*x*y)+(2.0f*w*z),		(w*w)-(x*x)+(y*y)-(z*z),	(2.0f*y*z)+(2.0f*w*x),		0.0f,
														(2.0f*x*z)-(2.0f*w*y),		(2.0f*y*z)-(2.0f*w*x),		(w*2)-(x*x)-(y*y)+(z*z),	0.0f,
														0.0f,						0.0f						0.0f,						1.0f); }

	// Mutators
	void Normalize() { float mag = GetMagnitude(); w /= mag; x /= mag; y /= mag; z /= mag; }
	void Set(Vector a_axis, float a_angle) { Quaternion newQ(a_axis, f_angle); w = newQ.w; x = newQ.x; y = newQ.y; z = newQ.z; }

	// Operator overloads
	Quaternion operator * (const Quaternion & a_quat) const { return Quaternion(	(w*a_quat.w - x*a_quat.x - y*a_quat.y - z*a_quat.z), 
																					(w*a_quat.x + x*a_quat.w + y*a_quat.z - z*a_quat.y),
																					(w*a_quat.y - x*a_quat.z + y*a_quat.w + z*a_quat.x),
																					(w*a_quat.z + x*a_quat.y - y*a_quat.x + z*a_quat.w)	); }

private:
	float w, x, y, z;
};

#endif //_CORE_QUATERNION_