#ifndef _ENGINE_GAME_OBJECT_
#define _ENGINE_GAME_OBJECT_
#pragma once

#include <assert.h>
#include <iostream>
#include <fstream>

#include "../core/Matrix.h"
#include "../core/Quaternion.h"

#include "GameFile.h"
#ifndef _RELEASE
	#include "FileManager.h"
#endif
#include "StringHash.h"
#include "StringUtils.h"

class AnimationBlender;
class Model;
class PhysicsObject;
class Shader;

//\brief GameObject state determines how the update effects related subsystems
enum class GameObjectState : unsigned char
{
	New = 0,	///< Object is created but not ready for life
	Loading,	///< Loading resources, shaders, models etc
	Active,		///< Out and about in the world
	Sleep,		///< Hibernation, no updates or rendering, can come back from sleep
	Death,		///< Unloading and cleaning up before destruction, no coming back from death
	Count,
};

//\brief Clipping is for render culling, simple collisions for picking and specifying physics objects
enum class ClipType : unsigned char
{
	None = 0,	///< Always rendered, can't be picked
	AxisBox,	///< Box that can't be rotated
	Sphere,		///< Sphere bounding volume
	Box,		///< Box with three seperate dimensions
	Mesh,		///< Triangles defined in a separate collision mesh
	Count,
};
	
//\brief A GameObject is the container for all entities involved in the gameplay.
//		 It is lightweight yet has provisions for all basic game related functions like
//		 2D sprites, 3D models, events and collision.
class GameObject
{

public:

	//\brief Creation and destruction
	GameObject()
		: m_id(0)
		, m_child(nullptr)
		, m_next(nullptr)
		, m_model(nullptr)
		, m_shader(nullptr)
		, m_physics(nullptr)
		, m_blender(nullptr)
		, m_state(GameObjectState::New)
		, m_lifeTime(0.0f)
		, m_shaderData(0.0f)
		, m_clipType(ClipType::AxisBox)
		, m_clipVolumeSize(1.0f)
		, m_clipVolumeOffset(0.0f)
		, m_clipping(true)
		, m_visible(true)
		, m_clipGroup()
		, m_worldMat(Matrix::Identity())
		, m_localMat(Matrix::Identity())
		, m_scriptRef(-1)
		{ 
			SetName("UNNAMED_GAME_OBJECT");
			SetTemplate("");
		}

	~GameObject() { assert(m_physics == nullptr); }

	//\brief Lifecycle functionality inherited by children
	bool Startup() { return true; }
	bool Update(float a_dt);
	bool Draw();
	bool Shutdown();

	//\brief State mutators and accessors
	inline void SetSleeping() { m_state = GameObjectState::Sleep; }
	inline void SetActive()	  { m_state = GameObjectState::Active; }
	inline bool IsActive()	  { return m_state == GameObjectState::Active; }
	inline bool IsSleeping()  { return m_state == GameObjectState::Sleep; }
	inline void SetId(unsigned int a_newId) { m_id = a_newId; }
	inline void SetLifeTime(float a_newTime) { m_lifeTime = a_newTime; }
	inline void SetShaderData(const Vector & a_shaderData) { m_shaderData = a_shaderData; }
	inline void SetClipType(ClipType a_newClipType) { m_clipType = a_newClipType; }
	inline void SetClipSize(const Vector & a_clipSize) { m_clipVolumeSize = a_clipSize; }
	inline void SetClipOffset(const Vector & a_clipOffset) { m_clipVolumeOffset = a_clipOffset; }
	inline void SetClipGroup(const char * a_clipGroupName) { m_clipGroup.SetCString(a_clipGroupName); }
	inline void SetClipping(bool a_enable) { m_clipping = a_enable; }
	inline void SetPhysicsMass(const float & a_newMass) { m_physicsMass = a_newMass; }
	inline void SetPhysicsElasticity(const float & a_newElastic) { m_physicsElasticity = a_newElastic; }
	inline void SetPhysicsLinearDrag(const Vector & a_newDrag) { m_physicsLinearDrag = a_newDrag; }
	inline void SetPhysicsAngularDrag(const Vector& a_newDrag) { m_physicsAngularDrag = a_newDrag; }
	inline void SetVisible(bool a_enable) { m_visible = a_enable; }
	inline void SetWorldMat(const Matrix & a_mat) { m_worldMat = a_mat; }
	inline void SetScriptReference(int a_scriptRef) { m_scriptRef = a_scriptRef; }
	inline void SetPhysics(const std::unique_ptr<PhysicsObject>& a_physics) { m_physics = a_physics.get(); }
	
	inline unsigned int GetId() const { return m_id; }
	inline const char * GetName() const { return m_name; }
	inline const char * GetTemplate() const { return m_template; }
	inline Model * GetModel() const { return m_model; }
	inline float GetLifeTime() const { return m_lifeTime; }
	inline Vector GetShaderData() const { return m_shaderData; }
	inline Matrix & GetLocalMat() { return m_localMat; }
	inline Matrix & GetWorldMat() { return m_worldMat; }
	inline Shader * GetShader() const { return m_shader; }
	inline Vector GetPos() const { return  m_worldMat.GetPos() + m_localMat.GetPos(); }
	inline Vector GetClipPos() const { return m_worldMat.GetPos() + m_localMat.GetPos() + m_clipVolumeOffset; }
	inline Vector GetClipOffset() { return m_clipVolumeOffset; }
	inline Vector GetClipSize() const { return m_clipVolumeSize; }
	inline ClipType GetClipType() const { return m_clipType; }
	inline StringHash GetClipGroup() const { return m_clipGroup; }
	inline float GetPhysicsMass() const { return m_physicsMass; }
	inline float GetPhysicsElasticity() const { return m_physicsElasticity; }
	inline Vector GetPhysicsLinearDrag() const { return m_physicsLinearDrag; }
	inline Vector GetPhysicsAngularDrag() const { return m_physicsAngularDrag; }
	inline bool HasTemplate() const { return strlen(m_template) > 0; }
	inline bool IsScriptOwned() const { return m_scriptRef >= 0; }
	inline bool IsClipping() const { return m_clipping; }
	inline int GetScriptReference() const { return m_scriptRef; }
	inline PhysicsObject * GetPhysics() const { return m_physics; }
	Quaternion GetRot() const;
	Vector GetScale() const;
	
	//\brief Resource mutators and accessors
	inline void SetModel(Model * a_newModel) { m_model = a_newModel; }
	inline void SetShader(Shader * a_newShader) { m_shader = a_newShader; }
	inline void SetState(GameObjectState a_newState) { m_state = a_newState; }
	inline void SetName(const char * a_name) { strncpy(m_name, a_name, StringUtils::s_maxCharsPerName); }
	void SetTemplate(const char * a_templateName);
	inline void SetPos(const Vector & a_newPos) { m_worldMat.SetPos(a_newPos); }
	inline void SetScale(const Vector & a_newScale) { m_worldMat.SetScale(a_newScale); }
	inline void MulScale(const Vector & a_newScale) { m_worldMat.MulScale(a_newScale); }
	inline void RemoveScale() { m_worldMat.RemoveScale(); }
	void SetRot(const Vector & a_newRot);
	void SetRot(const Quaternion & a_rot);
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
	inline bool HasAnimationBlender() { return m_blender != nullptr; }
	inline AnimationBlender * GetAnimationBlender() { return m_blender; }
	inline void SetAnimationBlender(AnimationBlender * a_newBlender) { m_blender = a_newBlender; }

	//\brief Add the game object, all instance properties and children to game file object
	//\param a_outputFile is a gamefile object that will be appended
	//\param a_parent is an object in the output file to add this object's properties to
	void Serialise(GameFile * a_outputFile, GameFile::Object * a_parent);
	void SerialiseTemplate();

	static const char * s_clipTypeStrings[static_cast<int>(ClipType::Count)];				///< String literals for the clip types

private:
	
	//\brief Reset member data from any template properties that exist
	void SetTemplateProperties();

	unsigned int			m_id;										///< Unique identifier, objects can be resolved from ids
	GameObject *			m_child;									///< Pointer to first child game obhject
	GameObject *			m_next;										///< Pointer to sibling game objects
	CollisionList			m_collisions;								///< List of objects that this game object has collided with this frame
	Model *					m_model;									///< Pointer to a mesh for display purposes
	Shader *				m_shader;									///< Pointer to a shader owned by the render manager to draw with
	PhysicsObject *			m_physics;									///< Pointer to physics manager object for collisions and dynamics
	AnimationBlender *		m_blender;									///< Pointer to an animation blender if present
	GameObjectState			m_state;									///< What state the object is in
	float					m_lifeTime;									///< How long this guy has been active
	Vector					m_shaderData;								///< 3 floats to transmit to the shader
	ClipType				m_clipType;									///< What kind of shape represents the bounds of the object
	Vector					m_clipVolumeSize;							///< Dimensions of the clipping volume for culling and picking
	Vector					m_clipVolumeOffset;							///< How far from the pivot of the object the clip volume is
	bool					m_clipping{ false };						///< If collision is enabled
	float					m_physicsMass{ 1.0f };						///< Mass of the object being simulated
	float					m_physicsElasticity{ 1.0f };				///< How much force is retained from collisions
	Vector					m_physicsLinearDrag{ 1.0f };				///< How much linear inertia is lost per time step
	Vector					m_physicsAngularDrag{ 1.0f };				///< How much rotational torque is lost per time step
	bool					m_visible{ true };							///< If the game object's model should be added to the render list
	StringHash				m_clipGroup;								///< What group the object belongs to and can collide with
	Matrix					m_worldMat;									///< Position and orientation in the world
	Matrix					m_localMat;									///< Position and orientation relative to world mat, used for animation
	Matrix					m_finalMat;									///< Aggregate of world and local only used by render
	char					m_name[StringUtils::s_maxCharsPerName];		///< Every creature needs a name
	char				  m_template[StringUtils::s_maxCharsPerName];	///< Every persistent, serializable creature needs a template
#ifndef _RELEASE
	FileManager::Timestamp m_templateTimeStamp;							///< For auto-reloading of templates
#endif
	int					  m_scriptRef;									///< If the object is created and managed by script, the ID on the script side is stored here
};

#endif // _ENGINE_GAME_OBJECT_
