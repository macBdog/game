#ifndef _ENGINE_PHYSICS_MANAGER
#define _ENGINE_PHYSICS_MANAGER
#pragma once

#include "..\core\BitSet.h"

#include "GameFile.h"
#include "Singleton.h"
#include "StringHash.h"

class btBulletWorldImporter;
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionShape;
class btCollisionObject;
class btCollisionWorld;
struct btDefaultMotionState;
class btRigidBody;

class GameObject;

//\ brief Grouping of collision and physics states
class PhysicsObject
{
public:

	PhysicsObject() 
		: m_collisionGroup(-1)
		, m_collisionShape(NULL)
		, m_collisionObject(NULL)
		, m_rigidBody(NULL)
		, m_fileLoader(NULL) { }
	~PhysicsObject();
	inline bool HasCollision() { return m_collisionShape != NULL; }
	inline bool HasRigidBody() { return m_rigidBody != NULL; }
	
	inline int GetCollisionGroup() { return m_collisionGroup; }
	inline btCollisionShape * GetCollisionShape() { return m_collisionShape; }
	inline btCollisionObject * GetCollisionObject() { return m_collisionObject; }
	inline btRigidBody * GetRigidBody() { return m_rigidBody; }
	inline btBulletWorldImporter * GetFileLoader() { return m_fileLoader; }

	inline void SetCollisionGroup(int a_newGroup) { m_collisionGroup = a_newGroup; }
	inline void SetCollisionShape(btCollisionShape * a_col) { if (m_collisionShape == NULL) { m_collisionShape = a_col; } }
	inline void SetCollisionObject(btCollisionObject * a_col) { if (m_collisionObject == NULL) { m_collisionObject = a_col; } }
	inline void SetRigidBody(btRigidBody * a_phy) { if (m_rigidBody == NULL) { m_rigidBody = a_phy; } }
	inline void SetFileLoader(btBulletWorldImporter * a_loader) { m_fileLoader = a_loader; }
	
private:

	int m_collisionGroup;					///< Group for collision filtering, if any
	btRigidBody * m_rigidBody;				///< Rigid body present only if object driven by dynamics
	btCollisionShape * m_collisionShape;	///< Dimensions and type of volume for collisions and dynamics if present
	btCollisionObject * m_collisionObject;	///< All objects are represented in the collision world
	btBulletWorldImporter * m_fileLoader;	///< Loader and memory for objects loaded from a bullet binary file
};

class PhysicsManager : public Singleton<PhysicsManager>
{
	friend class PhysicsObject;

public:

	PhysicsManager() 
		: m_broadphase(NULL)
		, m_collisionConfiguration(NULL)
		, m_dispatcher(NULL)
		, m_solver(NULL)
		, m_dynamicsWorld(NULL) 
		, m_collisionWorld(NULL) { m_meshPath[0] = '\0'; }
	~PhysicsManager() { Shutdown(); }

	//\brief Lifecycle functions
	bool Startup(const GameFile & a_config, const char * a_meshPath, const DataPack * a_dataPack);
	bool Shutdown();
	void Update(float a_dt);

	//\brief Add a bullet collision object
	//\param a_gameObj pointer to the game object to change
	//\return true if an object was added to the simulation
	bool AddCollisionObject(GameObject * a_gameObj);

	//\brief Add a bullet physically simulated object
	//\param a_gameObj pointer to the game object to change
	//\return true if an object was added to the simulation
	bool AddPhysicsObject(GameObject * a_gameObj);

	//\brief Sync transform of physics world object with game object
	//\param a_gameObj pointer to the game object to change
	void UpdateGameObject(GameObject * a_gameObj);

	//\brief Remove collision and rigid body physics for an object from the world
	//\param a_gameObj pointer to the game object to change
	//\return true if the physics world was affected
	bool RemovePhysicsObject(GameObject * a_gameObj);

	//\brief Cast a ray at the collision world and retrieve the results immediately
	//\param a_rayStart is the start point in worldspace of the ray
	//\param a_rayEnd is the end point in worldspace of the ray
	//\param a_worldHit_OUT will be written to with the world position of the first hit, if any
	//\param a_worldNormal_OUT will be written to with the normal of the first hit, if any
	//\param a_gameObjHit_OUT will a pointer to a game object that the ray collided with, if any
	//\return true if the ray hit some collision object, false if not
	bool RayCast(const Vector & a_rayStart, const Vector & a_rayEnd, Vector & a_worldHit_OUT, Vector & a_worldNormal_OUT);

	//\brief Get the group ID matching the name of a collision group
	//\return Collision group id, -1 means not found, 0 means nothing, > 0 is a valid group
	int GetCollisionGroupId(StringHash a_colGroupHash);
	inline int GetCollisionGroupId(const char * a_colGroupName) { return GetCollisionGroupId(StringHash(a_colGroupName)); }
	void ClearCollisions(GameObject * a_gameObj);

protected:

	//\brief Remove all entries from a game object collision list, called once a frame
	//		 before the physics world is queried
	//\param a_gameObj the object to affect
	void AddCollision(GameObject * a_gameObjA, GameObject * a_gameObjB);

	//\brief Accessor for game systems to query the physics world
	inline btDiscreteDynamicsWorld * GetDynamicsWorld() { return m_dynamicsWorld; }
	inline btCollisionWorld * GetCollisionWorld() { return m_collisionWorld; }

private:

	static const int s_maxCollisionGroups = 16;

	StringHash m_collisionGroups[s_maxCollisionGroups];
	BitSet m_collisionFilters[s_maxCollisionGroups];

	char m_meshPath[StringUtils::s_maxCharsPerLine];

	btBroadphaseInterface * m_broadphase;
	btDefaultCollisionConfiguration * m_collisionConfiguration;
	btCollisionDispatcher * m_dispatcher;
	btSequentialImpulseConstraintSolver * m_solver;
	btDiscreteDynamicsWorld * m_dynamicsWorld;
	btCollisionWorld * m_collisionWorld;
};

#endif //_ENGINE_PHYSICS_MANAGER