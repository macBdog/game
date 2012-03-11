#ifndef _CORE_LINEAR_ALLOCATOR_
#define _CORE_LINEAR_ALLOCATOR_
#pragma once


class cLinearAllocator
{
public:
	/*
	cLinearAllocator(int a_maxSize)
		: m_memory(NULL)
		, m_memorySize(a_maxSize)
		, m_currentOffset(0)
	{
		// TODO setup m_memory here
	}

	inline void * Allocate(uint32_t a_allocationSize)
	{
		if (a_allocationSize > 0)
		{
			uint32_t newOffset = m_currentOffset + a_allocationSize;
			if (newOffset <= m_memorySize)
			{
				// Create pointer to new section of memory
				void * ptr =  m_memory + m_currentOffset;

				// Set new offset to account for allocated memory
				m_currentOffset = newOffset;

				return ptr;
			}
		}

		// Out of memory or allocator not large enough
		return NULL;
	}
private:

	uint8_t * m_memory;			///< 1 byte pointer to our chunk of memory
	uint32_t m_memorySize;		///< Total memory size in bytes
	uint32_t m_currentOffset;		///< How far through allocation we are in bytes*/

};

#endif // _CORE_LINEAR_ALLOCATOR_