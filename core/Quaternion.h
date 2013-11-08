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
	Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) { }
	Quaternion(	const float & a_x, 
			const float & a_y, 
			const float & a_z, 
			const float & a_w) : x(a_x), y(a_y), z(a_z), w(a_w) { }
	Quaternion(Vector a_axis, float a_angle) 
	{ 
		// From axis angle
		float sin_a = sinf(a_angle / 2.0f);
		float cos_a = cosf(a_angle / 2.0f);
		return Quaternion(a_axis * sin_a, cos_a);
	}
	Quaternion(const Vector & a_eulerAngles)
	{
		return Quaternion(a_eulerAngles.GetX(), a_eulerAngles.GetY(), a_eulerAngles.GetZ())
	}
	Quaternion(const float & a_x, const float & a_y, const float & a_z)
	{
		// From euler degrees
		float cos_z_2 = cosf(0.5f * a_z);
		float cos_y_2 = cosf(0.5f * a_y);
		float cos_x_2 = cosf(0.5f * a_x);

		float sin_z_2 = sinf(0.5f * a_z);
		float sin_y_2 = sinf(0.5f * a_y);
		float sin_x_2 = sinf(0.5f * a_x);

		return Quaternion(
			cos_z_2*cos_y_2*sin_x_2 - sin_z_2*sin_y_2*cos_x_2,
			cos_z_2*sin_y_2*cos_x_2 + sin_z_2*cos_y_2*sin_x_2,
			sin_z_2*cos_y_2*cos_x_2 - cos_z_2*sin_y_2*sin_x_2,
			cos_z_2*cos_y_2*cos_x_2 + sin_z_2*sin_y_2*sin_x_2);
	}
	Quaternion(const Vector & a_u, const Vector & a_v)
	{
		// From two vectors
	    	Vector w = a_u.Cross(a_v);
	    	Quaternion q = Quaternion(1.0f + a_u.Dot(v), w.x, w.y, w.z);
	    	q.Normalise();
	    	return q;
	}
	
	Quaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) { }
	Quaternion(float a_w, float a_x, float a_y, float a_z) : w(a_w), x(a_x), y(a_y), z(a_z) { }
	Quaternion(Vector a_axis, float a_angle) {	w = cosf(a_angle*0.5f);
							x = a_axis.GetX() * sinf(a_angle*0.5f);
							y = a_axis.GetY() * sinf(a_angle*0.5f);
							z = a_axis.GetZ() * sinf(a_angle*0.5f);  }

	// Utility functions
	float GetMagnitude() const { return sqrt((w*w) + (x*x) + (y*y) + (z*z)); }
	bool IsUnit() const { return abs(1.0f - ((w*w) + (x*x) + (y*y) + (z*z))) > EPSILON; }
		inline ApplyToMatrix(Matrix & a_mat) 
	{

	}
	inline Vector GetXYZ() const { return Vector(x, y, z); }
	inline bool GetAxisAngle(Vector & a_axis_OUT, float & a_angle_OUT)
	{
		const float cos_a = w;
		float r = 1.0f - cos_a * cos_a;

		// Early out for no rotation
		if (r <= 0.0f) 
		{
			a_axis_OUT = Vectormath::Aos::Vector3(1.0f, 0.0f, 0.0f);
			a_angle_OUT = 0.0f;
			return false;
		}

		float sin_a = sqrtf(r);
		if (fabs( sin_a ) < 0.0005f)
		{
			sin_a = 1.0f;
		}
		a_angle_OUT = acosf(paMath::ClampValue(cos_a, 0.9999f)) * 2.0f;
		a_axis_OUT = Vector(x, y, z) / sin_a;
		return true;
	}
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
	float x, y, z, w;
};

#endif //_CORE_QUATERNION_
