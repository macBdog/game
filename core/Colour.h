#ifndef _CORE_COLOUR_
#define _CORE_COLOUR_
#pragma once

#include <stdio.h>

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
	inline void GetString(char * a_buf_OUT) const { sprintf(a_buf_OUT, "%d, %d, %d, %d", (int)(r*255.0f), (int)(g*255.0f), (int)(b*255.0f), (int)(a*255.0f)); }

	//\brief Standard arithmatic operator overloads
	Colour operator + (const Colour & a_val) const { return Colour(r + a_val.r, g + a_val.g, b + a_val.b, a + a_val.a); }
	Colour operator - (const Colour & a_val) const { return Colour(r - a_val.r, g - a_val.g, b - a_val.b, a - a_val.a); }
	Colour operator * (float a_scale) const { return Colour(r * a_scale, g * a_scale, b * a_scale, a * a_scale); }
	Colour operator * (const Colour & a_val) const { return Colour(r * a_val.r, g * a_val.g, b * a_val.b, a * a_val.a); }
	bool operator == (const Colour & a_compare) const { return r == a_compare.r && g == a_compare.g && b == a_compare.b && a == a_compare.a; }
	void operator += (const Colour & a_val) { r += a_val.r; g += a_val.g; b += a_val.b; a += a_val.a; }
	void operator -= (const Colour & a_val) { r -= a_val.r; g -= a_val.g; b -= a_val.b; a -= a_val.a; }

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
static const Colour sc_colourBrown			= Colour(0.625, 0.2f, 0.0f, 1.0f);

#endif //_CORE_COLOUR_
