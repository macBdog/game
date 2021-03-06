#include <btBulletDynamicsCommon.h>
#include "btBulletWorldImporter.h"

#include "DebugMenu.h"
#include "FontManager.h"
#include "GameObject.h"
#include "Log.h"
#include "RenderManager.h"

#include "PhysicsManager.h"

template<> PhysicsManager * Singleton<PhysicsManager>::s_instance = nullptr;

PhysicsObject::~PhysicsObject()
{
	// Clean up physics if present
	if (!m_rigidBodies.IsEmpty())
	{
		auto curRigidBodyNode = m_rigidBodies.GetHead();
		while (curRigidBodyNode)
		{
			btRigidBody * rigidBody = curRigidBodyNode->GetData();
			if (btDiscreteDynamicsWorld * dynWorld = PhysicsManager::Get().GetDynamicsWorld())
			{
				dynWorld->removeRigidBody(rigidBody);
			}
			delete rigidBody->getMotionState();
			delete rigidBody;
			auto nextNode = curRigidBodyNode->GetNext();
			m_rigidBodies.RemoveDelete(curRigidBodyNode);
			curRigidBodyNode = nextNode;
		}
	}

	// And collision objects
	if (!m_collisionObjects.IsEmpty())
	{
		auto curObjectNode = m_collisionObjects.GetHead();
		while (curObjectNode)
		{
			btCollisionObject * collisionObject = curObjectNode->GetData();
			if (btCollisionWorld * colWorld = PhysicsManager::Get().GetCollisionWorld())
			{
				colWorld->removeCollisionObject(collisionObject);
			}
			delete collisionObject;
			auto nextNode = curObjectNode->GetNext();
			m_collisionObjects.RemoveDelete(curObjectNode);
			curObjectNode = nextNode;
		}
	}

	// File loader owns memory for collision shapes
	if (m_fileLoader != nullptr) 
	{
		m_fileLoader->deleteAllData();
		delete m_fileLoader;

		auto curShapeNode = m_collisionShapes.GetHead();
		while (curShapeNode)
		{
			auto nextNode = curShapeNode->GetNext();
			m_collisionShapes.RemoveDelete(curShapeNode);
			curShapeNode = nextNode;
		}
	}
	else // Otherwise we own it
	{
		if (!m_collisionShapes.IsEmpty())
		{
			auto curShapeNode = m_collisionShapes.GetHead();
			while (curShapeNode)
			{
				btCollisionShape * collisionObject = curShapeNode->GetData();
				delete collisionObject;
				auto nextNode = curShapeNode->GetNext();
				m_collisionShapes.RemoveDelete(curShapeNode);
				curShapeNode = nextNode;
			}
		}
	}
}

bool PhysicsManager::Startup(const GameFile & a_config, const char * a_meshPath, const DataPack * a_dataPack)
{
	// Cache off path to bullet files
	if (a_meshPath != nullptr && a_meshPath[0] != '\0')
	{
		strncpy(m_meshPath, a_meshPath, sizeof(char) * strlen(a_meshPath) + 1);
	}

	if (a_dataPack != nullptr && a_dataPack->IsLoaded())
	{
		// Cache off the datapack path for loading models from pack
		m_dataPack = a_dataPack;
	}

	// Initialise physics world
	btScalar worldSize(10000);
	m_broadphase = new btDbvtBroadphase();
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_solver = new btSequentialImpulseConstraintSolver;
	btVector3 worldAabbMin(-worldSize, -worldSize, -worldSize);
	btVector3 worldAabbMax(worldSize, worldSize, worldSize);

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
					if (strstr(curFilters, ",") == nullptr)
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

		// Set debug draw if correct config
#ifndef _RELEASE
		m_debugRender = new PhysicsDebugRender();
		m_debugRender->setDebugMode(	btIDebugDraw::DBG_DrawWireframe |
										btIDebugDraw::DBG_DrawFeaturesText | 
										btIDebugDraw::DBG_DrawContactPoints);
		m_dynamicsWorld->setDebugDrawer(m_debugRender);
		m_collisionWorld->setDebugDrawer(m_debugRender);
#endif
	}

	return m_dynamicsWorld != nullptr || m_collisionWorld != nullptr;
}

bool PhysicsManager::Shutdown()
{
	if (m_dynamicsWorld == nullptr && m_collisionWorld == nullptr)
	{
		return false;
	}

	// Clean up world
	if (m_dynamicsWorld != nullptr)
	{
		delete m_dynamicsWorld;
	}
	delete m_collisionWorld;
    delete m_solver;
    delete m_dispatcher;
    delete m_collisionConfiguration;
    delete m_broadphase;

#ifndef _RELEASE
	delete m_debugRender;
#endif

	return true;
}

void PhysicsManager::Update(float a_dt)
{
	if (m_dynamicsWorld == nullptr && m_collisionWorld == nullptr)
	{
		return;
	}

	if (m_dynamicsWorld != nullptr)
	{
		m_dynamicsWorld->stepSimulation(a_dt, 10);

#ifndef _RELEASE
		if (DebugMenu::Get().IsPhysicsDebuggingOn())
		{
			m_collisionWorld->debugDrawWorld();
			m_dynamicsWorld->debugDrawWorld();
		}
#endif
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
		if (numContacts > 0 && gameObjA != nullptr && gameObjB != nullptr)
		{
			if (gameObjA->IsSleeping() || !gameObjA->IsClipping())
			{
				ClearCollisions(gameObjA);
				continue;
			}

			if (gameObjB->IsSleeping() || !gameObjB->IsClipping())
			{
				ClearCollisions(gameObjB);
				continue;
			}

			// Manually check the collision filter - bullet should be doing this??
			int collisionGroupA = GetCollisionGroupId(gameObjA->GetClipGroup());
			int collisionGroupB = GetCollisionGroupId(gameObjB->GetClipGroup());
			if (!m_collisionFilters[collisionGroupA].IsBitSet(collisionGroupB) &&
				!m_collisionFilters[collisionGroupB].IsBitSet(collisionGroupA))
			{
				continue;
			}
			AddCollision(gameObjA, gameObjB);
			AddCollision(gameObjB, gameObjA);
			
			// TODO Add more information to collisions
			for (int j=0; j < numContacts; j++)
			{
				btManifoldPoint& pt = contactManifold->getContactPoint(j);
				if (pt.getDistance() < 0.f)
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
	bool readFromDataPack = m_dataPack != nullptr && m_dataPack->IsLoaded();

	if (a_gameObj == nullptr)
	{
		return false;
	}

	// Do not execute this twice on the same object
	PhysicsObject * phys = a_gameObj->GetPhysics();
	if (phys != nullptr && !phys->GetCollisionObjects().IsEmpty())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Game, "Game object %s cannot be added to the collision world twice!", a_gameObj->GetName());
		return false;
	}

	PhysicsObject::CollisionShapeList collisionShapes;
	btBulletWorldImporter * fileLoader = nullptr;
	
	switch (a_gameObj->GetClipType())
	{
		case ClipType::Box: 
		{	
			const Vector halfBoxSize = a_gameObj->GetClipSize() * 0.5f;
			btVector3 halfExt(halfBoxSize.GetX(), halfBoxSize.GetY(), halfBoxSize.GetZ());
			collisionShapes.InsertNew(new btBoxShape(halfExt));
			break;
		}
		case ClipType::Sphere:
		{
			collisionShapes.InsertNew(new btSphereShape(a_gameObj->GetClipSize().GetX()));
			break;
		}
		case ClipType::Mesh:
		{
			char bulletFilePath[StringUtils::s_maxCharsPerLine];
			sprintf(bulletFilePath, "%s%s", m_meshPath, a_gameObj->GetPhysicsMeshName());
			if (fileLoader = new btBulletWorldImporter(m_dynamicsWorld))
			{
				if (readFromDataPack)
				{
					if (DataPackEntry * packedBulletFile = m_dataPack->GetEntry(bulletFilePath))
					{
						// This is crazy, bullet's file loader must read off the end of the buffer, the stack being safe?
						char * bulletReadBuffer = (char *)malloc(packedBulletFile->m_size + 1);
						memcpy(&bulletReadBuffer[0], &packedBulletFile->m_data[0], packedBulletFile->m_size);
						if (fileLoader->loadFileFromMemory(bulletReadBuffer, packedBulletFile->m_size))
						{
							// Add all collision shapes in buffer
							const int numShapes = fileLoader->getNumCollisionShapes();
							for (int i = 0; i < numShapes; ++i)
							{
								collisionShapes.InsertNew(fileLoader->getCollisionShapeByIndex(i));
							}
							free(bulletReadBuffer);
						}
						else
						{
							free(bulletReadBuffer);
							Log::Get().Write(LogLevel::Error, LogCategory::Game, "Error loading bullet collision mesh buffer %s!", bulletFilePath);
							fileLoader->deleteAllData();
							delete fileLoader;
							return false;
						}
					}
				}
				else
				{
					if (fileLoader->loadFile(bulletFilePath))
					{
						// Add all collision shapes in file
						const int numShapes = fileLoader->getNumCollisionShapes();
						for (int i = 0; i < numShapes; ++i)
						{
							collisionShapes.InsertNew(fileLoader->getCollisionShapeByIndex(i));
						}
					}
					else
					{
						Log::Get().Write(LogLevel::Error, LogCategory::Game, "Error loading bullet collision mesh file %s!", bulletFilePath);
						fileLoader->deleteAllData();
						delete fileLoader;
						return false;
					}
				}
			}
			break;
		}
		default: return false;
	}

	PhysicsObject::CollisionObjectList collisionObjects;
	auto curShapeNode = collisionShapes.GetHead();
	while (curShapeNode)
	{
		btCollisionShape * collisionShape = curShapeNode->GetData();
		collisionShape->setMargin(0.05f);

		btCollisionObject * collisionObject = new btCollisionObject();
		collisionObjects.InsertNew(collisionObject);

		// Assign the physics object to the game object
		if (collisionShape != nullptr && collisionObject != nullptr)
		{
			// Only create a new physics object if one has not been created for this game object
			bool newPhys = false;
			if (phys == nullptr)
			{
				newPhys = true;
				phys = new PhysicsObject();
			}
			if (phys != nullptr)
			{
				btMatrix3x3 basis;
				basis.setIdentity();
				collisionObject->getWorldTransform().setBasis(basis);
				collisionObject->setCollisionShape(collisionShape);

				phys->AddCollisionObject(collisionObject);
				phys->AddCollisionShape(collisionShape);

				if (fileLoader != nullptr && phys->GetFileLoader() == nullptr)
				{
					phys->SetFileLoader(fileLoader);
				}
				else
				{
					// Should not be adding multiple file loaders
					if (fileLoader != nullptr && phys->GetFileLoader() != nullptr && fileLoader != phys->GetFileLoader())
					{
						Log::Get().WriteEngineErrorNoParams("Leaking physics file loaders!");
					}
				}

				// Assign if new
				if (newPhys)
				{
					a_gameObj->SetPhysics(phys);
				}
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
		//m_collisionWorld->addCollisionObject(collisionObject, colGroup, colFilter);
		m_collisionWorld->addCollisionObject(collisionObject);

		curShapeNode = curShapeNode->GetNext();
	}

	return a_gameObj->GetPhysics() != nullptr;
}

bool PhysicsManager::AddPhysicsObject(GameObject * a_gameObj)
{
	if (a_gameObj == nullptr || a_gameObj->GetPhysics() == nullptr || m_dynamicsWorld == nullptr)
	{
		return false;
	}

	PhysicsObject * phys = a_gameObj->GetPhysics();
	PhysicsObject::CollisionShapeList collisionShapes = phys->GetCollisionShapes();
	auto curShapeNode = collisionShapes.GetHead();
	while (curShapeNode)
	{
		// Setup motion state and construction info for game object shape
		btCollisionShape * collisionShape = curShapeNode->GetData();
		btVector3 bodyPos(a_gameObj->GetPos().GetX(), a_gameObj->GetPos().GetY(), a_gameObj->GetPos().GetZ());
		btQuaternion bodyRotation(0, 0, 0, 1);

		// Activate physical properties mass and intertia
		btVector3 startingInertia = btVector3(0, 0, 0);
		btDefaultMotionState * motionState = new btDefaultMotionState(btTransform(bodyRotation, bodyPos));

		const float objectMass = a_gameObj->GetPhysicsMass();
		collisionShape->calculateLocalInertia(objectMass, startingInertia);

		btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(objectMass, motionState, collisionShape, startingInertia);
		btRigidBody * rigidBody = new btRigidBody(rigidBodyCI);
		rigidBody->setFriction(10.0f);
		rigidBody->setRollingFriction(0.45f);
		rigidBody->setUserPointer(a_gameObj);
		phys->AddRigidBody(rigidBody);

		// Add object to physics world
		m_dynamicsWorld->addRigidBody(rigidBody);
		curShapeNode = curShapeNode->GetNext();
	}

	return true;
}

void PhysicsManager::UpdateGameObject(GameObject * a_gameObj)
{
	if (a_gameObj == nullptr)
	{
		return;
	}

	// Set the position of the game object and collision from the rigid body physics
	PhysicsObject * phys = a_gameObj->GetPhysics();
	if (phys->HasRigidBody())
	{
		// Get the origin and rotational information from the first rigid body in the physics world
		PhysicsObject::RigidBodyList rigidBodies = phys->GetRigidBodies();
		auto curRigidBodyNode = rigidBodies.GetHead();
		btRigidBody * rigidBody = curRigidBodyNode->GetData();
		btTransform rbTrans = rigidBody->getWorldTransform();
		btQuaternion rbRot = rbTrans.getRotation();
		Quaternion gameObjRot(Vector(rbRot.getAxis().getX(), rbRot.getAxis().getY(), rbRot.getAxis().getZ()), -rbRot.getAngle());

		// Apply physics world transform to game object and collision state
		Matrix gameObjMat = a_gameObj->GetWorldMat();
		gameObjMat.SetIdentity();
		gameObjRot.ApplyToMatrix(gameObjMat);
		gameObjMat.SetPos(Vector(rbTrans.getOrigin().getX(), rbTrans.getOrigin().getY(), rbTrans.getOrigin().getZ()));
		a_gameObj->SetWorldMat(gameObjMat);

		// Set it on all the collision objects
		PhysicsObject::CollisionObjectList collisionObjects = phys->GetCollisionObjects();
		auto curObjectNode = collisionObjects.GetHead();
		while (curObjectNode)
		{
			btCollisionObject * collisionObject = curObjectNode->GetData();
			collisionObject->setWorldTransform(rbTrans);
			curObjectNode = curObjectNode->GetNext();
		}
	}
	else // Or the other way around if this object is collision only
	{
		Matrix & gMat = a_gameObj->GetWorldMat();
		
		// Create a bullet transform that matches the game object's matrix
		btTransform newWorldTrans;
		newWorldTrans.setFromOpenGLMatrix(gMat.GetValues());

		// Set it on all the physics and collision
		PhysicsObject::CollisionObjectList collisionObjects = phys->GetCollisionObjects();
		auto curObjectNode = collisionObjects.GetHead();
		while (curObjectNode)
		{
			btCollisionObject * collisionObject = curObjectNode->GetData();
			collisionObject->setWorldTransform(newWorldTrans);
			curObjectNode = curObjectNode->GetNext();
			m_collisionWorld->updateSingleAabb(collisionObject);
		}

	}
	ClearCollisions(a_gameObj);
}

bool PhysicsManager::RemovePhysicsObject(GameObject * a_gameObj)
{
	if (a_gameObj != nullptr)
	{
		PhysicsObject * phys = a_gameObj->GetPhysics();
		if (phys != nullptr)
		{
			ClearCollisions(a_gameObj);
			delete phys;
			a_gameObj->SetPhysics(nullptr);
			return true;
		}
	}
	return false;
}

bool PhysicsManager::ApplyForce(GameObject * a_gameObj, const Vector & a_force)
{
	if (a_gameObj && a_gameObj->GetPhysicsMass() > 0.0f && a_gameObj->GetPhysics() != nullptr)
	{
		if (PhysicsObject * phys = a_gameObj->GetPhysics())
		{
			if (phys->HasRigidBody())
			{
				PhysicsObject::RigidBodyList rigidBodies = phys->GetRigidBodies();
				auto curRigidBodyNode = rigidBodies.GetHead();
				while (curRigidBodyNode)
				{
					btRigidBody * rigidBody = curRigidBodyNode->GetData();
					rigidBody->activate(true);
					rigidBody->applyCentralImpulse(btVector3(a_force.GetX(), a_force.GetY(), a_force.GetZ()));
					curRigidBodyNode = curRigidBodyNode->GetNext();
				}
			}
			return true;
		}
	}
	return false;
}

Vector PhysicsManager::GetVelocity(GameObject * a_gameObj)
{
	Vector vel(0.0f);
	if (a_gameObj && a_gameObj->GetPhysicsMass() > 0.0f && a_gameObj->GetPhysics() != nullptr)
	{
		if (PhysicsObject * phys = a_gameObj->GetPhysics())
		{
			if (phys->HasRigidBody())
			{
				PhysicsObject::RigidBodyList rigidBodies = phys->GetRigidBodies();
				auto curRigidBodyNode = rigidBodies.GetHead();
				while (curRigidBodyNode)
				{
					btRigidBody * rigidBody = curRigidBodyNode->GetData();
					btVector3 outVel = rigidBody->getLinearVelocity();
					vel.SetX(outVel.getX());
					vel.SetY(outVel.getY());
					vel.SetZ(outVel.getZ());
					curRigidBodyNode = curRigidBodyNode->GetNext();
				}
			}
		}
	}
	return vel;
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
	auto cur = colList->GetHead();
	while(cur != nullptr)
	{
		// Cache off next pointer and remove and deallocate memory
		auto next = cur->GetNext();
		colList->Remove(cur);
		delete cur;

		cur = next;
	}
}

int PhysicsManager::GetNumManifolds()
{ 
	return m_collisionWorld->getDispatcher()->getNumManifolds(); 
}

void PhysicsManager::AddCollision(GameObject * a_gameObjA, GameObject * a_gameObjB)
{
	if (a_gameObjA == nullptr || a_gameObjB == nullptr)
	{
		return;
	}

	GameObject::CollisionList * colList = a_gameObjA->GetCollisions();
	GameObject::Collider * collider = new GameObject::Collider();
	collider->SetData(a_gameObjB);
	colList->Insert(collider);
}

void PhysicsDebugRender::drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor)
{
	RenderManager::Get().AddDebugLine(Vector(from.getX(), from.getY(), from.getZ()), Vector(to.getX(), to.getY(), to.getZ()), Colour(fromColor.getX(), fromColor.getY(), fromColor.getZ(), 1.0f));
}

void PhysicsDebugRender::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	RenderManager::Get().AddDebugLine(Vector(from.getX(), from.getY(), from.getZ()), Vector(to.getX(), to.getY(), to.getZ()), Colour(color.getX(), color.getY(), color.getZ(), 1.0f));
}

void PhysicsDebugRender::drawSphere(const btVector3& p, btScalar radius, const btVector3& color)
{
	RenderManager::Get().AddDebugSphere(Vector(p.getX(), p.getY(), p.getZ()), radius, Colour(color.getX(), color.getY(), color.getZ(), 1.0f));
}

void PhysicsDebugRender::drawTriangle(const btVector3& a, const btVector3& b, const btVector3& c, const btVector3& color, btScalar alpha)
{
	RenderManager::Get().AddDebugLine(Vector(a.getX(), a.getY(), a.getZ()), Vector(b.getX(), b.getY(), b.getZ()), Colour(color.getX(), color.getY(), color.getZ(), alpha));
	RenderManager::Get().AddDebugLine(Vector(b.getX(), b.getY(), b.getZ()), Vector(c.getX(), c.getY(), c.getZ()), Colour(color.getX(), color.getY(), color.getZ(), alpha));
	RenderManager::Get().AddDebugLine(Vector(c.getX(), c.getY(), c.getZ()), Vector(a.getX(), a.getY(), a.getZ()), Colour(color.getX(), color.getY(), color.getZ(), alpha));
}

void PhysicsDebugRender::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
	RenderManager::Get().AddDebugSphere(Vector(PointOnB.getX(), PointOnB.getY(), PointOnB.getZ()), 0.1f, Colour(color.getX(), color.getY(), color.getZ(), 1.0f));
}

void PhysicsDebugRender::reportErrorWarning(const char* warningString)
{
	Log::Get().WriteEngineErrorNoParams(warningString);
}

void PhysicsDebugRender::draw3dText(const btVector3& location, const char* textString)
{
	FontManager::Get().DrawDebugString3D(textString, Vector(location.getX(), location.getY(), location.getZ()));
}
