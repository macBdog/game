#ifndef _ENGINE_PHYSICS_MANAGER
#define _ENGINE_PHYSICS_MANAGER
#pragma once

#include "..\core\Matrix.h"
#include "..\core\Vector.h"
#include "../core/LinkedList.h"

#include "Singleton.h"

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionShape;
struct btDefaultMotionState;
class btRigidBody;

class GameObject;

//\ brief Grouping of collision and physics states
class PhysicsObject
{

public:

	PhysicsObject() : m_collision(NULL), m_rigidBody(NULL) { }
	~PhysicsObject();
	inline bool HasPhysics() { return m_rigidBody != NULL; }

private:

	btRigidBody * m_rigidBody;
	btCollisionShape * m_collision;
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
		, m_groundPlane() { }
	~PhysicsManager() { Shutdown(); }

	//\brief Lifecycle functions
	bool Startup();
	bool Shutdown();
	void Update(float a_dt);

	//\brief Add a bullet collision object
	//\return true if an object was added to the simulation
	bool AddCollisionObject(GameObject * a_gameObj);

	//\brief Add a bullet physically simulated object
	//\return true if an object was added to the simulation
	bool AddPhysicsObject(GameObject * a_gameObj);

protected:

	inline btDiscreteDynamicsWorld * GetDynamicsWorld() { return m_dynamicsWorld; }

private:

	typedef LinkedListNode<PhysicsObject> PhysicsObjectNode;	///< Alias for a linked list node that points to physics object
	typedef LinkedList<PhysicsObject> PhysicsObjects;			///< Alias for a linked list of objects

	btBroadphaseInterface * m_broadphase;
	btDefaultCollisionConfiguration * m_collisionConfiguration;
	btCollisionDispatcher * m_dispatcher;
	btSequentialImpulseConstraintSolver* m_solver;
	btDiscreteDynamicsWorld * m_dynamicsWorld;

	PhysicsObject * m_groundPlane;
	PhysicsObjects m_objects;
};

#endif //_ENGINE_PHYSICS_MANAGER