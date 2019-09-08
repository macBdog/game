#ifndef _CORE_CALLBACK_
#define _CORE_CALLBACK_
#pragma once

//\brief Abstract base class so any class can instance this and still have Execute called
template <typename TReturnType, typename TParam>
class CallbackBase
{
public:
	
	virtual TReturnType Execute(TParam a_param) = 0;
};

//\brief Templated version of callback providing access to execute for all subclasses and params
template <typename TReturnType, typename TParam, typename TObj, typename TMethod>
class Callback : public CallbackBase<TReturnType, TParam>
{
public:

	//\brief No default ctor, must Set the instance of the this pointer and the function to call
	Callback(void * a_object, TMethod a_method)
	{
		m_object = a_object;
		m_method = a_method;
	}

	//\brief Call the function pointer on the instance of the callback
	virtual TReturnType Execute(TParam a_param)
	{
		TObj * obj = static_cast<TObj *>(m_object);
		return (obj->*m_method)(a_param);
	}

private:

	void * m_object;		// The object that the method is run on 
	TMethod m_method;		// The method to call on the object
};

#endif //_CORE_CALLBACK_