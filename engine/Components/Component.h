#ifndef _ENGINE_COMPONENT_
#define _ENGINE_COMPONENT_
#pragma once

class GameObject;

/// \brief Component is designed to be pure virtual base class for GameObject component functionality.
class Component
{
public:
	
	enum eComponentType
	{
		eComponentTypeInvalid = -1,
		eComponentTypeRootMotion,

		eComponentTypeCount,
	};

	virtual eComponentType GetComponentType() = 0;
	
	virtual bool Startup(GameObject * a_parentGameObject)
	{
		if (a_parentGameObject != NULL)
		{
			m_parentGameObject = a_parentGameObject;

			return true;
		}

		return false;
	}
	virtual bool Update(float a_dt) = 0;
	virtual bool Shutdown() { return true; }

	virtual GameObject * GetParentGameObject() { return m_parentGameObject; }

protected:
	
	GameObject * m_parentGameObject;

};

#endif //_ENGINE_COMPONENT_
