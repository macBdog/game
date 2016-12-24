#ifndef _CORE_MATRIX_
#define _CORE_MATRIX_
#pragma once

#include <math.h>

#include "MathUtils.h"
#include "Vector.h"

//\brief Basic 4x4 matrix with row major layout in memory
class Matrix
{
public:
	Matrix() {};
	explicit Matrix(float a_f[16]) { for (int i = 0; i < 16; ++i) { f[i] = a_f[i]; } }
	Matrix(	float a_0, float a_1, float a_2, float a_3, 
			float a_4, float a_5, float a_6, float a_7, 
			float a_8, float a_9, float a_10, float a_11, 
			float a_12, float a_13, float a_14, float a_15) 
			{ 
				f[0]=a_0;	f[1]=a_1;	f[2]=a_2;	f[3]=a_3; 
				f[4]=a_4;	f[5]=a_5;	f[6]=a_6;	f[7]=a_7; 
				f[8]=a_8;	f[9]=a_9;	f[10]=a_10; f[11]=a_11; 
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
	inline Matrix GetInverse() const 
	{
		float inv[16];
		inv[0] = f[5]  * f[10] * f[15] - 
				 f[5]  * f[11] * f[14] - 
				 f[9]  * f[6]  * f[15] + 
				 f[9]  * f[7]  * f[14] +
				 f[13] * f[6]  * f[11] - 
				 f[13] * f[7]  * f[10];

		inv[4] = -f[4]  * f[10] * f[15] + 
				  f[4]  * f[11] * f[14] + 
				  f[8]  * f[6]  * f[15] - 
				  f[8]  * f[7]  * f[14] - 
				  f[12] * f[6]  * f[11] + 
				  f[12] * f[7]  * f[10];

		inv[8] = f[4]  * f[9] * f[15] - 
				 f[4]  * f[11] * f[13] - 
				 f[8]  * f[5] * f[15] + 
				 f[8]  * f[7] * f[13] + 
				 f[12] * f[5] * f[11] - 
				 f[12] * f[7] * f[9];

		inv[12] = -f[4]  * f[9] * f[14] + 
				   f[4]  * f[10] * f[13] +
				   f[8]  * f[5] * f[14] - 
				   f[8]  * f[6] * f[13] - 
				   f[12] * f[5] * f[10] + 
				   f[12] * f[6] * f[9];

		inv[1] = -f[1]  * f[10] * f[15] + 
				  f[1]  * f[11] * f[14] + 
				  f[9]  * f[2] * f[15] - 
				  f[9]  * f[3] * f[14] - 
				  f[13] * f[2] * f[11] + 
				  f[13] * f[3] * f[10];

		inv[5] = f[0]  * f[10] * f[15] - 
				 f[0]  * f[11] * f[14] - 
				 f[8]  * f[2] * f[15] + 
				 f[8]  * f[3] * f[14] + 
				 f[12] * f[2] * f[11] - 
				 f[12] * f[3] * f[10];

		inv[9] = -f[0]  * f[9] * f[15] + 
				  f[0]  * f[11] * f[13] + 
				  f[8]  * f[1] * f[15] - 
				  f[8]  * f[3] * f[13] - 
				  f[12] * f[1] * f[11] + 
				  f[12] * f[3] * f[9];

		inv[13] = f[0]  * f[9] * f[14] - 
				  f[0]  * f[10] * f[13] - 
				  f[8]  * f[1] * f[14] + 
				  f[8]  * f[2] * f[13] + 
				  f[12] * f[1] * f[10] - 
				  f[12] * f[2] * f[9];

		inv[2] = f[1]  * f[6] * f[15] - 
				 f[1]  * f[7] * f[14] - 
				 f[5]  * f[2] * f[15] + 
				 f[5]  * f[3] * f[14] + 
				 f[13] * f[2] * f[7] - 
				 f[13] * f[3] * f[6];

		inv[6] = -f[0]  * f[6] * f[15] + 
				  f[0]  * f[7] * f[14] + 
				  f[4]  * f[2] * f[15] - 
				  f[4]  * f[3] * f[14] - 
				  f[12] * f[2] * f[7] + 
				  f[12] * f[3] * f[6];

		inv[10] = f[0]  * f[5] * f[15] - 
				  f[0]  * f[7] * f[13] - 
				  f[4]  * f[1] * f[15] + 
				  f[4]  * f[3] * f[13] + 
				  f[12] * f[1] * f[7] - 
				  f[12] * f[3] * f[5];

		inv[14] = -f[0]  * f[5] * f[14] + 
				   f[0]  * f[6] * f[13] + 
				   f[4]  * f[1] * f[14] - 
				   f[4]  * f[2] * f[13] - 
				   f[12] * f[1] * f[6] + 
				   f[12] * f[2] * f[5];

		inv[3] = -f[1] * f[6] * f[11] + 
				  f[1] * f[7] * f[10] + 
				  f[5] * f[2] * f[11] - 
				  f[5] * f[3] * f[10] - 
				  f[9] * f[2] * f[7] + 
				  f[9] * f[3] * f[6];

		inv[7] = f[0] * f[6] * f[11] - 
				 f[0] * f[7] * f[10] - 
				 f[4] * f[2] * f[11] + 
				 f[4] * f[3] * f[10] + 
				 f[8] * f[2] * f[7] - 
				 f[8] * f[3] * f[6];

		inv[11] = -f[0] * f[5] * f[11] + 
				   f[0] * f[7] * f[9] + 
				   f[4] * f[1] * f[11] - 
				   f[4] * f[3] * f[9] - 
				   f[8] * f[1] * f[7] + 
				   f[8] * f[3] * f[5];

		inv[15] = f[0] * f[5] * f[10] - 
				  f[0] * f[6] * f[9] - 
				  f[4] * f[1] * f[10] + 
				  f[4] * f[2] * f[9] + 
				  f[8] * f[1] * f[6] - 
				  f[8] * f[2] * f[5];

		// Matrix with a zero determinant has no computable inverse
		float det;
		det = f[0] * inv[0] + f[1] * inv[4] + f[2] * inv[8] + f[3] * inv[12];
		if (det == 0.0f)
		{
			return Identity();
		}

		Matrix out;
		det = 1.0f / det;
		for (int i = 0; i < 16; i++)
		{
			out.f[i] = inv[i] * det;
		}
		return out;
	}

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
	static const Matrix Frustum(float a_left, float a_right, float a_bottom, float a_top, float a_near, float a_far)
	{
		
		float vals[16] = {	2.0f * a_near / (a_right - a_left),		0.0f,								0.0f,										0.0f,
							0.0f,									2.0f * a_near / (a_top - a_bottom), 0.0f,										0.0f,
							(a_right + a_left) / (a_right - a_left),(a_top + a_bottom) / (a_top - a_bottom), (a_near + a_far) / (a_near - a_far),	-1.0f,
							0.0f,									0.0f,								2.0f * a_near * a_far / (a_near - a_far),	0.0f };
		return Matrix(vals);
	}
	static const Matrix Perspective(float a_fovY, float a_aspect, float a_zNear, float a_zFar)
	{
		float m[16];
		float ymax = a_zNear * tan(a_fovY * 0.5f * PI/180.0f);
		float ymin = -ymax;
		float xmax = ymax * a_aspect;
		float xmin = ymin * a_aspect;

		float width = xmax - xmin;
		float height = ymax - ymin;

		float depth = a_zFar - a_zNear;
		float q = -(a_zFar + a_zNear) / depth;
		float qn = -2.0f * (a_zFar * a_zNear) / depth;

		float w = 2.0f * a_zNear / width;
		w = w / a_aspect;
		float h = 2.0f * a_zNear / height;

		m[0]  = w;
		m[1]  = 0;
		m[2]  = 0;
		m[3]  = 0;

		m[4]  = 0;
		m[5]  = h;
		m[6]  = 0;
		m[7]  = 0;

		m[8]  = 0;
		m[9]  = 0;
		m[10] = q;
		m[11] = -1;

		m[12] = 0;
		m[13] = 0;
		m[14] = qn;
		m[15] = 0;

	//match the matrix order of the ortho function then get the texture and colour shader multiplication orders to match up
	//then fixup camera

		return Matrix(m);
	}
	static const Matrix Orthographic(float a_left, float a_right, float a_bottom, float a_top, float a_zNear, float a_zFar)
	{
		float vals[16] = {	2.0f/(a_right-a_left),	0.0f,					0.0f,					0.0f,
							0.0f,					2.0f/(a_top-a_bottom),	0.0f,					0.0f,
							0.0f,					0.0f,					-2.0f/(a_zFar-a_zNear), 0.0f,
							-((a_right+a_left)/(a_right-a_left)), -((a_top+a_bottom)/(a_top-a_bottom)), -((a_zFar+a_zNear)/(a_zFar/a_zNear)), 1.0f };
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
		 return Vector(	a_vec.GetX() * row[0][0] + a_vec.GetY() * row[0][1] + a_vec.GetZ() * row[0][2],
						a_vec.GetX() * row[1][0] + a_vec.GetY() * row[1][1] + a_vec.GetZ() * row[1][2],
						a_vec.GetX() * row[2][0] + a_vec.GetY() * row[2][1] + a_vec.GetZ() * row[2][2]);
	}
	inline void MulScale(const float & a_scalar) 
	{ 
		row[0][0] *= a_scalar;
		row[1][1] *= a_scalar;
		row[2][2] *= a_scalar;
	}
	inline void MulScale(const Vector & a_scalar) 
	{ 
		row[0][0] *= a_scalar.GetX();
		row[1][1] *= a_scalar.GetY();
		row[2][2] *= a_scalar.GetZ();
	}
	inline void SetScale(const float & a_scalar)
	{
		RemoveScale();
		MulScale(a_scalar);
	}
	inline void SetScale(const Vector & a_scalar)
	{
		RemoveScale();
		MulScale(a_scalar);
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
	inline Matrix GetTranspose() const
	{
		return Matrix(	f[0], f[4], f[8], f[12], 
						f[1], f[5], f[9], f[13], 
						f[2], f[6], f[10], f[14], 
						f[3], f[7], f[11], f[15]);
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
			Vector		up;		float upW;		// 0 is for vectors/directions
			Vector		pos;	float posW;		// 1 is for points.
		};
	};
};

#endif // _CORE_MATRIX_