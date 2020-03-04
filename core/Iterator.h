#pragma once

//\brief A bit useless
template <class T>
class Iterator
{
public:

	Iterator(T * a_object)
		: m_count(0)
	{ 
		m_object = a_object;
	}

	inline void operator++() { Inc(); } 
	inline void operator++(int) { Inc(); } 
	inline void operator--() { Dec(); } 
	inline void operator--(int) { Dec(); } 
	inline T & operator*() { return *Resolve(); } 
	inline const T & operator*() const { return *Resolve(); } 
	inline T * operator->() { return Resolve(); } 
	inline const T * operator->() const { return Resolve(); } 
	inline T * GetObject() const { return m_object; }

	inline void SetObject(T * a_object) { m_object = a_object; }
	inline void Inc() { ++m_count; }
	inline void Dec() { --m_count; }
	inline T * Resolve() { return m_object; }
	inline unsigned int GetCount() const { return m_count; }

private:
	T * m_object;
	unsigned int m_count;
};
