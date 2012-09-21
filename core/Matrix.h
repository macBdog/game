#ifndef _CORE_MATRIX_
#define _CORE_MATRIX_
#pragma once

#include <math.h>

#include "Vector.h"

//\brief Basic 4x4 matrix
class Matrix
{
public:
	Matrix() {};
	Matrix(float a_f[16]) { for (int i = 0; i < 16; ++i) { f[i] = a_f[i]; } }
	inline float GetValue(unsigned int a_row, unsigned int a_col)			{ return row[a_row][a_col]; }
	inline float * GetValues() { return &f[0]; }
	inline void SetRight(Vector a_right,	float a_w = 0.0f) { right = a_right; rightW = a_w;}
	inline void SetLook(Vector a_look,		float a_w = 0.0f) { look = a_look; lookW = a_w;}
	inline void SetUp(Vector a_up,			float a_w = 0.0f) { up = a_up; upW = a_w;}
	inline void SetPos(Vector a_pos,		float a_w = 1.0f) { pos = a_pos; posW = a_w;}
	inline Vector GetRight() const	{ return right; }
	inline Vector GetLook()	const	{ return look; }
	inline Vector GetUp() const		{ return up; }
	inline Vector GetPos() const	{ return pos; }
	static const Matrix Identity() 
	{ 
		float vals[16] = {	1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f };
		return Matrix(vals);
	}
	inline Matrix Multiply(const Matrix & a_mat) 
	{
		Matrix mOut;
		for (unsigned int r = 0; r < 4; ++r)
		{
			for (unsigned int c = 0; c < 4; ++c)
			{
				mOut.row[r][c] = 0.0f;
				for (unsigned int i = 0; i < 4; ++i) 
				{
					mOut.row[r][c] += row[r][i] * a_mat.row[i][c];
				}
			}
		}

		return mOut;
	}
	inline static Matrix GetRotateX(float a_angleRadians)
	{
		Matrix rotMat;
		rotMat.SetRight(	Vector(1.0f,	0.0f,					0.0f));
		rotMat.SetLook(		Vector(0.0f,	cos(a_angleRadians),	sin(a_angleRadians)));
		rotMat.SetUp(		Vector(0.0f,	-sin(a_angleRadians),	cos(a_angleRadians)));
		rotMat.SetPos(	Vector::Zero());
		return rotMat;
	}
	inline static Matrix GetRotateY(float a_angleRadians)
	{
		Matrix rotMat;
		rotMat.SetRight(	Vector(cos(a_angleRadians), 0.0f,		-sin(a_angleRadians)));
		rotMat.SetLook(		Vector(0.0f,				1.0f,		0.0f));
		rotMat.SetUp(		Vector(sin(a_angleRadians),	0.0f,		cos(a_angleRadians)));
		rotMat.SetPos(	Vector::Zero());
		return rotMat;	
	}
	inline static Matrix GetRotateZ(float a_angleRadians)
	{
		Matrix rotMat;
		rotMat.SetRight(	Vector(cos(a_angleRadians), -sin(a_angleRadians),	0.0f));
		rotMat.SetLook(		Vector(sin(a_angleRadians), cos(a_angleRadians),	0.0f));
		rotMat.SetUp(		Vector(0.0f,				0.0f,					1.0f));
		rotMat.SetPos(	Vector::Zero());
		return rotMat;
	}
	inline Matrix Scale(const float & a_scalar) { } //TODO!



private:
	
	//\brief Union provided for different styles of access
	union 
	{
		float f[16];
		float row[4][4];
		struct
		{
			Vector		right;	float rightW;	// Vectors need a W component to indicate 
			Vector		look;	float lookW;	// if they are a point or a vector
			Vector		up;		float upW;		// 0 is for vectors
			Vector		pos;	float posW;		// 1 is for points.
		};
	};
};

#endif // _CORE_MATRIX_