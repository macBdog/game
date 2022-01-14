#pragma once

template <class T>
class Iterator
{
public:
	Iterator()
		: m_cursor(nullptr)
		, m_countMajor(-1)
		, m_countMinor(-1)
		, m_valid(false) 
	{}

	Iterator(T * a_start) 
		: m_cursor(a_start)
		, m_countMajor(0)
		, m_countMinor(0)
		, m_valid(true)
	{}

	inline void operator++() { Inc(); } 
	inline void operator++(int) { Inc(); } 
	inline void operator--() { Dec(); } 
	inline void operator--(int) { Dec(); } 
	inline T & operator*() { return *Resolve(); } 
	inline const T & operator*() const { return *Resolve(); } 
	inline T * operator->() { return Resolve(); } 
	inline const T * operator->() const { return Resolve(); } 
	inline T * GetObject() const { return m_cursor; }

	inline void Inc() { ++m_cursor; ++m_countMajor; }
	inline void Dec() { --m_cursor; --m_countMajor; }
	inline T * Resolve() { return m_valid ? m_cursor : nullptr; }
	inline void Invalidate() { m_valid = false; }
	inline unsigned int GetCount() const { return m_countMajor; }
	inline void SetObject(T* a_object) { m_cursor = a_object; }

private:
	T * m_cursor;
	unsigned int m_countMajor;
	unsigned int m_countMinor;
	bool m_valid;
};
