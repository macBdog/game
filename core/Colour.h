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

	// Operator overloads
	/* TODO!
	Colour operator + (const Colour & a_val) const { return Colour(x + a_val.x, y + a_val.y, z + a_val.z); }
	Colour operator - (const Colour & a_val) const { return Colour(x - a_val.x, y - a_val.y, z - a_val.z); }
	Colour operator * (float a_scale) const { return Colour(x * a_scale, y * a_scale, z * a_scale); }
	Colour operator * (const Colour & a_val) const { return Colour(x * a_val.x, y * a_val.y, z * a_val.z); }
	bool operator == (const Colour & a_compare) const { return x == a_compare.x && y == a_compare.y && z == a_compare.z; }
	*/

private:
	float r, g, b, a;

};

// Statics for commonly referenced debug colours
static const Colour sc_colourWhite			= Colour(1.0f, 1.0f, 1.0f, 1.0f);
static const Colour sc_colourBlack          = Colour(0.0f, 0.0f, 0.0f, 1.0f);
static const Colour sc_colourRed            = Colour(1.0f, 0.0f, 0.0f, 1.0f);
static const Colour sc_colourGreen          = Colour(0.0f, 1.0f, 0.0f, 1.0f);
static const Colour sc_colourBlue           = Colour(0.0f, 0.0f, 1.0f, 1.0f);

#endif //_CORE_COLOUR_
