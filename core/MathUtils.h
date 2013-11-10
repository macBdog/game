#ifndef _MATH_UTILS_
#define _MATH_UTILS_
#pragma once

#include <random>
#include <time.h>

#define PI 3.141592654f
#define TAU 6.283185308f
#define EPSILON 0.0001f

//\brief Math helpers
namespace MathUtils
{
	//\brief Get the larger of two primitive types
	template <class T>
	static T GetMax(T a_val1, T a_val2)
	{
		return a_val1 >= a_val2 ? a_val1 : a_val2;
	}

	//\brief Get the smaller of two primitive types
	template <class T>
	static T GetMin(T a_val1, T a_val2)
	{
		return a_val1 <= a_val2 ? a_val1 : a_val2;
	}

	//\brief Must be called once a program execution
	static void InitialiseRandomNumberGenerator()
	{
		srand((unsigned)time(0));
	}

	//\brief Return a random float between 0.0 and 1.0
	static float RandFloat()
	{
		return (float)rand()/(float)RAND_MAX;
	}

	//\brief Return a random float between between two ranges
	static float RandFloatRange(float a_lowerBound, float a_upperBound)
	{
		return a_lowerBound + ((a_upperBound - a_lowerBound) * RandFloat());
	}

	//\brief Return a random int between 0 and RAND_MAX
	static int RandInt()
	{
		return rand();
	}

	//\brief Return a random into between two ranges
	static int RandIntRange(int a_lowerBound, int a_upperBound)
	{
		return a_lowerBound + (rand()%(a_upperBound+1 - a_lowerBound));
	}

	//\brief Return a float a fraction between two bounds
	static float LerpFloat(float a_from, float a_to, float a_frac)
	{
		if (a_frac > 1.0f)
		{
			a_frac = 1.0f / a_frac;
		}

		if (a_from > a_to)
		{
			return a_from - ((a_from - a_to) * a_frac);
		}
		else	
		{
			return a_from + ((a_to - a_from) * a_frac);
		}
	}
	
	//\brief Convert between degrees and radians
	static float Deg2Rad(const float & a_degrees)
	{
		return a_degrees * (PI / 180.0f);
	}

	//\brief Compare a float value  to close to zero
	static bool IsZeroEpsilon(const float & a_val)
	{
		return fabsf(a_val) < EPSILON;
	}

	//\brief Reduce a value to a set minimum or maximum
	static float Clamp(const float & a_min, const float & a_val, const float & a_max)
	{
		return (a_val < a_min) ? a_min : (a_val > a_max) ? a_max : a_val;
	}
}

#endif //_CORE_MATH_UTILS_