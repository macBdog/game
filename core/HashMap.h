#ifndef _CORE_HASH_MAP_
#define _CORE_HASH_MAP_
#pragma once

//\brief Unordered hash node class to be used with HashMap
template <class K, class T>
class HashNode
{
public:
	HashNode(const K & a_key, T * a_object)
		: m_object(a_object)
		, m_next(nullptr)
	{
		m_key = GetHash();
	}

	bool IsValid() { return m_object != nullptr; }

private:

	unsigned int m_key;
	T * m_object;
	HashNode * m_next;
};

//\brief Unordered map class with simple hash functions for string types resolving to uints.
//		 Utilises separate chaining with list head cells for collision management.
template <class K, class T>
class HashMap
{
public:

	explicit HashMap() 
		: m_tableSize(s_defaultTableSize)
		, m_length(0)
	{
		Allocate();
	}

	~HashMap()
	{
		Deallocate();
	}

	void ResizeAndClearAll(size_t a_newSize)
	{
		Deallocate();
		m_tableSize = a_newSize;
		m_length = 0;
		Allocate()
	}

	bool Contains(K a_key)
	{
		HashNode t<K, T>(a_key, nullptr);
		HashNode c<K, T> = m_map[t.m_key];
		return c.IsValid();
	}

	//\brief Retrieve an element in the map
	//\param a_key the identifier for the item to look for
	//\param a_value_OUT is a ref to and itme to populate if found
	//\return true if the item was found and a_value_OUT will be modified
	bool Get(K a_key, T & a_value_OUT)  
	{
		return false;
	}

	//\brief Add an element to the map
	//\param a_key the unique way to identify the object
	//\param a_data the object to insert
	void Insert(K a_key, T a_data) 
	{
		HashNode<K, T> toInsert(a_key, a_data);
		unsigned int rawKey = toInsert.m_key;
		m_map[rawKey].Insert(toInsert); 
	}

	//\brief Iterator like functionality
	bool GetNext(T & a_value_OUT)
	{
		// Early out for no objects
		if (m_length == 0)
		{
			return false;
		}

		return false;	
	}

private:

	bool Allocate()
	{
		// Zero initialise the complete table so that IsValid calls will always fail to empty buckets
		HashNode<K, T> tempNode;
		m_map = (HashNode<K, T>*)malloc(m_tableSize); // TODO correct size of node/table
		memset(m_map, 0, m_tableSize); // TODO correct size of node/table
		return m_map != nullptr;
	}

	bool Deallocate()
	{
		free(m_map);
		m_map = nullptr;
	}

	// Default hash function assuming unique keys supplied, collision free is impossible esp over small data
	unsigned int GetHash(const K a_key) { return m_key % m_tableSize; }

	static const size_t s_defaultTableSize = 512;
	static const int s_maxStringHash32bit = 9; // TODO Move the StringHash guts into the HashMap to replace this hacky hash stuff

	int m_length;									///< How many objects are inserted into the map
	int m_tableSize;								///< Number of bytes allocated to hold the complete table
	HashNode<K, T> * m_map;							///< Chunk of memory pointing to the objects in the map ordered by key
};

// String function assumes uniqueness in 9 letters sampled from string over length
template<>
unsigned int HashMap<const char *, class T>::GetHash(const char * a_key)
{
	const int strSize = strlen(a_key);
	const unsigned int numChars = strSize < HashMap::s_maxStringHash32bit ? strSize : s_maxStringHash32bit;
	unsigned int key = 0;
	int strPos = 0;
	for (unsigned int i = 0; i < numChars; ++i)
	{
		key += (int)a_key[strPos] * (int)pow(10, i);
		++strPos;
	}
	return key % m_tableSize;
}

#endif //_CORE_HASH_MAP