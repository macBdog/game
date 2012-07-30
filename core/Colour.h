#ifndef _CORE_COLOUR_
#define _CORE_COLOUR_
#pragma once

//\brief Basic 4 value colour class
class Colour
{
public:

	// Constructors
	inline Colour() { r = 0.0f; g = 0.0f; b = 0.0f; a = 0.0f; }
	inline Colour(float a_val) { r = a_val; g = a_val; b = a_val; a = a_val; }
	inline Colour(float a_r, float a_g, float a_b, float a_a) { r = a_r; g = a_g; b = a_b; a = a_a; }

	// Mutators
	inline void SetR(float a_val) { r = a_val; }
	inline void SetG(float a_val) { g = a_val; }
	inline void SetB(float a_val) { b = a_val; }
	inline void SetA(float a_val) { a = a_val; }
	inline float GetR() { return r; }
	inline float GetG() { return g; }
	inline float GetB() { return b; }
	inline float GetA() { return a; }

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
static const Colour sc_colourGreyAlpha		= Colour(0.25f, 0.25f, 0.25f, 0.5f);
static const Colour sc_colourRed            = Colour(1.0f, 0.0f, 0.0f, 1.0f);
static const Colour sc_colourGreen          = Colour(0.0f, 1.0f, 0.0f, 1.0f);
static const Colour sc_colourBlue           = Colour(0.0f, 0.0f, 1.0f, 1.0f);
static const Colour sc_colourPurple         = Colour(1.0f, 0.0f, 1.0f, 1.0f);
static const Colour sc_colourYellow			= Colour(1.0f, 1.0f, 0.0f, 1.0f);
static const Colour sc_colourOrange			= Colour(1.0f, 0.33f, 0.0f, 1.0f);

#endif //_CORE_COLOUR_
