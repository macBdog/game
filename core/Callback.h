#ifndef _CORE_CALLBACK_
#define _CORE_CALLBACK_
#pragma once

//\brief Pure virtual base class so any class can instance this and still have Execute called
class CallbackBase
{
public:
	virtual void Execute() const  = 0;				// No parameter version
	//virtual void Execute(int a_param) const = 0;	// Single int param version
};

//\brief Templated version of callback providing access to execute for all subclasses
template <class T>
class Callback : public CallbackBase
{
public:
	
	typedef void (T::*FunctionPointer)();			// Single parameter points to instance of class
	// typedef void (T::*FunctionPointer)(int a_param)	// Single param version

	Callback() : m_funcPointer(0) {}

	//\brief Call the function pointer on the instance of the callback
	virtual void Execute() const
	{
		if (m_funcPointer)
		{
			(m_thisPointer->*m_funcPointer)();
		}
	}

	//\brief Set the instance of the this pointer and the function to call
	void SetCallback(T * a_thisPointer, FunctionPointer a_funcPointer)
	{
		m_thisPointer = a_thisPointer;
		m_funcPointer = a_funcPointer;
	}

	T * m_thisPointer;				// Storage for the pointer to the class instance
	FunctionPointer m_funcPointer;	// Storage to the function to call on the class
};

#endif //_CORE_CALLBACK_