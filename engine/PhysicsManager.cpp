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

	// Get all collisions between objects
	int numManifolds = m_dynamicsWorld->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++)
	{
		btPersistentManifold* contactManifold =  m_dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
		//btCollisionObject* obA = static_cast<btCollisionObject*>(contactManifold->getBody0());
		//btCollisionObject* obB = static_cast<btCollisionObject*>(contactManifold->getBody1());
		btCollisionObject* obA = (btCollisionObject*)(contactManifold->getBody0());
		btCollisionObject* obB = (btCollisionObject*)(contactManifold->getBody1());
		GameObject * gameObjA = (GameObject*)obA->getUserPointer();
		GameObject * gameObjB = (GameObject*)obB->getUserPointer();

		int numContacts = contactManifold->getNumContacts();
		if (numContacts > 0 && gameObjA != NULL && gameObjB != NULL)
		{
			AddCollision(gameObjA, gameObjB);
			AddCollision(gameObjB, gameObjA);
			
			/* TODO Add more information to collisions
			for (int j=0;j<numContacts;j++)
			{
				btManifoldPoint& pt = contactManifold->getContactPoint(j);
				if (pt.getDistance()<0.f)
				{
					const btVector3& ptA = pt.getPositionWorldOnA();
					const btVector3& ptB = pt.getPositionWorldOnB();
					const btVector3& normalOnB = pt.m_normalWorldOnB;
				}
			}*/
		}
	}
}

bool PhysicsManager::AddCollisionObject(GameObject * a_gameObj)
{
	if (a_gameObj == NULL)
	{
		return false;
	}

	btCollisionShape * collision = NULL;
	btRigidBody * rigidBody = NULL;
	switch (a_gameObj->GetClipType())
	{
		case ClipType::Box: 
		{	
			const Vector halfBoxSize = a_gameObj->GetClipSize() * 0.5f;
			btVector3 halfExt(halfBoxSize.GetX(), halfBoxSize.GetY(), halfBoxSize.GetZ());
			collision = new btBoxShape(halfExt);
			break;
		}
		case ClipType::Sphere:
		{
			collision = new btSphereShape(a_gameObj->GetClipSize().GetX());
			break;
		}
		default: return false;
	}

	// Setup motion state and construction info for game object shape
	btVector3 bodyPos(a_gameObj->GetPos().GetX(), a_gameObj->GetPos().GetY(), a_gameObj->GetPos().GetZ());
	btQuaternion bodyRotation(0, 0, 0, 1);
	btDefaultMotionState * motionState = new btDefaultMotionState(btTransform(bodyRotation, bodyPos));
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(0, motionState, collision, btVector3(0,0,0));
	rigidBody = new btRigidBody(rigidBodyCI);

	// Add object to physics world
	m_dynamicsWorld->addRigidBody(rigidBody);
	
	// Assign the physics object to the game object
	if (collision != NULL)
	{
		if (PhysicsObject * newObj = new PhysicsObject())
		{
			newObj->SetCollision(collision);
			newObj->SetPhysics(rigidBody);
			a_gameObj->SetPhysics(newObj);
		}
	}

	// And vice versa
	rigidBody->setUserPointer(a_gameObj);
	collision->setUserPointer(a_gameObj);
	
	return a_gameObj->GetPhysics() != NULL;
}

bool PhysicsManager::AddPhysicsObject(GameObject * a_gameObj)
{
	if (a_gameObj == NULL || a_gameObj->GetPhysics() == NULL)
	{
		return false;
	}

	// TODO activate physical properties like mass and intertia
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
	phys->GetPhysics()->setWorldTransform(newWorldTrans);
	ClearCollisions(a_gameObj);
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

void PhysicsManager::ClearCollisions(GameObject * a_gameObj)
{
	// Iterate through all objects in this file and clean up memory
	GameObject::CollisionList * colList = a_gameObj->GetCollisions();
	GameObject::Collider * cur = colList->GetHead();
	while(cur != NULL)
	{
		// Cache off next pointer and remove and deallocate memory
		GameObject::Collider * next = cur->GetNext();
		colList->Remove(cur);
		delete cur;

		cur = next;
	}
}

void PhysicsManager::AddCollision(GameObject * a_gameObjA, GameObject * a_gameObjB)
{
	if (a_gameObjA == NULL || a_gameObjB == NULL)
	{
		return;
	}

	GameObject::CollisionList * colList = a_gameObjA->GetCollisions();
	GameObject::Collider * collider = new GameObject::Collider();
	collider->SetData(a_gameObjB);
	colList->Insert(collider);
}

