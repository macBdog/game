#include <btBulletDynamicsCommon.h>
#include "btBulletWorldImporter.h"

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

	// And collision object
	if (m_collisionObject != NULL)
	{
		if (btCollisionWorld * colWorld = PhysicsManager::Get().GetCollisionWorld())
		{
			colWorld->removeCollisionObject(m_collisionObject);
		}
		delete m_collisionObject;
	}

	// Now shape
	if (m_collisionShape != NULL)
	{
		delete m_collisionShape;
	}
}

bool PhysicsManager::Startup(const GameFile & a_config, const char * a_meshPath)
{
	// Cache off path to bullet files
	if (a_meshPath != NULL && a_meshPath[0] != '\0')
	{
		strncpy(m_meshPath, a_meshPath, sizeof(char) * strlen(a_meshPath) + 1);
	}

	// Initialise physics world
	m_broadphase = new btDbvtBroadphase();
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_solver = new btSequentialImpulseConstraintSolver;
	btVector3 worldAabbMin(-1000,-1000,-1000);
	btVector3 worldAabbMax(1000,1000,1000);

	btAxisSweep3*	broadphase = new btAxisSweep3(worldAabbMin,worldAabbMax);
	
	m_collisionWorld = new btCollisionWorld(m_dispatcher, broadphase,m_collisionConfiguration);

	// First group is always nothing, user groups start at index 1
	m_collisionGroups[0] = StringHash("Nothing");

	// Setup collision groups and flags from config file
	if (GameFile::Object * colConfig = a_config.FindObject("collision"))
	{
		if (GameFile::Object * colGroups = colConfig->FindObject("groups"))
		{
			char colGroupName[StringUtils::s_maxCharsPerName];
			colGroupName[0] = '\0';
			for (int i = 1; i < s_maxCollisionGroups; ++i)
			{
				sprintf(colGroupName, "group%d", i);
				if (GameFile::Property * colGroupProp = colGroups->FindProperty(colGroupName))
				{
					m_collisionGroups[i] = StringHash(colGroupProp->GetString());
				}
			}
		}

		if (GameFile::Object * colFilters = colConfig->FindObject("filters"))
		{
			for (int i = 1; i < s_maxCollisionGroups; ++i)
			{
				if (GameFile::Property * filterProp = colFilters->FindProperty(m_collisionGroups[i].GetCString()))
				{
					// If the collision filter is a single entry
					const char * curFilters = filterProp->GetString();
					if (strstr(curFilters, ",") == NULL)
					{
						int colFilterInListId = GetCollisionGroupId(curFilters);
						if (colFilterInListId > 0)
						{
							m_collisionFilters[i].Set(colFilterInListId);
						}
						else
						{
							Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Invalid collision group name %s in filter for group called %s", curFilters, m_collisionGroups[i].GetCString());
						}
					}
					else // Collision filter is a comma separated list of collision group names
					{
						for (int j = 0; j < s_maxCollisionGroups; ++j)
						{
							if (const char * colFilterInList = StringUtils::ExtractField(curFilters, ",", j))
							{
								int colFilterInListId = GetCollisionGroupId(colFilterInList);
								if (colFilterInListId > 0)
								{
									m_collisionFilters[i].Set(colFilterInListId);
								}
								else
								{
									Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Invalid collision group name %s in filter for group called %s", curFilters, m_collisionGroups[i].GetCString());
								}
							}
						}
					}
				}
			}
		}
	}

	// If the config specifies physics properties, set up the world
	if (GameFile::Object * physConfig = a_config.FindObject("physics"))
	{
		Vector gravity(0.0f, 0.0f, -10.0f);
		if (GameFile::Property * gravProp = physConfig->FindProperty("gravity"))
		{
			gravity = gravProp->GetVector();
		}
		m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);
		m_dynamicsWorld->setGravity(btVector3(gravity.GetX(), gravity.GetY(), gravity.GetZ()));
	}

	return m_dynamicsWorld != NULL || m_collisionWorld != NULL;
}

bool PhysicsManager::Shutdown()
{
	if (m_dynamicsWorld == NULL && m_collisionWorld == NULL)
	{
		return false;
	}

	// Clean up world
	if (m_dynamicsWorld != NULL)
	{
		delete m_dynamicsWorld;
	}
	delete m_collisionWorld;
    delete m_solver;
    delete m_dispatcher;
    delete m_collisionConfiguration;
    delete m_broadphase;

	return true;
}

void PhysicsManager::Update(float a_dt)
{
	if (m_dynamicsWorld == NULL && m_collisionWorld == NULL)
	{
		return;
	}

	if (m_dynamicsWorld != NULL)
	{
		m_dynamicsWorld->stepSimulation(a_dt, 10);
	}
	m_collisionWorld->performDiscreteCollisionDetection();

	// Get all collisions between objects
	int numManifolds = m_collisionWorld->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++)
	{
		btPersistentManifold* contactManifold =  m_collisionWorld->getDispatcher()->getManifoldByIndexInternal(i);
		btCollisionObject* obA = (btCollisionObject*)(contactManifold->getBody0());
		btCollisionObject* obB = (btCollisionObject*)(contactManifold->getBody1());
		GameObject * gameObjA = (GameObject*)obA->getUserPointer();
		GameObject * gameObjB = (GameObject*)obB->getUserPointer();

		int numContacts = contactManifold->getNumContacts();
		if (numContacts > 0 && gameObjA != NULL && gameObjB != NULL)
		{
			if (gameObjA->IsSleeping())
			{
				ClearCollisions(gameObjA);
				continue;
			}

			if (gameObjB->IsSleeping())
			{
				ClearCollisions(gameObjB);
				continue;
			}

			AddCollision(gameObjA, gameObjB);
			AddCollision(gameObjB, gameObjA);
			
			// TODO Add more information to collisions
			for (int j=0;j<numContacts;j++)
			{
				btManifoldPoint& pt = contactManifold->getContactPoint(j);
				if (pt.getDistance()<0.f)
				{
					const btVector3& ptA = pt.getPositionWorldOnA();
					const btVector3& ptB = pt.getPositionWorldOnB();
					const btVector3& normalOnB = pt.m_normalWorldOnB;
				}
			}
		}

		contactManifold->clearManifold();
	}
}

bool PhysicsManager::AddCollisionObject(GameObject * a_gameObj)
{
	if (a_gameObj == NULL)
	{
		return false;
	}

	btCollisionShape * collisionShape = NULL;
	
	switch (a_gameObj->GetClipType())
	{
		case ClipType::Box: 
		{	
			const Vector halfBoxSize = a_gameObj->GetClipSize() * 0.5f;
			btVector3 halfExt(halfBoxSize.GetX(), halfBoxSize.GetY(), halfBoxSize.GetZ());
			collisionShape = new btBoxShape(halfExt);
			break;
		}
		case ClipType::Sphere:
		{
			collisionShape = new btSphereShape(a_gameObj->GetClipSize().GetX());
			break;
		}
		case ClipType::Mesh:
		{
			char bulletFilePath[StringUtils::s_maxCharsPerLine];
			sprintf(bulletFilePath, "%s%s", m_meshPath, a_gameObj->GetPhysicsMeshName());
			if (btBulletWorldImporter * bulletFileLoader = new btBulletWorldImporter(m_dynamicsWorld))
			{
				bulletFileLoader->loadFile(bulletFilePath);
				if (bulletFileLoader->getNumCollisionShapes() > 0)
				{
					collisionShape = bulletFileLoader->getCollisionShapeByIndex(0);
				}
				delete bulletFileLoader;
			}
			break;
		}
		default: return false;
	}

	btCollisionObject * collisionObject = NULL;
	collisionShape->setMargin(0.0f);
	collisionObject = new btCollisionObject();

	// Assign the physics object to the game object
	if (collisionShape != NULL && collisionObject != NULL)
	{
		if (PhysicsObject * newObj = new PhysicsObject())
		{
			btMatrix3x3 basis;
			basis.setIdentity();
			collisionObject->getWorldTransform().setBasis(basis);
			collisionObject->setCollisionShape(collisionShape);
			
			newObj->SetCollisionObject(collisionObject);
			newObj->SetCollisionShape(collisionShape);
			a_gameObj->SetPhysics(newObj);
		}
	}

	// And vice versa
	collisionShape->setUserPointer(a_gameObj);
	collisionObject->setUserPointer(a_gameObj);
	
	// Add to world
	short colGroup = (short)1;
	short colFilter = (short)-1;
	int colGroupId = GetCollisionGroupId(a_gameObj->GetClipGroup());
	if (colGroupId > 0)
	{
		colGroup = (short)(colGroupId);
		colFilter = (short)(m_collisionFilters[colGroupId].GetBits());
	}
	m_collisionWorld->addCollisionObject(collisionObject, colGroup, colFilter);

	return a_gameObj->GetPhysics() != NULL;
}

bool PhysicsManager::AddPhysicsObject(GameObject * a_gameObj)
{
	if (a_gameObj == NULL || a_gameObj->GetPhysics() == NULL || m_dynamicsWorld == NULL)
	{
		return false;
	}

	PhysicsObject * phys = a_gameObj->GetPhysics();
	btCollisionShape * collisionShape = phys->GetCollisionShape();

	// Setup motion state and construction info for game object shape
	btVector3 bodyPos(a_gameObj->GetPos().GetX(), a_gameObj->GetPos().GetY(), a_gameObj->GetPos().GetZ());
	btQuaternion bodyRotation(0, 0, 0, 1);

	// TODO activate physical properties like mass and intertia
	btDefaultMotionState * motionState = new btDefaultMotionState(btTransform(bodyRotation, bodyPos));
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(0, motionState, collisionShape, btVector3(0,0,0));
	btRigidBody * rigidBody = new btRigidBody(rigidBodyCI);
	rigidBody->setUserPointer(a_gameObj);
	phys->SetRigidBody(rigidBody);

	// Add object to physics world
	m_dynamicsWorld->addRigidBody(rigidBody);

	return true;
}

void PhysicsManager::UpdateGameObject(GameObject * a_gameObj)
{
	if (a_gameObj == NULL)
	{
		return;
	}

	PhysicsObject * phys = a_gameObj->GetPhysics();
	const Quaternion gRot = a_gameObj->GetRot();
	const btQuaternion rot(gRot.GetX(), gRot.GetY(), gRot.GetZ(), gRot.GetW());
	const Vector gPos = a_gameObj->GetPos();
	const btVector3 trans(gPos.GetX(), gPos.GetY(), gPos.GetZ());

	btTransform newWorldTrans;
	newWorldTrans.setIdentity();
	newWorldTrans.setRotation(rot);
	newWorldTrans.setOrigin(trans);
	phys->GetCollisionObject()->setWorldTransform(newWorldTrans);

	if (phys->HasRigidBody())
	{
		phys->GetRigidBody()->setWorldTransform(newWorldTrans);
	}
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

bool PhysicsManager::RayCast(const Vector & a_rayStart, const Vector & a_rayEnd, Vector & a_worldHit_OUT, Vector & a_worldNormal_OUT)
{
	// Start and End are vectors
	const btVector3 start(a_rayStart.GetX(), a_rayStart.GetY(), a_rayStart.GetZ());
	const btVector3 end(a_rayEnd.GetX(), a_rayEnd.GetY(), a_rayEnd.GetZ());
	
	// Perform raycast
	btCollisionWorld::ClosestRayResultCallback rayCallback(start, end);
	rayCallback.m_collisionFilterGroup = -1;
	rayCallback.m_collisionFilterMask = -1;
	m_collisionWorld->rayTest(start, end, rayCallback);

	if (rayCallback.hasHit()) 
	{
		a_worldHit_OUT = Vector(rayCallback.m_hitPointWorld.getX(), rayCallback.m_hitPointWorld.getY(), rayCallback.m_hitPointWorld.getZ());
		a_worldNormal_OUT = Vector(rayCallback.m_hitNormalWorld.getX(), rayCallback.m_hitNormalWorld.getY(), rayCallback.m_hitNormalWorld.getZ());
		return true;
	}
	return false;
}

int PhysicsManager::GetCollisionGroupId(StringHash a_colGroupHash)
{
	for (int i = 1; i < s_maxCollisionGroups; ++i)
	{
		if (m_collisionGroups[i] == a_colGroupHash)
		{
			return i;
		}
	}
	return -1;
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

