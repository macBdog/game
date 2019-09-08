#pragma once

#include "Colour.h"
#include "MathUtils.h"
#include "Vector.h"

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

template <>
Vector Range<Vector>::GetRandom()
{
	return Vector(	m_low.GetX() + (m_hi.GetX() - m_low.GetX()) * MathUtils::RandFloat(),
					m_low.GetY() + (m_hi.GetY() - m_low.GetY()) * MathUtils::RandFloat(),
					m_low.GetZ() + (m_hi.GetZ() - m_low.GetZ()) * MathUtils::RandFloat());
}

template <>
Colour Range<Colour>::GetRandom()
{
	return Colour(	m_low.GetR() + (m_hi.GetR() - m_low.GetR()) * MathUtils::RandFloat(),
					m_low.GetG() + (m_hi.GetG() - m_low.GetG()) * MathUtils::RandFloat(),
					m_low.GetB() + (m_hi.GetB() - m_low.GetB()) * MathUtils::RandFloat(),
					m_low.GetA() + (m_hi.GetA() - m_low.GetA()) * MathUtils::RandFloat());
}

template <>
bool Range<Colour>::IsValid() const
{
	return m_low.GetR() + m_low.GetG() + m_low.GetB() + m_low.GetA() > 0.0f && m_hi.GetR() + m_hi.GetG() + m_hi.GetB() + m_hi.GetA() > 0.0f;
}