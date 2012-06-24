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

private:

	std::unordered_map<Key, T> m_map;
};

#endif //_CORE_HASH_MAP