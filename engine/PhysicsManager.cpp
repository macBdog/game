#include <btBulletDynamicsCommon.h>

#include "GameObject.h"
#include "Log.h"

#include "PhysicsManager.h"

template<> PhysicsManager * Singleton<PhysicsManager>::s_instance = NULL;

PhysicsObject::~PhysicsObject()
{
	// Clean up physics if present
	if (m_rigidBody != NULL)
	{
		if (btDiscreteDynamicsWorld * dynWorld = PhysicsManager::Get().GetDynamicsWorld())
		{
			dynWorld->removeRigidBody(m_rigidBody);
		}
		delete m_rigidBody->getMotionState();
		delete m_rigidBody;
	}

	// Clean up collision
	if (m_collision != NULL)
	{
		delete m_collision;
	}
}

bool PhysicsManager::Startup()
{
	// Initialise physics world
	m_broadphase = new btDbvtBroadphase();
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_solver = new btSequentialImpulseConstraintSolver;
	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);
	m_dynamicsWorld->setGravity(btVector3(0.0f, 0.0f, -10.0f));

	return m_dynamicsWorld != NULL;
}

bool PhysicsManager::Shutdown()
{
	if (m_dynamicsWorld == NULL)
	{
		return false;
	}

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

bool PhysicsManager::AddCollisionObject(GameObject * a_gameObj)
{
	if (a_gameObj == NULL)
	{
		return false;
	}

	btCollisionShape * collision = NULL;
	switch (a_gameObj->GetClipType())
	{
		case GameObject::eClipTypeBox: 
		{	
			const Vector halfBoxSize = a_gameObj->GetClipSize() * 0.5f;
			btVector3 halfExt(halfBoxSize.GetX(), halfBoxSize.GetY(), halfBoxSize.GetZ());
			collision = new btBoxShape(halfExt);
			break;
		}
		case GameObject::eClipTypeSphere:
		{
			collision = new btSphereShape(a_gameObj->GetClipSize().GetX());
			break;
		}
		default: break;
	}

	if (collision != NULL)
	{
		if (PhysicsObject * newObj = new PhysicsObject())
		{
			newObj->SetCollision(collision);
			a_gameObj->SetPhysics(newObj);
		}
	}
	
	return a_gameObj->GetPhysics() != NULL;
}

bool PhysicsManager::AddPhysicsObject(GameObject * a_gameObj)
{
	if (a_gameObj == NULL || a_gameObj->GetPhysics() == NULL)
	{
		return false;
	}

	PhysicsObject * phys = a_gameObj->GetPhysics();
	if (!phys->HasPhysics() && m_dynamicsWorld)
	{
		// Setup motion state and construction info for game object shape
		btVector3 bodyShape(a_gameObj->GetClipSize().GetX(), a_gameObj->GetClipSize().GetY(), a_gameObj->GetClipSize().GetZ());
		btQuaternion bodyRotation(0, 0, 0, 1);
		btDefaultMotionState * motionState = new btDefaultMotionState(btTransform(bodyRotation, bodyShape));
		btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(0, motionState, phys->GetCollision(), btVector3(0,0,0));
		btRigidBody * newBody = new btRigidBody(rigidBodyCI);
		phys->SetPhysics(newBody);

		// Add object to physics world
		m_dynamicsWorld->addRigidBody(newBody);
	}

	return false;
}

void PhysicsManager::UpdateGameObject(GameObject * a_gameObj)
{
	if (a_gameObj == NULL)
	{
		return;
	}

	PhysicsObject * phys = a_gameObj->GetPhysics();
	btTransform newWorldTrans;
	btVector3 trans(a_gameObj->GetPos().GetX(), a_gameObj->GetPos().GetY(), a_gameObj->GetPos().GetZ());
	newWorldTrans.setOrigin(trans);
	//phys->GetPhysics()->setWorldTransform(newWorldTrans);
}

bool PhysicsManager::RemovePhysicsObject(GameObject * a_gameObj)
{
	if (a_gameObj != NULL)
	{
		PhysicsObject * phys = a_gameObj->GetPhysics();
		if (phys != NULL)
		{
			delete phys;
			a_gameObj->SetPhysics(NULL);
			return true;
		}
	}
	return false;
}