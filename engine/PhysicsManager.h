#ifndef _ENGINE_PHYSICS_MANAGER
#define _ENGINE_PHYSICS_MANAGER
#pragma once

#include "..\core\BitSet.h"
#include "..\core\LinkedList.h"

#include "GameFile.h"
#include "Singleton.h"
#include "StringHash.h"

class GameObject;

//\ brief Grouping of collision and physics states, can have multiple objects and shapes
class PhysicsObject
{
public:

	PhysicsObject() = default;
	~PhysicsObject();

	inline bool HasCollision() { return false; }
	inline bool HasRigidBody() { return false; }

	inline int GetCollisionGroup() { return m_collisionGroup; }
	inline void SetCollisionGroup(int a_newGroup) { m_collisionGroup = a_newGroup; }

private:
	int m_collisionGroup{ -1 };								///< Group for collision filtering, if any
};

class PhysicsManager : public Singleton<PhysicsManager>
{
	friend class PhysicsObject;

public:

	PhysicsManager() 
		: m_dataPack(nullptr)
	{
		m_meshPath[0] = '\0';
	}
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

	//\brief Apply an impulse to the centre of the physics object in a direction
	//\param a_gameObj the object that owns the physics body to apply the force to
	//\param a_force the direction the force should be applied
	//\return true if the game object owned a rigid body that the force could be applied to
	bool ApplyForce(GameObject * a_gameObj, const Vector & a_force);

	//\\brief Get the velocity of a physics rigid body owned by a game objet
	//\param a_gameObj the object that owns the physics body
	//\return A vector of the velocity of the physics body
	Vector GetVelocity(GameObject * a_gameObj);

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

	//\brief Remove all entries from a game object collision list, called once a frame
	//		 before the physics world is queried
	//\param a_gameObj the object to affect
	void ClearCollisions(GameObject * a_gameObj);

protected:

	//\brief Add a collision between objects
	void AddCollision(GameObject * a_gameObjA, GameObject * a_gameObjB);

private:

	static const int s_maxCollisionGroups = 16;

	StringHash m_collisionGroups[s_maxCollisionGroups];
	BitSet m_collisionFilters[s_maxCollisionGroups];

	char m_meshPath[StringUtils::s_maxCharsPerLine];

	const DataPack* m_dataPack{ nullptr };									///< Pointer to a datapack to load from, if any
};

#endif //_ENGINE_PHYSICS_MANAGER