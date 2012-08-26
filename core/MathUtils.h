#ifndef _MATH_UTILS_
#define _MATH_UTILS_
#pragma once

//\brief Math helpers
namespace MathUtils
{
	//\brief Get the larger of two floats
	template <class T>
	static T GetMax(T a_val1, T a_val2)
	{
		if (a_val1 >= a_val2)
		{
			return a_val1;
		}
		else
		{	
			return a_val2;
		}
	}

	//\brief Get the smaller of two floats
	template <class T>
	static T GetMin(T a_val1, T a_val2)
	{
		if (a_val1 <= a_val2)
		{
			return a_val1;
		}
		else
		{	
			return a_val2;
		}
	}
}

#endif //_CORE_MATH_UTILS_