#ifndef _ENGINE_GAME_OBJECT_
#define _ENGINE_GAME_OBJECT_
#pragma once

#include "GameObjectComponent.h"
#include "StringUtils.h"

class GameObject
{

public:

	//\brief GameObject state determines how the update effects related subsystems
	enum eGameObjectState
	{
		eGameObjectState_New = 0,	// Object is created but not ready for life
		eGameObjectState_Loading,	// Loading resources, scripts, models etc
		eGameObjectState_Active,	// Out and about in the world
		eGameObjectState_Sleep,		// Hibernation, no updates or rendering
		eGameObjectState_Death,		// Unloading and cleaning up before destruction

		eGameObjectState_Count,
	};

	//\brief Creation and destruction
	GameObject(unsigned int a_id) 
		: m_id(a_id) 
		, m_state(eGameObjectState_New)
		, m_child(NULL)
		, m_next(NULL)
		{ }

	~GameObject() { Destroy(); }

	//\brief Functionality inherited by children
	virtual bool Update(float a_dt) { return true; }
	virtual bool Draw() { return true;}

	//\brief State mutators and accessors
	inline void SetSleeping() { if (m_state == eGameObjectState_Active) m_state = eGameObjectState_Sleep; }
	inline void SetActive()	  { if (m_state == eGameObjectState_Sleep) m_state = eGameObjectState_Active; }
	inline bool IsActive()	  { return m_state == eGameObjectState_Active; }
	inline bool IsSleeping()  { return m_state == eGameObjectState_Sleep; }
	inline unsigned int GetId() { return m_id; }

	//\brief Child object accessors
	inline GameObject * GetChild() { return m_child; }
	inline GameObject * GetNext() { return m_next; }

private:

	//\brief Destruction is private as it should only be handled by object management
	void Destroy();

	unsigned int		  m_id;				///< Unique identifier, objects can be resolved from ids
	GameObject *		  m_child;			///< Pointer to first child game obhject
	GameObject *		  m_next;			///< Pointer to sibling game objects
	GameObjectComponent * m_components;		///< Purpose built object features live in a list of components
	//Script			  m_script;			///< The lua script for user defined behavior
	eGameObjectState	  m_state;			///< What state the object is in
	char				  m_name[StringUtils::s_maxCharsPerName];	///< Every object needs a name

};

#endif // _ENGINE_GAME_OBJECT_
