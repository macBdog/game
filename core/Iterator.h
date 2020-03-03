#ifndef _CORE_ITERATOR_
#define _CORE_ITERATOR_
#pragma once

//\brief A bit useless
template <class T>
class Iterator
{
public:

	Iterator(T * a_start) { m_cursor = a_start; }

	inline void operator++() { Inc(); } 
	inline void operator++(int) { Inc(); } 
	inline void operator--() { Dec(); } 
	inline void operator--(int) { Dec(); } 
	inline T & operator*() { return *Resolve(); } 
	inline const T & operator*() const { return *Resolve(); } 
	inline T * operator->() { return Resolve(); } 
	inline const T * operator->() const { return Resolve(); } 

	inline void Inc() { ++m_cursor; }
	inline void Dec() { --m_cursor; }
	inline T * Resolve() { return m_cursor; }

private:
	T * m_cursor;
};

#endif //_CORE_ITERATOR_