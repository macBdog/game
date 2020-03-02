#ifndef _CORE_DELEGATE_
#define _CORE_DELEGATE_
#pragma once

#include <cstdlib>

#include "Callback.h"

//\brief Wrapper for callback class to provide access to the correct method through polymorphism
template <typename TReturnType, typename TParam>
class Delegate
{
public:
	
	// Default constructor so delegate can be initialised without arguments
	Delegate() : m_callback(nullptr) {}

	// Setup the delegate to call the correct method on the object
	template <typename TObj, typename TMethod>
	inline void SetCallback(TObj * a_object, TMethod a_method)
	{
		// This can only happen once as the callback is not reference counted
		if (m_callback == nullptr)
		{
			m_callback = new Callback<TReturnType, TParam, TObj, TMethod>(a_object, a_method);
		}
	}

	// Clear the callback
	inline void ClearCallback()
	{
		m_callback = nullptr;
	}

	//\brief Test if the delegate has been set up
	inline bool IsSet() { return m_callback != nullptr; }
	
	// Cleanup allocation
	~Delegate()
	{
		delete m_callback;
	}

	//\brief Call through to the stored callback
	TReturnType Execute(TParam a_param)
	{
		return m_callback->Execute(a_param);
	}

private:

	CallbackBase<TReturnType, TParam> * m_callback;	// Storage for the callback
};

#endif //_CORE_DELEGATE_