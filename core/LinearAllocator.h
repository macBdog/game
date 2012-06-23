#ifndef _CORE_LINEAR_ALLOCATOR_
#define _CORE_LINEAR_ALLOCATOR_
#pragma once


class LinearAllocator
{
public:
	
	LinearAllocator(int a_maxSizeBytes)
		: m_memory(0)
		, m_memorySize(a_maxSizeBytes)
		, m_currentOffset(0)
	{
		// TODO setup m_memory here
	}

	inline void * Allocate(unsigned int a_allocationSizeBytes)
	{
		if (a_allocationSizeBytes > 0)
		{
			unsigned int newOffset = m_currentOffset + a_allocationSizeBytes;
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
		return 0;
	}

	inline unsigned int GetAllocationSizeBytes() { return m_memorySize; }
	inline float GetAllocationRatio() { return m_memorySize > 0 ? m_currentOffset / m_memorySize : 0.0f; }
private:

	unsigned int * m_memory;			///< 1 byte pointer to our chunk of memory
	unsigned int m_memorySize;			///< Total memory size in bytes
	unsigned int m_currentOffset;		///< How far through allocation we are in bytes*/

};

#endif // _CORE_LINEAR_ALLOCATOR_