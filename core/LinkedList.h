#ifndef _CORE_LINKED_LIST_
#define _CORE_LINKED_LIST_
#pragma once

#include <functional>

template <class T>
class LinkedListNode
{
public:

	// Default constructor - nodes are valid without data
	LinkedListNode()
	: m_data(0)
	, m_next(0)
	, m_prev(0)
	{}

	// Constructor where data is passed
	LinkedListNode(T * a_data)
	: m_data(a_data)
	, m_next(0)
	, m_prev(0)
	{}

	// Data accessor
	inline T * GetData() { return m_data; }
	inline void SetData(T * a_data) { m_data = a_data; }

	// Neighbour accessors
	inline LinkedListNode * GetNext() { return m_next; }
	inline LinkedListNode * GetPrev() { return m_prev; }
	inline bool IsLinked() { return m_next != 0 || m_prev != 0; }

private:

	// Data item 
	T * m_data;

	// Links to neighbour data
	LinkedListNode * m_next;
	LinkedListNode * m_prev;

	// Allows list and iterators to access neighbours without function calls
	template <class T> friend class LinkedList;
	template <class T> friend class cIterator;
};

template <class T>
class LinkedList
{
public:

	LinkedList()
	: m_head(0)
	, m_tail(0)
	, m_cursor(0)
	, m_length(0)
	{}

	//\brief Accessors for head and tail
	inline LinkedListNode<T> * GetHead() const { return m_head; }
	inline LinkedListNode<T> * GetTail() const { return m_tail; }
	inline bool IsEmpty() const { return m_length == 0; }
	inline unsigned int GetLength() const { return m_length; }

	//\brief Convenience method to new a node and insert it
	inline void InsertNew(T * a_data)
	{
		LinkedListNode<T> * newNode = new LinkedListNode<T>();
		newNode->SetData(a_data);
		Insert(newNode);
	}

	//\brief Add a node to the list
	inline void Insert(LinkedListNode<T> * a_node)
	{
		// First item in the list
		if (m_head == 0)
		{
			m_head = a_node;
			a_node->m_prev = 0;
			a_node->m_next = 0;
		}
		else // Add to list at current position and link forward and back
		{
			m_cursor->m_next = a_node;
			a_node->m_prev = m_cursor;
			a_node->m_next = 0;
		}

		// Set cursor to our current entry
		m_tail = a_node;
		m_cursor = a_node;

		m_length++;
	}

	//\brief Convenience method to delete the node
	inline void RemoveDelete(LinkedListNode<T> * a_node)
	{
		Remove(a_node);
		delete a_node;
	}

	//\brief Link next to prev and vice versa
	inline void Remove(LinkedListNode<T> * a_node) 
	{
		// Update new head of list
		if (a_node == m_head)
		{
			m_head = a_node->m_next;
		}

		// Update new tail of list
		if (a_node == m_tail)
		{
			m_tail = a_node->m_prev;
		}

		// Update operating cursor
		if (m_cursor == a_node)
		{
			m_cursor = a_node->m_prev != nullptr ? a_node->m_prev : a_node->m_next;
		}

		// Remove node by linking neighbours to each other
		if (a_node->m_next)
		{
			a_node->m_next->m_prev = a_node->m_prev;
		}
		if (a_node->m_prev)
		{
			a_node->m_prev->m_next = a_node->m_next;
		}

		m_length--;

		// Handle last node removal
		if (m_length == 0)
		{
			m_head = 0;
			m_tail = 0;
		}
	}

	//\brief Convenience function for iterative common case
	inline void Foreach(std::function<void(LinkedListNode<T> *)> a_callback)
	{
		auto next = GetHead();
		while (next != nullptr)
		{
			auto cur = next;
			next = cur->GetNext();

			a_callback(cur);
		}
	}

	//\brief Convenience function for iteractive common case
	inline void ForeachAndDelete(std::function<void(LinkedListNode<T> *)> a_callback)
	{
		auto next = GetHead();
		while (next != nullptr)
		{
			auto cur = next;
			next = cur->GetNext();
			
			a_callback(cur);
			delete cur->GetData();
			delete cur;
		}
	}
	
	//\brief Iterative search
	inline LinkedListNode<T> * Find(T * a_data)
	{
		LinkedListNode<T> * foundNode = nullptr;
		LinkedListNode<T> * curNode = m_head;
		while (curNode != nullptr)
		{
			if (curNode->GetData() == a_data)
			{
				foundNode = curNode;
				break;
			}
			curNode = curNode->m_next;
		}
		return foundNode;
	}

	inline void Swap(LinkedListNode<T> * a_node1, LinkedListNode<T> * a_node2)
	{
		// Copy nodes to be swapped so we dont write over neighbour addresses
		// that are used later in the swap.
		LinkedListNode<T> a_temp1;
		a_temp1 = *a_node1;

		LinkedListNode<T> a_temp2;
		a_temp2 = *a_node2;

		// Swap the neighbours of each node around preserving head and tail
		if (a_node1 != m_head && a_node1->m_prev)
		{
			a_node1->m_prev->m_next = a_node2;
		}
		else
		{
			m_head = a_node2;
		}
		a_node1->m_prev = a_node2;
		a_node1->m_next = a_node2->m_next;

		if (a_node2 != m_tail && a_node2->m_next)
		{
			a_node2->m_next->m_prev = a_node1;
		}
		a_node2->m_prev = a_temp1.m_prev;
		a_node2->m_next = a_node1;
	}

	inline void Merge(LinkedList<T> * a_result, LinkedList<T> & a_list1, LinkedList<T> & a_list2)
	{
		while (!a_list1.IsEmpty() && !a_list2.IsEmpty())
		{
			// Merge two lists in this list at the cursor preserving order
			LinkedListNode<T> * cursor1 = a_list1.m_head;
			LinkedListNode<T> * cursor2 = a_list2.m_head;

			T & data1 = *cursor1->GetData();
			T & data2 = *cursor2->GetData();
			if (data1 < data2)
			{
				a_list1.Remove(cursor1);
				a_result->Insert(cursor1);
			}
			else
			{
				a_list2.Remove(cursor2);
				a_result->Insert(cursor2);
			}
		}

		// Clean up overhang
		while (!a_list1.IsEmpty())
		{
			LinkedListNode<T> * cursor = a_list1.m_head;
			a_list1.Remove(cursor);
			a_result->Insert(cursor);
		}
		while (!a_list2.IsEmpty())
		{
			LinkedListNode<T> * cursor = a_list2.m_head;
			a_list2.Remove(cursor);
			a_result->Insert(cursor);
		}
	}

	// An O(n^2) sorting method - bubble sort
	inline void SlowSort();

	// An O(n log n) sorting method
	inline void MergeSort();

private:

	// Recursive declaration of MergeSort
	inline void MergeSort(LinkedList<T> * a_list);

	// Pointers to head, tail and current operating position in the ist
	LinkedListNode<T> * m_head;
	LinkedListNode<T> * m_tail;
	LinkedListNode<T> * m_cursor;

	// Helper for list length
	unsigned int m_length;

	// Allows linked list nodes to use static functions
	friend class LinkedListNode<T>;
};


template <class T> void LinkedList<T>::SlowSort() 
{
	// A list of size 0 or 1 is already sorted
	if (m_length <= 1)
	{
		return;
	}

	// Start swapping nodes at the head of the list
	LinkedListNode<T> * start = m_head;
	bool swapPerformed = true;
	while (swapPerformed)
	{
		swapPerformed = false;
		start = m_head;
		while (start != 0 && start->m_next)
		{
			// TODO - fix this! the > operator of cVector isn't working
			T & data1 = *start->GetData();
			T & data2 = *start->m_next->GetData();
			if (data1 > data2)
			{
				Swap(start, start->m_next);
				swapPerformed = true;
			}
			start = start->m_next;
		}
	}
}


template <class T> void LinkedList<T>::MergeSort() 
{
	MergeSort(this);
}

template <class T> void LinkedList<T>::MergeSort(LinkedList<T> * a_list)
{
	// A list of size 0 or 1 is already sorted
	if (a_list->m_length <= 1)
	{
		// Do nothing, list is sorted
		return;
	}

	int subListSize = a_list->m_length / 2;
	LinkedList<T> left, right;
	
	LinkedListNode<T> * cursor = a_list->m_head;
	int listLength =  a_list->m_length;
	for (int i = 0; i < listLength; ++i)
	{
		// Cache off the next neighbour as it's cleared
		// when inserting into a sublist
		LinkedListNode<T> cacheCurrent = *cursor;

		// Fill each sub list
		a_list->Remove(cursor);

		if (i < subListSize)
		{
			left.Insert(cursor);
		}
		else
		{
			right.Insert(cursor);
		}
		
		cursor = cacheCurrent.m_next;
	}

	// Recusively sort each list then merge the two together
	MergeSort(&left);
	MergeSort(&right);
	Merge(a_list, left, right);
}

#endif //_CORE_LINKED_LIST_