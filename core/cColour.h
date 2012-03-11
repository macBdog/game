#ifndef _CORE_COLOUR_
#define _CORE_COLOUR_
#pragma once

//\brief Basic 4 value colour class
class cColour
{
public:

	// Constructors
	inline cColour() { r = 0.0f; g = 0.0f; b = 0.0f; a = 0.0f; }
	inline cColour(float a_val) { r = a_val; g = a_val; b = a_val; a = a_val; }
	inline cColour(float a_r, float a_g, float a_b, float a_a) { r = a_r; g = a_g; b = a_b; a = a_a; }

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
	cColour operator + (const cColour & a_val) const { return cColour(x + a_val.x, y + a_val.y, z + a_val.z); }
	cColour operator - (const cColour & a_val) const { return cColour(x - a_val.x, y - a_val.y, z - a_val.z); }
	cColour operator * (float a_scale) const { return cColour(x * a_scale, y * a_scale, z * a_scale); }
	cColour operator * (const cColour & a_val) const { return cColour(x * a_val.x, y * a_val.y, z * a_val.z); }
	bool operator == (const cColour & a_compare) const { return x == a_compare.x && y == a_compare.y && z == a_compare.z; }
	*/

private:
	float r, g, b, a;

};

// Statics for commonly referenced debug colours
static const cColour sc_colourBlack          = cColour(0.0f, 0.0f, 0.0f, 1.0f);
static const cColour sc_colourRed            = cColour(1.0f, 0.0f, 0.0f, 1.0f);
static const cColour sc_colourGreen          = cColour(0.0f, 1.0f, 0.0f, 1.0f);
static const cColour sc_colourBlue           = cColour(0.0f, 0.0f, 1.0f, 1.0f);

#endif //_CORE_COLOUR_
