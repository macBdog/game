#ifndef _CORE_LINEAR_ALLOCATOR_
#define _CORE_LINEAR_ALLOCATOR_
#pragma once

#include <stdlib.h>

template <class T>
class LinearAllocator
{
public:
	
	//\brief Setup the size and offset tracking and allocate the memory
	LinearAllocator(size_t a_maxSizeBytes)
		: m_memory(NULL)
		, m_memoryEnd(NULL)
		, m_memoryPtr(NULL)
		, m_memorySize(0)
		, m_maxEnd(0)
	{ 
		m_memory = (T*)malloc(a_maxSizeBytes);
		m_memoryEnd = m_memory;
		m_memoryPtr = m_memory;
		m_memorySize = a_maxSizeBytes;
		m_maxEnd = a_maxSizeBytes + (size_t) m_memory;
	}

	//\brief Default constructor provided to allow array initialisers
	LinearAllocator()
		: m_memory(NULL)
		, m_memoryEnd(NULL)
		, m_memoryPtr(NULL)
		, m_memorySize(0)
		, m_maxEnd(0)
	{ }

	//\brief Make sure memory is freed if the allocator is deleted
	~LinearAllocator() { Done(); }

	//\brief For setting up the allocator after declaration
	//\return true if the allocation was succesful and if the allocator had not been
	//		  previously initialised by another call to Init or the Constructor
	inline bool Init(size_t a_maxSizeBytes)
	{
		if (m_memorySize == 0)
		{
			m_memory = (T*)malloc(a_maxSizeBytes);
			m_memoryEnd = m_memory;
			m_memoryPtr = m_memory;
			m_memorySize = a_maxSizeBytes;
			m_maxEnd = a_maxSizeBytes + (size_t) m_memory;
			
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
			m_memory = NULL;
			m_memoryEnd = NULL;
			m_memoryPtr = NULL;
			m_memorySize = 0;
			m_maxEnd = 0;
		}
	}

	//\brief Unallocate everything and ready for use again, memory is not freed
	inline void Reset()
	{
		if (m_memory != NULL)
		{
			m_memoryEnd = m_memory;
			m_memoryPtr = m_memory;
		}
	}

	//\brief Allocate a block from the contiguous memory and advance the offset
	//\param a_allocationSizeBytes how much memory is being allocated
	//\return a pointer to the allocated memory
	inline T * Allocate(size_t a_allocationSizeBytes, bool a_zeroMemory = true)
	{
		if (a_allocationSizeBytes > 0)
		{
			size_t newMemoryEnd = ((size_t) m_memoryEnd) + a_allocationSizeBytes;
			if (newMemoryEnd > m_maxEnd)
			{
				// Bad allocation
				return NULL;
			}
			else
			{
				// Set last pointer to new section of memory
				m_memoryPtr = m_memoryEnd;

				// Clear the memory that was just allocated
				if (a_zeroMemory)
				{
					memset(m_memoryPtr, 0, a_allocationSizeBytes);
				}

				// Update end sentinel
				m_memoryEnd = (T*)newMemoryEnd;

				return m_memoryPtr;
			}
		}

		// Out of memory or allocator not large enough
		return NULL;
	}

	//\brief DeAlloc a block from the contiguous memory, care should be taken to pair this with an alloc
	//\param a_allocationSizeBytes how much memory is being deallocated
	inline void DeAllocate(size_t a_allocationSizeBytes)
	{
		if (a_allocationSizeBytes > 0)
		{
			// Just rewind the pointer
			size_t newMemoryEnd = ((size_t) m_memoryEnd) - a_allocationSizeBytes;
			m_memoryEnd = (T*)newMemoryEnd;

			// Clear the overhang - this should be removed in debug configuration
			memset(m_memoryEnd, 0, a_allocationSizeBytes);
		}
	}

	//\brief Resize the last allocation keeping the head of the allocation untouched
	//\param a_newAllocationSizeBytes is the new, total size of the allocation
	//\return true if there was enough space to resize
	inline bool ResizeLastAllocation(size_t a_newAllocationSize)
	{
		if (a_newAllocationSize > 0)
		{
			size_t newMemoryEnd = ((size_t) m_memoryPtr) + a_newAllocationSize;
			if (newMemoryEnd > m_maxEnd)
			{
				// Bad allocation, not enough space for revised size
				return false;
			}
			else
			{
				// Update end sentinel
				m_memoryEnd = (T*)newMemoryEnd;

				return true;
			}
		}
		return false;
	}

	//\brief Informational functions to track how much memory is in use
	inline size_t GetAllocationSizeBytes() { return m_memorySize; }
	inline float GetAllocationRatio() { return m_memorySize > 0 ? m_currentOffset / m_memorySize : 0.0f; }
	inline T * GetHead() { return m_memory; }
	inline T * GetLastAllocation() { return m_memoryPtr; }

private:

	T * m_memory;						///< Pointer to our chunk of memory
	T * m_memoryEnd;					///< The end of the allocation
	T * m_memoryPtr;					///< Pointer to the head of the last allocation made
	size_t m_memorySize;				///< Total memory size in bytes
	size_t m_maxEnd;					///< The end marker of the stack

};

#endif // _CORE_LINEAR_ALLOCATOR_