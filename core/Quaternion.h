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
	Quaternion(const Vector & a_axis, float a_angle) {	w = cosf(a_angle*0.5f);
							x = a_axis.GetX() * sinf(a_angle*0.5f);
							y = a_axis.GetY() * sinf(a_angle*0.5f);
							z = a_axis.GetZ() * sinf(a_angle*0.5f);  }
	explicit Quaternion(const Vector & a_eulerAngles) { *this = FromEulerAngles(a_eulerAngles); }
	Quaternion(const Vector & a_vec1, const Vector & a_vec2) { *this = FromAngles(a_vec1, a_vec2); }
	explicit Quaternion(const Matrix & a_mat)
	{
		const float m0 = a_mat.GetValue(0);
		const float m11 = a_mat.GetValue(5);
		const float m22 = a_mat.GetValue(10);
		x = sqrt(MathUtils::GetMax(0.0f, 1.0f + m0 - m11 - m22)) * 0.5f;
		y = sqrt(MathUtils::GetMax(0.0f, 1.0f - m0 + m11 - m22)) * 0.5f;
		z = sqrt(MathUtils::GetMax(0.0f, 1.0f - m0 - m11 + m22)) * 0.5f;
		w = sqrt(MathUtils::GetMax(0.0f, 1.0f + m0 + m11 + m22)) * 0.5f;
	}

	// Mutators
	void Normalise() { const float magRecip = 1.0f / GetMagnitude(); w *= magRecip; x *= magRecip; y *= magRecip; z *= magRecip; }

	// Utility functions
	float GetMagnitude() const { return sqrt((w*w) + (x*x) + (y*y) + (z*z)); }
	float GetMagnitudeSquared() const { return (w*w) + (x*x) + (y*y) + (z*z); }
	void GetAxisAngle(Vector & a_axis, float & a_angle) const
	{
	  float len;

	  len = x*x + y*y + z*z;

	  if (len > EPSILON) 
	  {
		float lenRecip = 1.0f / len;
		a_axis = Vector(x * lenRecip, y * lenRecip, z * lenRecip);
		a_axis.Normalise();
		// Limit due to floating point error in max range for acos
		a_angle = (2.0f * acos(MathUtils::Clamp(-1.0f, w, 1.0f)));
	  }
	  else
	  {
		a_axis = Vector(0.0f, 0.0f, 1.0f);
		a_angle = 0;
	  }
	}
	Quaternion FromAxisAngle(Vector a_axis, float a_angle) 
	{ 
		float sin_a = sinf(a_angle / 2.0f);
		float cos_a = cosf(a_angle / 2.0f);
		return Quaternion(a_axis * sin_a, cos_a);
	}
	Quaternion FromEulerAngles(const float & a_x, const float & a_y, const float & a_z)
	{
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
	Quaternion FromEulerAngles(const Vector & a_eulerAngles)
	{
		return FromEulerAngles(a_eulerAngles.GetX(), a_eulerAngles.GetY(), a_eulerAngles.GetZ());
	}
	Quaternion FromAngles(const Vector & a_u, const Vector & a_v)
	{
	    Vector w = a_u.Cross(a_v);
	    Quaternion q = Quaternion(1.0f + a_u.Dot(a_v), w.GetX(), w.GetY(), w.GetZ());
	    q.Normalise();
	    return q;
	}

	bool IsUnit() const { return abs(1.0f - ((w*w) + (x*x) + (y*y) + (z*z))) > EPSILON; }
	inline void ApplyToMatrix(Matrix & a_mat) { a_mat = a_mat.Multiply(GetRotationMatrix()); }
	inline void GetString(char * a_buf_OUT) const { sprintf(a_buf_OUT, "%f, %f, %f, %f", x, y, z, w); }
	inline Vector GetXYZ() const { return Vector(x, y, z); }
	inline float GetX() const { return x; }
	inline float GetY() const { return y; }
	inline float GetZ() const { return z; }
	inline float GetW() const { return w; }
	inline bool GetAxisAngle(Vector & a_axis_OUT, float & a_angle_OUT)
	{
		const float cos_a = w;
		float r = 1.0f - cos_a * cos_a;

		// Early out for no rotation
		if (r <= 0.0f) 
		{
			a_axis_OUT = Vector(1.0f, 0.0f, 0.0f);
			a_angle_OUT = 0.0f;
			return false;
		}

		float sin_a = sqrtf(r);
		if (fabs( sin_a ) < 0.0005f)
		{
			sin_a = 1.0f;
		}
		a_angle_OUT = acosf(MathUtils::Clamp(-1.0f, cos_a, 0.9999f)) * 2.0f;
		a_axis_OUT = Vector(x, y, z) / sin_a;
		return true;
	}

	Matrix GetRotationMatrix() const { 

		float tx = 2.0f * x;
		float ty = 2.0f * y;
		float tz = 2.0f * z;
		float twx = tx * w;
		float twy = ty * w;
		float twz = tz * w;
		float txx = tx * x;
		float txy = ty * x;
		float txz = tz * x;
		float tyy = ty * y;
		float tyz = tz * y;
		float tzz = tz * z;
  
		return Matrix(	1.0f - (tyy + tzz),	txy - twz,			txz + twy,			0.0f,
						txy + twz,			1.0f - (txx + tzz),	tyz - twx,			0.0f,
						txz - twy,			tyz + twx,			1.0f - (txx + tyy), 0.0f,
						0.0f,				0.0f,				0.0f,				1.0f);
	}

	// Operator overloads
	Quaternion operator * (const Quaternion & a_quat) const { return Quaternion(	(w*a_quat.w - x*a_quat.x - y*a_quat.y - z*a_quat.z), 
																					(w*a_quat.x + x*a_quat.w + y*a_quat.z - z*a_quat.y),
																					(w*a_quat.y - x*a_quat.z + y*a_quat.w + z*a_quat.x),
																					(w*a_quat.z + x*a_quat.y - y*a_quat.x + z*a_quat.w)	); }
	Quaternion operator *= (const Quaternion & a_quat) const { return Quaternion(x, y, z, w) * a_quat; }
private:
	float x, y, z, w;
};

#endif //_CORE_QUATERNION_
