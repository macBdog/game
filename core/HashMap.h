#ifndef _CORE_HASH_MAP_
#define _CORE_HASH_MAP_
#pragma once

#include "Iterator.h"

//\brief Unordered hash node class to be used with HashMap
template <typename K, typename T>
class HashNode
{
public:
	HashNode()
		: m_next(nullptr) {}

	HashNode(const K & a_key, T & a_object)
		: m_key(a_key)
		, m_object(a_object)
		, m_next(nullptr) {}

private:

	template <typename K, typename T> friend class HashMap;

	K m_key;					// Index into the bucket
	T m_object;					// Storage for object type
	HashNode * m_next;			// In the case of collisions, each node is a singly linked list forward
};

// Default hash function assuming unique keys supplied, collision free is impossible esp over small data
template <typename K>
struct KeyHash 
{
	unsigned int GetHash(K a_key, unsigned int a_tableSize) const { return a_key % a_tableSize; }
};

//\brief Unordered map class with simple hash functions for string types resolving to uints.
//		 Utilises separate chaining with list head cells for collision management.
template <typename K, typename T>
class HashMap
{
public:

	HashMap(const HashMap & a_moveFrom)
	{

	}

	HashMap(HashMap && a_moveFrom)
		: m_map(a_moveFrom.m_map)
		, m_tableSize(a_moveFrom.m_tableSize)
		, m_length(a_moveFrom.m_length)
	{
		a_moveFrom.m_map = nullptr;
	}

	HashMap() 
		: m_map(nullptr)
		, m_tableSize(s_defaultTableSize)
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
		unsigned int rawKey = KeyHash<K>().GetHash(a_key, m_tableSize);
		HashNode<K, T> * found = m_map[rawKey];
		return found != nullptr;
	}

	//\brief Retrieve an element in the map
	//\param a_key the identifier for the item to look for
	//\param a_value_OUT is a ref to and itme to populate if found
	//\return true if the item was found and a_value_OUT will be modified
	bool Get(K a_key, T & a_value_OUT)
	{
		unsigned int rawKey = KeyHash<K>().GetHash(a_key, m_tableSize);
		HashNode<K, T> * found = m_map[rawKey];
		if (found != nullptr)
		{
			a_value_OUT = found->m_object;
			auto * next = found->m_next;
			while (next != nullptr)
			{
				a_value_OUT = next->m_object;
				next = next->m_next;
			}

			return true;
		}
		return false;
	}

	//\brief Add an element to the map
	//\param a_key the unique way to identify the object
	//\param a_data the object to insert
	void Insert(K a_key, T & a_data) 
	{
		HashNode<K, T> * newNode = new HashNode<K, T>(a_key, a_data);
		unsigned int rawKey = KeyHash<K>().GetHash(a_key, m_tableSize);
		HashNode<K, T> * tNode = m_map[rawKey];
		if (tNode != nullptr)
		{
			auto * next = tNode->m_next;
			while (next != nullptr)
			{
				next = next->m_next;
			}
			next->m_next = newNode;
		}
		else
		{
			m_map[rawKey] = newNode;
		}
	}

	//\brief Iterator like functionality for collections that need value matching and single element walks
	Iterator<HashNode<K, T>> GetIterator() { return Iterator<HashNode<K, T>>(m_map[0]); }	
	bool GetNext(Iterator<HashNode<K, T>> & a_it, T & a_value_OUT)
	{
		// Early out for no objects
		if (m_length == 0)
		{
			return false;
		}
		while (a_it.Resolve() != nullptr)
		{
			a_it.Inc();
			a_value_OUT = (*a_it).m_object;
		}
		return false;	
	}

private:

	bool Allocate()
	{
		// Zero initialise the complete table so that IsValid calls will always fail to empty buckets
		m_map = new HashNode<K, T> * [m_tableSize]();
		return m_map != nullptr;
	}

	bool Deallocate()
	{
		if (m_map != nullptr)
		{
			delete[] m_map;
			m_map = nullptr;
			return true;
		}
		return false;
	}

	static const size_t s_defaultTableSize = 512;	

	int m_length;									///< How many objects are inserted into the map
	int m_tableSize;								///< Number of bytes allocated to hold the complete table
	HashNode<K, T> ** m_map;						///< Chunk of memory pointing to the objects in the map ordered by key
	KeyHash<K> m_hashFunc;									///< Pointer to struct for hashing
};

static const int s_maxStringHash32bit = 9;		// TODO Move the StringHash guts into the HashMap to replace this hacky hash stuff

#endif //_CORE_HASH_MAP