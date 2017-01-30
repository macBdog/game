#pragma once

#include "MathUtils.h"

template <class T>
class Range
{
public:

	Range()
		: m_hi()
		, m_low(0)
	{}
	Range(const T & a_hi, const T & a_low)
		: m_hi(a_hi)
		, m_low(a_low)
	{}
	Range(const T & a_val)
		: m_hi(a_val)
		, m_low(a_val)
	{}

	inline T GetDiff() const { return m_hi - m_low; }
	inline bool IsValid() const { return m_hi > m_low; }

	T GetRandom()
	{
		return m_low + GetDiff() * MathUtils::RandFloat();
	}

	T m_hi;
	T m_low;
};
