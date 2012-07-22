#ifndef _CORE_DELEGATE_
#define _CORE_DELEGATE_
#pragma once

#include "Callback.h"

//\brief Wrapper for callback class to provide access to the correct method through polymorphism
template <typename TReturnType, typename TParam>
class Delegate
{
public:
	
	// Default constructor so delegate can be initialised without arguments
	Delegate() : m_callback(NULL) {}

	// Setup the delegate to call the correct method on the object
	template <typename TObj, typename TMethod>
	void SetCallback(TObj * a_object, TMethod a_method)
	{
		// This can only happen once as the callback is not reference counted
		if (m_callback == NULL)
		{
			m_callback = new Callback<TReturnType, TParam, TObj, TMethod>(a_object, a_method);
		}
	}
	
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