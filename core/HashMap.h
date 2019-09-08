#ifndef _CORE_HASH_MAP_
#define _CORE_HASH_MAP_
#pragma once

#include <unordered_map>

//\brief This is a wrapper to an STL unordered map which is not in keeping with the ideals
//		 of this project as being roll-your-own-everything but I need one NOW!
template <class Key, class T>
class HashMap
{
public:

	HashMap() : m_iteratorValid(false) {}

	//\brief Search for an element in the map
	//\param a_key the identifier for the item to look for
	//\param a_value_OUT is a ref to and itme to populate if found
	//\return true if the item was found and a_value_OUT will be modified
	bool Get(Key a_key, T & a_value_OUT)  
	{
		std::unordered_map<Key, T>::const_iterator got = m_map.find(a_key);
		if (got != m_map.end())
		{
			a_value_OUT = got->second;
			return true;
		}
		else
		{
			return false;
		}
	}

	//\brief Add an element to the map
	//\param a_key the unique way to identify the object
	//\param a_data the object to insert
	void Insert(Key a_key, T a_data) 
	{
		std::pair<Key, T> toInsert(a_key, a_data);
		m_map.insert(toInsert); 
	}

	//\brief Iterator like functionality
	bool GetNext(T & a_value_OUT)
	{
		// Early out for no objects
		if (m_map.size() == 0)
		{
			return false;
		}

		// Unititialised iterator
		if (!m_iteratorValid)
		{
			m_mapIt = m_map.begin();
			a_value_OUT = m_mapIt->second;
			m_iteratorValid = true;
			return true;
		}
		else // Elements after the first
		{
			++m_mapIt;
			if (m_mapIt != m_map.end())
			{
				a_value_OUT = m_mapIt->second;
				return true;
			}
			else // Reached the end
			{
				// Invalidate to be sure the next iteration fails
				m_iteratorValid = false;
				return false;
			}
		}
	}

private:

	typename std::unordered_map<Key, T> m_map;						// The underlying evil STL map
	typename std::unordered_map<Key, T>::const_iterator m_mapIt;		// Iterator to a map, prefix with typename because we are inside a template class
	bool m_iteratorValid;
};

#endif //_CORE_HASH_MAP