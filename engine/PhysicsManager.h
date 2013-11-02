#ifndef _ENGINE_PHYSICS_MANAGER
#define _ENGINE_PHYSICS_MANAGER
#pragma once

#include "..\core\Matrix.h"
#include "..\core\Vector.h"

#include "Singleton.h"

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionShape;
struct btDefaultMotionState;
class btRigidBody;

class PhysicsManager : public Singleton<PhysicsManager>
{

public:

	PhysicsManager() { }
	~PhysicsManager() { Shutdown(); }

	//\brief Lifecycle functions
	bool Startup();
	bool Shutdown();
	void Update(float a_dt);

private:

	btBroadphaseInterface * m_broadphase;
	btDefaultCollisionConfiguration * m_collisionConfiguration;
	btCollisionDispatcher * m_dispatcher;
	btSequentialImpulseConstraintSolver* m_solver;
	btDiscreteDynamicsWorld * m_dynamicsWorld;

	btDefaultMotionState * m_groundMotionState;
	btCollisionShape * m_groundCollision;
	btRigidBody * m_groundPlane;
};

#endif //_ENGINE_PHYSICS_MANAGER