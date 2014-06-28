#ifndef _CORE_PAGE_ALLOCATOR_
#define _CORE_PAGE_ALLOCATOR_
#pragma once

#include <assert.h>
#include <stdlib.h>

//\brief An allocation meant to store multiple copies of what the allocator stores
template <class T>
class Page
{
public:

	//\brief Setup the size of the page
	Page()
		: m_size(0)
		, m_itemSize(0)
		, m_maxItems(0)
		, m_memory(NULL)
	{ }

	inline bool Init(size_t a_itemSize, unsigned int a_maxItems)
	{
		m_size = a_itemSize * a_maxItems;
		m_itemSize = a_itemSize;
		m_maxItems = a_maxItems;
		m_memory = (T*)(malloc(m_size));
#ifdef _DEBUG
		memset(m_memory, 0, m_size);
#endif
		return m_memory != NULL;
	}

	inline T * Add(unsigned int a_index)
	{
		assert(m_memory != NULL);
		if (a_index >= 0 && a_index < m_maxItems)
		{
			// Offset into block of memory is garbage, call placement new
			T * newObject = new (m_memory + a_index) T();
			return newObject;
		}
		return NULL;
	}

	inline T * Get(unsigned int a_index)
	{
		assert(m_memory != NULL);
		if (a_index >= 0 && a_index < m_maxItems)
		{
			return m_memory + a_index;
		}
		return NULL;
	}
	
	~Page()
	{
		free(m_memory);
	}
	
private:
	size_t m_size;
	size_t m_itemSize;
	unsigned int m_maxItems;
	T * m_memory;
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
	PageAllocator()
		: m_itemSize(0)
		, m_numPages(0)
		, m_itemsPerPage(0)
	{ }

	inline bool Init(unsigned int a_numItems, size_t a_itemSize)
	{
		m_itemSize = a_itemSize;
		m_itemsPerPage = a_numItems;
		return m_pages[m_numPages++].Init(m_itemSize, m_itemsPerPage);
	}

	inline T * Add(unsigned int a_index)
	{
		// Calculate position in page from index
		int pagePos = a_index - ((m_numPages-1) * m_itemsPerPage);

		// Try to add to current page, otherwise setup a new page and add to that
		void * newAlloc = m_pages[m_numPages-1].Add(pagePos);
		if (newAlloc == NULL)
		{
			m_pages[m_numPages++].Init(m_itemSize, m_itemsPerPage);
			newAlloc = m_pages[m_numPages-1].Add(pagePos);
		}
		assert(newAlloc != NULL);
		return (T*)newAlloc;
	}

	inline bool Reset()
	{
		m_numPages = 1;
		return true;
	}

	inline T * Get(unsigned int a_index)
	{
		// Calculate position in page from index
		int pagePos = a_index - ((m_numPages-1) * m_itemsPerPage);
		return (T*)m_pages[m_numPages-1].Get(pagePos);
	}

private:

	static const int s_maxPages = 32;	///< Pages are 20 byte classes so assume a max number of pages

	Page<T> m_pages[s_maxPages];		///< Contiguous array of pointers to allocations
	size_t m_itemSize;					///< How big each item is inside each page
	unsigned int m_itemsPerPage;		///< The number of items in each page
	unsigned int m_numPages;			///< How many pages are currently being utilised
};

#endif // _CORE_PAGE_ALLOCATOR_