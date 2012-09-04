#ifndef _MATH_UTILS_
#define _MATH_UTILS_
#pragma once

#include <random>
#include <time.h>

#define PI 3.141592654f 

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
	static float Deg2Rad(float a_degrees)
	{
		return a_degrees * (PI / 180.0f);
	}
}

#endif //_CORE_MATH_UTILS_