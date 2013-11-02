#include <btBulletDynamicsCommon.h>

#include "Log.h"

#include "PhysicsManager.h"

template<> PhysicsManager * Singleton<PhysicsManager>::s_instance = NULL;

bool PhysicsManager::Startup()
{
	// Initialise physics world
	m_broadphase = new btDbvtBroadphase();
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_solver = new btSequentialImpulseConstraintSolver;
	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);
	m_dynamicsWorld->setGravity(btVector3(0.0f, 0.0f, -10.0f));

	// Add ground plane physics and collision
	btCollisionShape* m_groundCollision = new btStaticPlaneShape(btVector3(0,0,1),1);
	btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,0,-1)));
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, m_groundCollision, btVector3(0,0,0));
    btRigidBody* m_groundPlane = new btRigidBody(groundRigidBodyCI);

	// Add ground to physics world
	m_dynamicsWorld->addRigidBody(m_groundPlane);

	return m_dynamicsWorld != NULL;
}

bool PhysicsManager::Shutdown()
{
	if (m_dynamicsWorld == NULL)
	{
		return false;
	}

	// Remove ground plane and clean up
	m_dynamicsWorld->removeRigidBody(m_groundPlane);
	delete m_groundMotionState;
	delete m_groundPlane;
	delete m_groundCollision;

	// Clean up world
	delete m_dynamicsWorld;
    delete m_solver;
    delete m_dispatcher;
    delete m_collisionConfiguration;
    delete m_broadphase;

	
	
	return true;
}

void PhysicsManager::Update(float a_dt)
{
	if (m_dynamicsWorld == NULL)
	{
		return;
	}

	m_dynamicsWorld->stepSimulation(a_dt, 10);
}

