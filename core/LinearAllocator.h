#ifndef _CORE_LINEAR_ALLOCATOR_
#define _CORE_LINEAR_ALLOCATOR_
#pragma once

#include <stdlib.h>

template <class T>
class LinearAllocator
{
public:
	
	//\brief Setup the size and offset tracking and allocate the memory
	LinearAllocator(int a_maxSizeBytes)
		: m_memory(NULL)
		, m_memorySize(a_maxSizeBytes)
		, m_currentOffset(0)
	{ 
		m_memory = malloc(a_maxSizeBytes);
		m_memorySize = a_maxSizeBytes;
		m_currentOffset = 0;
	}

	//\brief Default constructor provided to allow array initialisers
	LinearAllocator()
		: m_memory(NULL)
		, m_memorySize(0)
		, m_currentOffset(0)
	{ }

	//\brief Make sure memory is freed if the allocator is deleted
	~LinearAllocator() { Done(); }

	//\brief For setting up the allocator after declaration
	//\return true if the allocation was succesful and if the allocator had not been
	//		  previously initialised by another call to Init or the Constructor
	inline bool Init(int a_maxSizeBytes)
	{
		if (m_memorySize == 0 && m_currentOffset == 0)
		{
			m_memory = (T*)malloc(a_maxSizeBytes);
			m_memorySize = a_maxSizeBytes;
			m_currentOffset = 0;
			return m_memory != NULL;
		}
		else
		{
			return false;
		}
	}

	//\brief Proper usage of the allocator will call done to clean up
	inline void Done()
	{
		if (m_memory != NULL)
		{
			free(m_memory);
			m_memorySize = 0;
			m_currentOffset = 0;
		}
	}

	//\brief Allocate a block from the contiguous memory and advance the offset
	//\param a_allocationSizeBytes how much memory is being allocated
	//\return a pointer to the allocated memory
	inline T * Allocate(unsigned int a_allocationSizeBytes)
	{
		if (a_allocationSizeBytes > 0)
		{
			unsigned int newOffset = m_currentOffset + a_allocationSizeBytes;
			if (newOffset <= m_memorySize)
			{
				// Create pointer to new section of memory
				T * ptr =  m_memory + m_currentOffset;

				// Set new offset to account for allocated memory
				m_currentOffset = newOffset;

				return ptr;
			}
		}

		// Out of memory or allocator not large enough
		return NULL;
	}

	//\brief Informational functions to track how much memory is in use
	inline unsigned int GetAllocationSizeBytes() { return m_memorySize; }
	inline float GetAllocationRatio() { return m_memorySize > 0 ? m_currentOffset / m_memorySize : 0.0f; }
	inline T * GetHead() { return m_memory; }

private:

	T * m_memory;						///< Pointer to our chunk of memory
	unsigned int m_memorySize;			///< Total memory size in bytes
	unsigned int m_currentOffset;		///< How far through allocation we are in bytes

};

#endif // _CORE_LINEAR_ALLOCATOR_