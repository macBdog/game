#ifndef _CORE_PAGE_ALLOCATOR_
#define _CORE_PAGE_ALLOCATOR_
#pragma once

#include <assert.h>
#include <stdlib.h>

class Page
{
public:

	//\brief Setup the size of the page
	Page(size_t a_size)
		: m_size(0)
		, m_next(NULL)
		, m_prev(NULL)
		, m_memory(NULL)
	{
		m_memory = malloc(a_size);
		assert(m_memory != NULL);
	}
	
	~Page()
	{
		free(m_memory);
	}
	
	inline void AddNext(size_t a_size)
	{
		assert(m_next == NULL);
		m_next = (Page*)malloc(sizeof(Page));
		m_next(a_size);
		assert(m_next != NULL);
		m_next->m_prev = this;
	}
	
private:
	size_t m_size;
	Page * m_next;
	Page * m_prev;
	void * m_memory;
};

//\brief For use where contiguous resizable array like storage is required. 
//	 It's intended usage is to estimate the maximum number of elements
//	 upon construction then Add and Get in order to make use of cache/prefetch.
//	 When more than the initial estimated max items are added, a new allocation 
//	 of the same initial size is made. There is no defragmentation.
template <class T>
class PageAllocator
{
public:

	//\brief Setup the size of the initial and every subsequent allocation
	PageAllocator(int a_numElements)
		: m_itemSize(0)
		, m_numAllocations(0)
		, m_numItemsPerAllocation(0)
		, m_headAllocation(NULL)
	{
		m_itemSize = sizeof(T);
		m_headAllocation = malloc(sizeof(Page));
		m_headAllocation(m_itemSize * a_numElements);
		m_numAllocations = 1;
		m_numItemsPerAllocation = a_numElements;
	}

	inline T & Add(int a_index)
	{
		if (m_memory != NULL)
		{
			if (a_index < m_numItemsPerAllocation * m_numAllocations)
			{
				return *((T*)m_memory + (m_itemSize * a_index));
			}
			else // Need to allocate extra memory for the items
			{
				// TODO
				assert(false);
				return *((T*)m_memory);
			}
		}

		assert(false);
		return *((T*)m_memory);
	}

	inline T & Get(int a_index)
	{
		if (m_memory != NULL && a_index < m_numItemsPerAllocation * m_numAllocations)
		{
			return *(T*)(m_memory + (m_itemSize * a_index)));
		}

		assert(false);
		return *(T*)(m_memory);
	}

	inline T * Get(int a_index)
	{
		if (m_memory != NULL && a_index < m_numItemsPerAllocation * m_numAllocations)
		{
			return (T*)(m_memory + (m_itemSize * a_index));
		}

		assert(false);
		return NULL;
	}

	~PageAllocator()
	{
		free(m_memory);
	}

private:
	size_t m_itemSize;
	int m_numAllocations;
	int m_numItemsPerAllocation;
	Page * m_headAllocation;
	
};

#endif // _CORE_PAGE_ALLOCATOR_
