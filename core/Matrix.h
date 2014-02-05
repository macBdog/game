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
	Matrix(	float a_0, float a_1, float a_2, float a_3, 
			float a_4, float a_5, float a_6, float a_7, 
			float a_8, float a_9, float a_10, float a_11, 
			float a_12, float a_13, float a_14, float a_15) 
			{ 
				f[0]=a_0; f[1]=a_1; f[2]=a_2; f[3]=a_3; 
				f[4]=a_4; f[5]=a_5; f[6]=a_6; f[7]=a_7; 
				f[8]=a_8; f[9]=a_9; f[10]=a_10; f[11]=a_11; 
				f[12]=a_12; f[13]=a_13; f[14]=a_14; f[15]=a_15; 
			}
	inline float GetValue(unsigned int a_row, unsigned int a_col) const			{ return row[a_row][a_col]; }
	inline float GetValue(unsigned int a_index) const							{ return f[a_index]; }
	inline float * GetValues() { return &f[0]; }
	inline void SetValue(int a_index, float a_val) { if (a_index >= 0 && a_index <= 15) { f[a_index] = a_val; } }
	inline void SetRight(Vector a_right,	float a_w = 0.0f) { right = a_right; rightW = a_w;}
	inline void SetLook(Vector a_look,		float a_w = 0.0f) { look = a_look; lookW = a_w;}
	inline void SetUp(Vector a_up,			float a_w = 0.0f) { up = a_up; upW = a_w;}
	inline void SetPos(Vector a_pos,		float a_w = 1.0f) { pos = a_pos; posW = a_w;}
	inline void SetPosX(float a_posX) { pos.SetX(a_posX); }
	inline void SetPosY(float a_posY) { pos.SetY(a_posY); }
	inline void SetPosZ(float a_posZ) { pos.SetZ(a_posZ); }
	inline void Translate(Vector a_trans) { pos += a_trans; }
	inline Vector GetRight() const	{ return right; }
	inline Vector GetLook()	const	{ return look; }
	inline Vector GetUp() const		{ return up; }
	inline Vector GetPos() const	{ return pos; }
	inline float GetDeterminant() const { return 0.0f; } //TODO
	inline bool HasInverse() const { return GetDeterminant() > 0.0f; }
	inline void SetIdentity()
	{
		f[0] = 1.0f;	f[1] = 0.0f;	f[2] = 0.0f;	f[3] = 0.0f;
		f[4] = 0.0f;	f[5] = 1.0f;	f[6] = 0.0f;	f[7] = 0.0f;
		f[8] = 0.0f;	f[9] = 0.0f;	f[10] = 1.0f;	f[11] = 0.0f;
		f[12] = 0.0f;	f[13] = 0.0f;	f[14] = 0.0f;	f[15] = 1.0f;
	}
	static const Matrix Identity() 
	{ 
		float vals[16] = {	1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f };
		return Matrix(vals);
	}
	inline Matrix Multiply(const Matrix & a_mat) const
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
	inline Matrix Add(const Matrix & a_mat, bool a_addW = false) const
	{
		Matrix mOut;
		for (int i = 0; i < 16; ++i)
		{
			mOut.f[i] = a_mat.f[i] + f[i];
		}
		if (!a_addW)
		{
			mOut.row[0][3] = row[0][3];
			mOut.row[1][3] = row[1][3];
			mOut.row[2][3] = row[2][3];
			mOut.row[3][3] = row[3][3];
		}
		return mOut;
	}
	inline Matrix Sub(const Matrix & a_mat, bool a_subW = false) const
	{
		Matrix mOut;
		for (int i = 0; i < 16; ++i)
		{
			mOut.f[i] = a_mat.f[i] - f[i];
		}
		if (!a_subW)
		{
			mOut.row[0][3] = row[0][3];
			mOut.row[1][3] = row[1][3];
			mOut.row[2][3] = row[2][3];
			mOut.row[3][3] = row[3][3];
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
	inline Vector Transform(const Vector & a_vec)
	{
		return Vector(	a_vec.GetX() * row[0][0] + a_vec.GetY() * row[1][0] + a_vec.GetZ() * row[2][0] + row[3][0],
						a_vec.GetX() * row[0][1] + a_vec.GetY() * row[1][1] + a_vec.GetZ() * row[2][1] + row[3][1],
						a_vec.GetX() * row[0][2] + a_vec.GetY() * row[1][2] + a_vec.GetZ() * row[2][2] + row[3][2]);
	}
	inline void SetScale(const float & a_scalar) 
	{ 
		row[0][0] *= a_scalar;
		row[1][1] *= a_scalar;
		row[2][2] *= a_scalar;
	}
	inline void SetScale(const Vector & a_scalar) 
	{ 
		row[0][0] *= a_scalar.GetX();
		row[1][1] *= a_scalar.GetY();
		row[2][2] *= a_scalar.GetZ();
	}
	inline Vector GetScale() const
	{
		return Vector(right.Length(), look.Length(), up.Length());
	}
	inline void RemoveScale()
	{
		right.Normalise();
		look.Normalise();
		up.Normalise();
	}

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