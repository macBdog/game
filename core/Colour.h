#ifndef _CORE_COLOUR_
#define _CORE_COLOUR_
#pragma once

#include <stdio.h>

#include "MathUtils.h"

//\brief Basic 4 value colour class
class Colour
{
public:

	// Constructors
	inline Colour() { r = 0.0f; g = 0.0f; b = 0.0f; a = 0.0f; }
	inline Colour(float a_val) { r = a_val; g = a_val; b = a_val; a = a_val; }
	inline Colour(float a_r, float a_g, float a_b, float a_a) { r = a_r; g = a_g; b = a_b; a = a_a; }
	inline Colour(int a_r, int a_g, int a_b, int a_a) { r = a_r/255.0f; g = a_g/255.0f; b = a_b/255.0f, a = a_a/255.0f; }

	// Mutators
	inline void SetR(float a_val) { r = a_val; }
	inline void SetG(float a_val) { g = a_val; }
	inline void SetB(float a_val) { b = a_val; }
	inline void SetA(float a_val) { a = a_val; }
	inline float GetR() const { return r; }
	inline float GetG() const { return g; }
	inline float GetB() const { return b; }
	inline float GetA() const { return a; }
	inline float * GetValues() { return &r; }
	inline void GetString(char * a_buf_OUT) const { sprintf(a_buf_OUT, "%f, %f, %f, %f", r, g, b, a); }
	inline void GetStringAsRGBAInt(char * a_buf_OUT) const { sprintf(a_buf_OUT, "%d, %d, %d, %d", (int)(r*255.0f), (int)(g*255.0f), (int)(b*255.0f), (int)(a*255.0f)); }

	//\brief Standard arithmatic operator overloads
	Colour operator + (const Colour & a_val) const { return Colour(r + a_val.r, g + a_val.g, b + a_val.b, a + a_val.a); }
	Colour operator - (const Colour & a_val) const { return Colour(r - a_val.r, g - a_val.g, b - a_val.b, a - a_val.a); }
	Colour operator * (float a_scale) const { return Colour(r * a_scale, g * a_scale, b * a_scale, a * a_scale); }
	Colour operator * (const Colour & a_val) const { return Colour(r * a_val.r, g * a_val.g, b * a_val.b, a * a_val.a); }
	bool operator == (const Colour & a_compare) const { return r == a_compare.r && g == a_compare.g && b == a_compare.b && a == a_compare.a; }
	bool operator > (const Colour & a_compare) const { return r > a_compare.r && g > a_compare.g && b > a_compare.b && a > a_compare.a; }
	bool operator < (const Colour & a_compare) const { return r < a_compare.r && g < a_compare.g && b < a_compare.b && a < a_compare.a; }
	void operator += (const Colour & a_val) { r += a_val.r; g += a_val.g; b += a_val.b; a += a_val.a; }
	void operator -= (const Colour & a_val) { r -= a_val.r; g -= a_val.g; b -= a_val.b; a -= a_val.a; }

	static void HSVtoRGB(float hue, float saturation, float val, float& red_OUT, float& green_OUT, float& blue_OUT)
	{
		// Branchless conversion lifted from http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl 
		const float cKx = hue + 1.0f;
		const float cKy = hue + (2.0f / 3.0f);
		const float cKz = hue + (1.0f / 3.0f);
		const float fractPx = cKx - floorf(cKx);
		const float fractPy = cKy - floorf(cKy);
		const float fractPz = cKz - floorf(cKz);
		const float pX = fabs(fractPx * 6.0f - 3.0f);
		const float pY = fabs(fractPy * 6.0f - 3.0f);
		const float pZ = fabs(fractPz * 6.0f - 3.0f);
		red_OUT = val * MathUtils::LerpFloat(1.0f, MathUtils::Clamp(0.0f, pX - 1.0f, 1.0f), saturation);
		green_OUT = val * MathUtils::LerpFloat(1.0f, MathUtils::Clamp(0.0f, pY - 1.0f, 1.0f), saturation);
		blue_OUT = val * MathUtils::LerpFloat(1.0f, MathUtils::Clamp(0.0f, pZ - 1.0f, 1.0f), saturation);
	}

	static void RGBtoHSV(float red, float green, float blue, float& hue_OUT, float& saturation_OUT, float& value_OUT)
	{
		// Fast conditional conversion lifted from http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
		const float px = green < blue ? blue : green;
		const float py = green < blue ? green : blue;
		const float pz = green < blue ? -1.0f : 0.0f;
		const float pw = green < blue ? 2.0f / 3.0f : 1.0f / 3.0f;
		const float qx = red < px ? px : red;
		const float qy = red < px ? py : py;
		const float qz = red < px ? pw : pz;
		const float qw = red < px ? red : px;
		const float d = qx - MathUtils::GetMin(qw, qy);
		const float e = 1.0e-10f;
		hue_OUT = fabsf(qz + (qw - qy) / (6.0f * d + e));
		saturation_OUT = d / (qx + e);
		value_OUT = qx;
	}

private:
	float r, g, b, a;

};

// Statics for commonly referenced debug colours
static const Colour sc_colourWhite			= Colour(1.0f, 1.0f, 1.0f, 1.0f);
static const Colour sc_colourBlack          = Colour(0.0f, 0.0f, 0.0f, 1.0f);
static const Colour sc_colourGrey			= Colour(0.5f, 0.5f, 0.5f, 1.0f);
static const Colour sc_colourGreyAlpha		= Colour(0.2f, 0.2f, 0.2f, 0.4f);
static const Colour sc_colourRed            = Colour(1.0f, 0.0f, 0.0f, 1.0f);
static const Colour sc_colourGreen          = Colour(0.0f, 1.0f, 0.0f, 1.0f);
static const Colour sc_colourBlue           = Colour(0.0f, 0.0f, 1.0f, 1.0f);
static const Colour sc_colourSkyBlue        = Colour(0.0f, 0.5f, 1.0f, 1.0f);
static const Colour sc_colourPurple         = Colour(1.0f, 0.0f, 1.0f, 1.0f);
static const Colour sc_colourPink			= Colour(1.0f, 0.5f, 0.9f, 1.0f);
static const Colour sc_colourYellow			= Colour(1.0f, 1.0f, 0.0f, 1.0f);
static const Colour sc_colourOrange			= Colour(1.0f, 0.33f, 0.0f, 1.0f);
static const Colour sc_colourBrown			= Colour(0.625f, 0.2f, 0.0f, 1.0f);
static const Colour sc_colourMauve			= Colour(0.35f, 0.1875f, 1.0f, 1.0f);

#endif //_CORE_COLOUR_
