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

	// And collision objects

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
	}

	return true;
}

bool PhysicsManager::Shutdown()
{
	return true;
}

void PhysicsManager::Update(float a_dt)
{

}

bool PhysicsManager::AddCollisionObject(GameObject * a_gameObj)
{
	bool readFromDataPack = m_dataPack != nullptr && m_dataPack->IsLoaded();

	if (a_gameObj == nullptr)
	{
		return false;
	}

	switch (a_gameObj->GetClipType())
	{
		case ClipType::Box: 
		{	
			const Vector halfBoxSize = a_gameObj->GetClipSize() * 0.5f;
			
			break;
		}
		case ClipType::Sphere:
		{
			
			break;
		}
		case ClipType::Mesh:
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Game, "Mesh collision is not supported!");
			break;
		}
		default: return false;
	}
	return false;
}

bool PhysicsManager::AddPhysicsObject(GameObject * a_gameObj)
{
	if (a_gameObj == nullptr || a_gameObj->GetPhysics() == nullptr)
	{
		return false;
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
		
		// Apply physics world transform to game object and collision state
		Matrix gameObjMat = a_gameObj->GetWorldMat();
		gameObjMat.SetIdentity();
		a_gameObj->SetWorldMat(gameObjMat);

		// Set it on all the collision objects
		
	}
	else // Or the other way around if this object is collision only
	{
		Matrix & gMat = a_gameObj->GetWorldMat();
		

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
				
			}
		}
	}
	return vel;
}

bool PhysicsManager::RayCast(const Vector & a_rayStart, const Vector & a_rayEnd, Vector & a_worldHit_OUT, Vector & a_worldNormal_OUT)
{
	
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
