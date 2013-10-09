#ifndef _ENGINE_GAME_OBJECT_
#define _ENGINE_GAME_OBJECT_
#pragma once

#include <iostream>
#include <fstream>

#include "../core/Matrix.h"

#include "GameFile.h"
#include "StringUtils.h"

#include "Components\Component.h"

class GameObjectComponent;
class Model;
class Shader;

//\brief A GameObject is the container for all entities involved in the gameplay.
//		 It is lightweight yet has provisions for all basic game related functions like
//		 2D sprites, 3D models, events and collision.
class GameObject
{

public:

	//\brief GameObject state determines how the update effects related subsystems
	enum eGameObjectState
	{
		eGameObjectState_New = 0,	///< Object is created but not ready for life
		eGameObjectState_Loading,	///< Loading resources, shaders, models etc
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
		eClipTypeAxisBox,			///< Box with three seperate dimensions aligned to world XYZ (can't rotate)
		eClipTypeBox,				///< Box with thrree seperate dimensions

		eClipTypeCount,
	};

	//\brief Creation and destruction
	GameObject() 
		: m_id(0) 
		, m_child(NULL)
		, m_next(NULL)
		, m_model(NULL)
		, m_shader(NULL)
		, m_state(eGameObjectState_New)
		, m_lifeTime(0.0f)
		, m_clipType(eClipTypeAxisBox)
		, m_clipVolumeSize(1.0f)
		, m_clipVolumeOffset(0.0f)
		, m_worldMat(Matrix::Identity())
		, m_scriptRef(-1)
		{ 
			SetName("UNAMED_GAME_OBJECT");
			SetTemplate("");
			memset(&m_components[0], 0, sizeof(Component *) * Component::eComponentTypeCount);
		}

	~GameObject() { Destroy(); }

	//\brief Lifecycle functionality inherited by children
	virtual bool Startup() { return true; }
	virtual bool Update(float a_dt);
	virtual bool Draw();
	virtual bool Shutdown() { return true; }

	//\brief Component accessors
	template <typename T>
	inline bool AddComponent()
	{
		if (T * newComponent = new T())
		{
			newComponent->Startup(this);
			m_components[(unsigned int)newComponent->GetComponentType()] = dynamic_cast<Component *>(newComponent);
			return true;
		}
		return false;
	}
	inline bool HasComponent(Component::eComponentType a_type)
	{
		return m_components[(unsigned int)a_type] != NULL;
	}
	template <typename T>
	inline T * GetComponent(Component::eComponentType a_type)
	{
		Component * curComp = m_components[(unsigned int)a_type];
		if (curComp != NULL)
		{
			return dynamic_cast<T *>(curComp);
		}
		return NULL;
	}
	inline bool RemoveComponent(Component::eComponentType a_type)
	{
		Component * curComp = m_components[(unsigned int)a_type];
		if (curComp != NULL)
		{
			delete curComp;
			m_components[(unsigned int)a_type] = NULL;
			return true;
		}

		return false;
	}
	inline void RemoveAllComponents()
	{
		for (unsigned int i = 0; i < Component::eComponentTypeCount; ++i)
		{
			RemoveComponent((Component::eComponentType)i);
		}
	}

	//\brief State mutators and accessors
	inline void SetSleeping() { if (m_state == eGameObjectState_Active) m_state = eGameObjectState_Sleep; }
	inline void SetActive()	  { if (m_state == eGameObjectState_Sleep) m_state = eGameObjectState_Active; }
	inline bool IsActive()	  { return m_state == eGameObjectState_Active; }
	inline bool IsSleeping()  { return m_state == eGameObjectState_Sleep; }
	inline void SetId(unsigned int a_newId) { m_id = a_newId; }
	inline void SetClipType(eClipType a_newClipType) { m_clipType = a_newClipType; }
	inline void SetClipSize(const Vector & a_clipSize) { m_clipVolumeSize = a_clipSize; }
	inline void SetClipOffset(const Vector & a_clipOffset) { m_clipVolumeOffset = a_clipOffset; }
	inline void SetWorldMat(const Matrix & a_mat) { m_worldMat = a_mat; }
	inline void SetScriptReference(int a_scriptRef) { m_scriptRef = a_scriptRef; }
	inline unsigned int GetId() { return m_id; }
	inline const char * GetName() { return m_name; }
	inline const char * GetTemplate() { return m_template; }
	inline Model * GetModel() { return m_model; }
	inline Matrix GetWorldMat() const { return m_worldMat; }
	inline Shader * GetShader() const { return m_shader; }
	inline Vector GetPos() const { return m_worldMat.GetPos(); }
	inline Vector GetClipPos() const { return m_worldMat.GetPos() + m_clipVolumeOffset; }
	inline Vector GetClipSize() const { return m_clipVolumeSize; }
	inline eClipType GetClipType() const { return m_clipType; }
	inline bool HasTemplate() const { return strlen(m_template) > 0; }
	inline bool IsScriptOwned() const { return m_scriptRef >= 0; }
	inline int GetScriptReference() const { return m_scriptRef; }

	//\brief Child object accessors
	inline GameObject * GetChild() { return m_child; }
	inline GameObject * GetNext() { return m_next; }

	//\brief Collision functions
	//\param The vector(s) to check intersection against the game object
	//\return bool true if there is an intersection, false if none
	bool CollidesWith(Vector a_worldPos);
	bool CollidesWith(Vector a_lineStart, Vector a_lineEnd);

	//\brief Resource mutators and accessors
	inline void SetModel(Model * a_newModel) { m_model = a_newModel; }
	inline void SetShader(Shader * a_newShader) { m_shader = a_newShader; }
	inline void SetState(eGameObjectState a_newState) { m_state = a_newState; }
	inline void SetName(const char * a_name) { sprintf(m_name, "%s", a_name); }
	inline void SetTemplate(const char * a_templateName) { sprintf(m_template, "%s", a_templateName); }
	inline void SetPos(const Vector & a_newPos) { m_worldMat.SetPos(a_newPos); }

	//\brief Add the game object, all instance properties and children to game file object
	//\param a_outputFile is a gamefile object that will be appended
	//\param a_parent is an object in the output file to add this object's properties to
	void Serialise(GameFile * a_outputFile, GameFile::Object * a_parent);

private:

	//\brief Destruction is private as it should only be handled by object management
	void Destroy();

	//\ingroup Component management
	Component * m_components[Component::eComponentTypeCount];

	//\ingroup Local properties
	unsigned int		  m_id;					///< Unique identifier, objects can be resolved from ids
	GameObject *		  m_child;				///< Pointer to first child game obhject
	GameObject *		  m_next;				///< Pointer to sibling game objects
	Model *				  m_model;				///< Pointer to a mesh for display purposes
	Shader *			  m_shader;				///< Pointer to a shader owned by the render manager to draw with
	eGameObjectState	  m_state;				///< What state the object is in
	float				  m_lifeTime;			///< How long this guy has been active
	eClipType			  m_clipType;			///< What kind of shape represents the bounds of the object
	Vector				  m_clipVolumeSize;		///< Dimensions of the clipping volume for culling and picking
	Vector				  m_clipVolumeOffset;	///< How far from the pivot of the object the clip volume is
	Matrix				  m_worldMat;			///< Position and orientation in the world
	char				  m_name[StringUtils::s_maxCharsPerName];		///< Every creature needs a name
	char				  m_template[StringUtils::s_maxCharsPerName];	///< Every persistent, serializable creature needs a template
	int					  m_scriptRef;			///< If the object is created and managed by script, the ID on the script side is stored here
};

#endif // _ENGINE_GAME_OBJECT_
