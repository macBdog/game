#ifndef _ENGINE_SINGLETON_H__
#define _ENGINE_SINGLETON_H__
#pragma once

template<class T>
class Singleton
{
public:
	static bool CreateSingleton()		
	{ 
		Get();
		if (!s_instance)
		{
			return false;
		}
		return true;
	}

	// Call this last because the instance is destructed in this call
	static bool DestroySingleton()	
	{ 
		FreeInstance(); 
		return true;
	}

	static T& Get()
	{
		if (s_instance == NULL)
		{
			s_instance = new(T); 
		}

		return *s_instance;
	}

protected:
	Singleton() {}
	virtual ~Singleton() {}
	
	static void FreeInstance()
	{
		if (s_instance)
		{
			delete s_instance;
			s_instance = NULL;
		}
	}

	static T* s_instance;
};

#endif // _ENGINE__SINGLETON_H__