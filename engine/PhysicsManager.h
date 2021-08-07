#ifndef _ENGINE_PHYSICS_MANAGER
#define _ENGINE_PHYSICS_MANAGER
#pragma once

#include "..\core\BitSet.h"
#include "..\core\LinkedList.h"

#include "GameFile.h"
#include "Singleton.h"
#include "StringHash.h"

class GameObject;

//\ brief A way to try out multiple integration methods
enum class PhysicsIntegrationType : unsigned char
{
	Euler = 0,
	Verlet,
	RungeKutta,
};

//\ brief Grouping of collision and physics states, can have multiple objects and shapes
class PhysicsObject
{
	friend class PhysicsManager;
public:
	PhysicsObject(GameObject* a_owner) : m_gameObject(a_owner) 
	{
		InitialiseFromGameObject();
	}
	PhysicsObject() = delete;
	~PhysicsObject()
	{
		m_gameObject->SetPhysics(nullptr);
		m_gameObject = nullptr;
	}
	// Copy 
	PhysicsObject(PhysicsObject& a_other) 
	{
		m_gameObject = a_other.m_gameObject;
		m_gameObject->SetPhysics(this);
	};
	// Move
	PhysicsObject(PhysicsObject&& a_other) noexcept
	{
		m_gameObject = a_other.m_gameObject;
		m_gameObject->SetPhysics(this);
	}
	// Copy assignment 
	PhysicsObject& operator =(PhysicsObject& a_other) noexcept
	{
		m_gameObject = a_other.m_gameObject;
		m_gameObject->SetPhysics(this);
		return *this;
	};
	// Move assignment 
	PhysicsObject& operator =(PhysicsObject&& a_other) noexcept
	{
		m_gameObject = a_other.m_gameObject;
		m_gameObject->SetPhysics(this);
		return *this;
	};

	inline bool HasCollision() const { return false; }
	inline Vector GetVelocity() const  { return m_vel; }
	inline Vector GetPos() const { return m_pos; }
	inline Quaternion GetRot() const { return m_rot; }
	inline Vector GetForce() const { return m_force; }

	inline void SetForce(const Vector& a_newForce) { m_force = a_newForce; }
	inline void AddLinearForce(const Vector& a_newForce, const float& a_mass) { m_force += a_newForce + a_mass; }
	inline void AddLinearImpulse(const Vector& a_newImpulse, const float& a_mass) { m_vel += a_newImpulse * (1.0f / a_mass); }
	inline void SetVelocity(const Vector& a_newVel) { m_vel = a_newVel; }

protected:
	GameObject* m_gameObject{ nullptr };		///< The object that is controlled by this physics object
	Vector m_pos{};								///< Position just for simulations
	Quaternion m_rot{};							///< Angular torque affects the rotation
	Vector m_torque{};							///< Rotational force applied at a point
	Vector m_inertia{};							///< TODO: Like m_acc?
	Vector m_force{};							///< Force to be applied each sim step
	Vector m_vel{};								///< The resulting velocity
	Vector m_acc{};								///< Acceleration is accumulation of force
	Vector m_avgAcc{};
	Vector m_lastAcc{};

private:
	inline void InitialiseFromGameObject()
	{
		if (m_gameObject == nullptr)
		{
			return;
		}
		m_pos = m_gameObject->GetPos();
		m_rot = m_gameObject->GetRot();
		m_gameObject->SetPhysics(this);
	}
};

class PhysicsManager : public Singleton<PhysicsManager>
{
public:
	PhysicsManager() = default;
	~PhysicsManager() { Shutdown(); }

	//\brief Lifecycle functions
	bool Startup(const GameFile & a_config);
	bool Shutdown();

	//\brief In order of operations:	1. Solve collision world, calculate restitution and report back to the game object's collision lists
	//									2. Integrate dynamic physics and store in physics object register
	//									3. Update game object transform
	inline void Update(float a_dt)
	{
		UpdateCollisionWorld(a_dt);
		UpdatePhysicsWorld(a_dt);
		UpdateGameObjects(a_dt);
		UpdateDebugRender(a_dt);
	}
	
	//\brief Set the constant force to be applied throughout the simulation, usually done from the game config file
	inline void SetGravity(const Vector& a_gravity) { m_gravity = a_gravity; }
	inline Vector GetGravity() const { return m_gravity; }

	//\brief Add a bullet collision object
	//\param a_gameObj pointer to the game object to change
	//\return true if an object was added to the simulation
	bool AddCollisionObject(GameObject * a_gameObj);

	//\brief Add a bullet physically simulated object
	//\param a_gameObj pointer to the game object to change
	//\return true if an object was added to the simulation
	bool AddPhysicsObject(GameObject * a_gameObj);
	PhysicsObject * GetAddPhysicsObject(GameObject* a_gameObj);

	//\brief Remove dynamic physics for an object from the world
	//\param a_gameObj pointer to the game object to change
	//\return true if the physics world was affected
	bool RemovePhysicsObject(GameObject * a_gameObj);

	//\brief Remove collision for an object from the world
	//\param a_gameObj pointer to the game object to change
	//\return true if the collision world was affected
	bool RemoveCollisionObject(GameObject* a_gameObj);

	//\brief Apply an impulse to the centre of the physics object in a direction
	//\param a_gameObj the object that owns the physics body to apply the force to
	//\param a_force the direction the force should be applied
	//\return true if the game object owned a rigid body that the force could be applied to
	bool ApplyForce(GameObject * a_gameObj, const Vector & a_force);

	//\\brief Get the velocity of a physics rigid body owned by a game objet
	//\param a_gameObj the object that owns the physics body
	//\return A vector of the velocity of the physics body
	Vector GetVelocity(GameObject * a_gameObj) const;

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
	int GetCollisionGroupId(StringHash a_colGroupHash) const;
	inline int GetCollisionGroupId(const char * a_colGroupName) const { return GetCollisionGroupId(StringHash(a_colGroupName)); }

	//\brief Remove all entries from a game object collision list, called once a frame
	//		 before the physics world is queried
	//\param a_gameObj the object to affect
	void ClearCollisions(GameObject * a_gameObj);

protected:

	//\brief Add a collision between objects
	void AddCollision(GameObject * a_gameObjA, GameObject * a_gameObjB);

private:

	//\brief Helper functions to run the steps of the dynamics equation
	void UpdateCollisionWorld(const float & a_dt);
	void UpdatePhysicsWorld(const float& a_dt);
	void UpdateGameObjects(const float& a_dt);
	void UpdateDebugRender(const float& a_dt);

	static constexpr int s_maxCollisionGroups = 16;
	static constexpr float s_minPhysicsStep = 1.0f / 500.0f;
	static constexpr float s_maxPhysicsStep = 1.0f / 30.0f;

	StringHash m_collisionGroups[s_maxCollisionGroups];						///< Set of hashes of the user defined groups that collide
	BitSet m_collisionFilters[s_maxCollisionGroups];						///< Precomputed bitmask to determine if two objects should collide
	PhysicsIntegrationType m_type{ PhysicsIntegrationType::Euler };			///< What type of integration algorith will be used for the sim
	Vector m_gravity{ 0.0f, 0.0f, 0.0f };									///< Constant force applied to world wide sim
	std::vector<GameObject*> m_collisionWorld{ };							///< Every object that is checking collisions against itself
	std::vector<PhysicsObject> m_physicsWorld{ };							///< Every object we simulate dynamics with	
};

#endif //_ENGINE_PHYSICS_MANAGER