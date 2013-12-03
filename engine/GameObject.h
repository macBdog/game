#ifndef _ENGINE_GAME_OBJECT_
#define _ENGINE_GAME_OBJECT_
#pragma once

#include <iostream>
#include <fstream>

#include "../core/Matrix.h"

#include "GameFile.h"
#include "StringHash.h"
#include "StringUtils.h"

class AnimationBlender;
class Model;
class PhysicsObject;
class Shader;

//\brief GameObject state determines how the update effects related subsystems
namespace GameObjectState
{
	enum Enum
	{
		New = 0,	///< Object is created but not ready for life
		Loading,	///< Loading resources, shaders, models etc
		Active,		///< Out and about in the world
		Sleep,		///< Hibernation, no updates or rendering, can come back from sleep
		Death,		///< Unloading and cleaning up before destruction, no coming back from death
		Count,
	};
}

//\brief Clipping is for render culling and simple collisions for picking 
namespace ClipType
{
	enum Enum
	{
		None = 0,	///< Always rendered, can't be picked
		AxisBox,	//< Box that can't be rotated
		Sphere,		///< Sphere bounding volume
		Box,		///< Box with three seperate dimensions
		Count,
	};
}
	
//\brief A GameObject is the container for all entities involved in the gameplay.
//		 It is lightweight yet has provisions for all basic game related functions like
//		 2D sprites, 3D models, events and collision.
class GameObject
{

public:

	//\brief Creation and destruction
	GameObject() 
		: m_id(0) 
		, m_child(NULL)
		, m_next(NULL)
		, m_model(NULL)
		, m_shader(NULL)
		, m_physics(NULL)
		, m_blender(NULL)
		, m_state(GameObjectState::New)
		, m_lifeTime(0.0f)
		, m_clipType(ClipType::AxisBox)
		, m_clipVolumeSize(1.0f)
		, m_clipVolumeOffset(0.0f)
		, m_clipping(true)
		, m_clipGroup()
		, m_worldMat(Matrix::Identity())
		, m_localMat(Matrix::Identity())
		, m_scriptRef(-1)
		{ 
			SetName("UNAMED_GAME_OBJECT");
			SetTemplate("");
		}

	~GameObject() { Destroy(); }

	//\brief Lifecycle functionality inherited by children
	bool Startup() { return true; }
	bool Update(float a_dt);
	bool Draw();
	bool Shutdown() { return true; }

	//\brief State mutators and accessors
	inline void SetSleeping() { if (m_state == GameObjectState::Active) m_state = GameObjectState::Sleep; }
	inline void SetActive()	  { if (m_state == GameObjectState::Sleep) m_state = GameObjectState::Active; }
	inline bool IsActive()	  { return m_state == GameObjectState::Active; }
	inline bool IsSleeping()  { return m_state == GameObjectState::Sleep; }
	inline void SetId(unsigned int a_newId) { m_id = a_newId; }
	inline void SetClipType(ClipType::Enum a_newClipType) { m_clipType = a_newClipType; }
	inline void SetClipSize(const Vector & a_clipSize) { m_clipVolumeSize = a_clipSize; }
	inline void SetClipOffset(const Vector & a_clipOffset) { m_clipVolumeOffset = a_clipOffset; }
	inline void SetClipGroup(const char * a_clipGroupName) { m_clipGroup.SetCString(a_clipGroupName); }
	inline void SetClipping(bool a_enable) { m_clipping = a_enable; }
	inline void SetWorldMat(const Matrix & a_mat) { m_worldMat = a_mat; }
	inline void SetScriptReference(int a_scriptRef) { m_scriptRef = a_scriptRef; }
	inline void SetPhysics(PhysicsObject * a_physics) { m_physics = a_physics; }
	inline unsigned int GetId() { return m_id; }
	inline const char * GetName() { return m_name; }
	inline const char * GetTemplate() { return m_template; }
	inline Model * GetModel() { return m_model; }
	inline Matrix & GetLocalMat() { return m_localMat; }
	inline Shader * GetShader() const { return m_shader; }
	inline Vector GetPos() const { return  m_worldMat.GetPos() + m_localMat.GetPos(); }
	inline Vector GetClipPos() const { return m_worldMat.GetPos() + m_localMat.GetPos() + m_clipVolumeOffset; }
	inline Vector GetClipSize() const { return m_clipVolumeSize; }
	inline ClipType::Enum GetClipType() const { return m_clipType; }
	inline StringHash GetClipGroup() const { return m_clipGroup; }
	inline bool HasTemplate() const { return strlen(m_template) > 0; }
	inline bool IsScriptOwned() const { return m_scriptRef >= 0; }
	inline int GetScriptReference() const { return m_scriptRef; }
	inline PhysicsObject * GetPhysics() const { return m_physics; }
	Vector GetRot() const;
	
	//\brief Resource mutators and accessors
	inline void SetModel(Model * a_newModel) { m_model = a_newModel; }
	inline void SetShader(Shader * a_newShader) { m_shader = a_newShader; }
	inline void SetState(GameObjectState::Enum a_newState) { m_state = a_newState; }
	inline void SetName(const char * a_name) { sprintf(m_name, "%s", a_name); }
	inline void SetTemplate(const char * a_templateName) { sprintf(m_template, "%s", a_templateName); }
	inline void SetPos(const Vector & a_newPos) { m_worldMat.SetPos(a_newPos); }
	void SetRot(const Vector & a_newRot);
	void AddRot(const Vector & a_rot);

	//\brief Child object accessors
	inline GameObject * GetChild() { return m_child; }
	inline GameObject * GetNext() { return m_next; }

	//\ingroup Collision
	typedef LinkedListNode<GameObject> Collider;		///< Alias for passing lists of game objects around
	typedef LinkedList<GameObject> CollisionList;		///< Alias for passing lists of game objects around

	//\brief Collision utility functions
	inline CollisionList * GetCollisions() { return &m_collisions; }
	inline bool HasColliders() { return !m_collisions.IsEmpty(); }
	
	//\brief Realtime collision functions
	//\param The vector(s) to check intersection against the game object
	//\return bool true if there is an intersection, false if none
	bool CollidesWith(Vector a_worldPos);
	bool CollidesWith(GameObject * a_collider);
	bool CollidesWith(Vector a_lineStart, Vector a_lineEnd);

	//\ingroup Animation
	inline bool HasAnimationBlender() { return m_blender != NULL; }
	inline AnimationBlender * GetAnimationBlender() { return m_blender; }
	inline void SetAnimationBlender(AnimationBlender * a_newBlender) { m_blender = a_newBlender; }

	//\brief Add the game object, all instance properties and children to game file object
	//\param a_outputFile is a gamefile object that will be appended
	//\param a_parent is an object in the output file to add this object's properties to
	void Serialise(GameFile * a_outputFile, GameFile::Object * a_parent);

private:

	//\brief Destruction is private as it should only be handled by object management
	void Destroy();

	//\ingroup Local properties
	unsigned int		  m_id;					///< Unique identifier, objects can be resolved from ids
	GameObject *		  m_child;				///< Pointer to first child game obhject
	GameObject *		  m_next;				///< Pointer to sibling game objects
	CollisionList		  m_collisions;			///< List of objects that this game object has collided with this frame
	Model *				  m_model;				///< Pointer to a mesh for display purposes
	Shader *			  m_shader;				///< Pointer to a shader owned by the render manager to draw with
	PhysicsObject *		  m_physics;			///< Pointer to physics manager object for collisions and dynamics
	AnimationBlender *	  m_blender;			///< Pointer to an animation blender if present
	GameObjectState::Enum m_state;				///< What state the object is in
	float				  m_lifeTime;			///< How long this guy has been active
	ClipType::Enum		  m_clipType;			///< What kind of shape represents the bounds of the object
	Vector				  m_clipVolumeSize;		///< Dimensions of the clipping volume for culling and picking
	Vector				  m_clipVolumeOffset;	///< How far from the pivot of the object the clip volume is
	bool				  m_clipping;			///< If collision is enabled
	StringHash			  m_clipGroup;			///< What group the object belongs to
	Matrix				  m_worldMat;			///< Position and orientation in the world
	Matrix				  m_localMat;			///< Position and orientation relative to world mat, used for animation
	Matrix				  m_finalMat;			///< Aggregate of world and local only used by render
	char				  m_name[StringUtils::s_maxCharsPerName];		///< Every creature needs a name
	char				  m_template[StringUtils::s_maxCharsPerName];	///< Every persistent, serializable creature needs a template
	int					  m_scriptRef;			///< If the object is created and managed by script, the ID on the script side is stored here
};

#endif // _ENGINE_GAME_OBJECT_
