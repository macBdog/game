#ifndef _ENGINE_GAME_OBJECT_
#define _ENGINE_GAME_OBJECT_
#pragma once

#include <iostream>
#include <fstream>

#include "../core/Matrix.h"

#include "StringUtils.h"

class GameObjectComponent;
class Model;

//\brief A GameObject is the container for all entities involved in the gameplay.
//		 It is lightweight yet has provisions for all basic game related functions like
//		 2D sprites, 3D models, scripts, events and collision.
class GameObject
{

public:

	//\brief GameObject state determines how the update effects related subsystems
	enum eGameObjectState
	{
		eGameObjectState_New = 0,	///< Object is created but not ready for life
		eGameObjectState_Loading,	///< Loading resources, scripts, models etc
		eGameObjectState_Active,	///< Out and about in the world
		eGameObjectState_Sleep,		///< Hibernation, no updates or rendering, can come back from sleep
		eGameObjectState_Death,		///< Unloading and cleaning up before destruction, no coming back from death

		eGameObjectState_Count,
	};

	//\brief Clipping is for render culling and simple collisions for picking 
	enum eClipType
	{
		eClipTypeNone = 0,			///< Always rendered, can't be picked
		eClipTypeSphere,			///< Sphere bounding volume
		eClipTypeAABB,				///< Box with three seperate dimensions aligned to world XYZ (can't rotate)
		eClipTypeCube,				///< Sphere bounding volume
		eClipTypeBox,				///< Box with thrree seperate dimensions

		eClipTypeCount,
	};

	//\brief Creation and destruction
	GameObject(unsigned int a_id) 
		: m_id(a_id) 
		, m_child(NULL)
		, m_next(NULL)
		, m_components(NULL)
		, m_model(NULL)
		, m_state(eGameObjectState_New)
		, m_lifeTime(0.0f)
		, m_clipType(eClipTypeNone)
		, m_clipVolumeSize(0.0f)
		, m_worldMat(Matrix::Identity())
		{ }

	~GameObject() { Destroy(); }

	//\brief Functionality inherited by children
	virtual bool Update(float a_dt);
	virtual bool Draw();

	//\brief State mutators and accessors
	inline void SetSleeping() { if (m_state == eGameObjectState_Active) m_state = eGameObjectState_Sleep; }
	inline void SetActive()	  { if (m_state == eGameObjectState_Sleep) m_state = eGameObjectState_Active; }
	inline bool IsActive()	  { return m_state == eGameObjectState_Active; }
	inline bool IsSleeping()  { return m_state == eGameObjectState_Sleep; }
	inline unsigned int GetId() { return m_id; }
	inline const char * GetName() { return m_name; }
	inline Vector GetPos() { return m_worldMat.GetPos(); }
	inline Vector GetRot() { return m_worldMat.GetPos(); } // TODO
	inline Vector GetScale() { return m_worldMat.GetPos(); } //TODO

	//\brief Child object accessors
	inline GameObject * GetChild() { return m_child; }
	inline GameObject * GetNext() { return m_next; }

	//\brief Resource mutators and accessors
	inline void SetModel(Model * a_newModel) { m_model = a_newModel; }
	//inline void SetScript(Script * a_newScript) { m_script = a_newScript; }
	inline void SetState(eGameObjectState a_newState) { m_state = a_newState; }
	inline void SetName(const char * a_name) { sprintf(m_name, "%s", a_name); }
	inline void SetPos(Vector & a_newPos) { m_worldMat.SetPos(a_newPos); }

	//\brief Write the game object, all instance properties and children to a file stream
	//\param a_outputStream is a pointer to an output stream to write to
	//\param a_indentCount is an optional number of tab character to prefix each line with
	void Serialise(std::ofstream * a_outputStream, unsigned int a_indentCount = 0);

private:

	//\brief Destruction is private as it should only be handled by object management
	void Destroy();

	unsigned int		  m_id;				///< Unique identifier, objects can be resolved from ids
	GameObject *		  m_child;			///< Pointer to first child game obhject
	GameObject *		  m_next;			///< Pointer to sibling game objects
	GameObjectComponent * m_components;		///< Purpose built object features live in a list of components
	Model *				  m_model;			///< Pointer to a mesh for display purposes
	//Script			  m_script;			///< The LUA script for user defined behavior
	eGameObjectState	  m_state;			///< What state the object is in
	float				  m_lifeTime;		///< How long this guy has been active
	eClipType			  m_clipType;		///< What kind of shape represents the bounds of the object
	Vector				  m_clipVolumeSize; ///< Dimensions of the clipping volume for culling and picking
	Matrix				  m_worldMat;		///< Position and orientation in the world
	char				  m_name[StringUtils::s_maxCharsPerName];	///< Every creature needs a name

};

#endif // _ENGINE_GAME_OBJECT_
