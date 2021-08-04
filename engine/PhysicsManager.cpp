#include <unordered_map>

#include "CollisionUtils.h"
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
	if (m_gameObject != nullptr)
	{
		m_gameObject = nullptr;
	}
}

bool PhysicsManager::Startup(const GameFile & a_config)
{
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
		if (GameFile::Property * gravProp = physConfig->FindProperty("gravity"))
		{
			SetGravity(gravProp->GetVector());
		}
	}

	return true;
}

bool PhysicsManager::Shutdown()
{
	m_physicsWorld.clear();
	m_collisionWorld.clear();
	return true;
}

void PhysicsManager::UpdateCollisionWorld(const float& a_dt)
{
	// Solve collisions
	for (const auto& objA : m_collisionWorld)
	{
		for (const auto& objB : m_collisionWorld)
		{
			if (objA == objB)
			{
				continue;
			}

			bool colResult = false;
			Vector colPos = Vector::Zero();
			float colDepth = 0.0f;
			Vector colNormal = Vector::Zero();
			switch (objA->GetClipType())
			{
				case ClipType::Sphere:
				{
					if (objB->GetClipType() == ClipType::Sphere)
					{
						if (CollisionUtils::IntersectSpheres(objA->GetPos(), objA->GetClipSize().GetX(), objB->GetPos(), objB->GetClipSize().GetX(), colPos, colNormal))
						{
							colResult = true;
							const auto toCol = colPos - objA->GetPos();
							colDepth = MathUtils::GetMax(objA->GetClipSize().GetX() - toCol.Length(), 0.0f);
						}
					}
					break;
				}
				case ClipType::AxisBox:
				{
					if (objB->GetClipType() == ClipType::Sphere)
					{
						if (CollisionUtils::IntersectAxisBoxSphere(objB->GetPos(), objB->GetClipSize().GetX(), objA->GetPos(), objA->GetClipSize(), colPos, colNormal))
						{
							colResult = true;
							const auto toCol = colPos - objB->GetPos();
							colDepth = MathUtils::GetMax(objB->GetClipSize().GetX() - toCol.Length(), 0.0f);
						}
					}
					break;
				}
				case ClipType::Box:
				{
					if (objB->GetClipType() == ClipType::Sphere)
					{
						if (CollisionUtils::IntersectBoxSphere(objB->GetPos(), objB->GetClipSize().GetX(), objA->GetPos(), objA->GetClipSize(), objA->GetRot(), colPos, colNormal))
						{
							colResult = true;
							const auto toCol = colPos - objB->GetPos();
							colDepth = MathUtils::GetMax(objB->GetClipSize().GetX() - toCol.Length(), 0.0f);
						}
					}
					break;
				}
				default: break;
			}

			if (colResult)
			{
				ClearCollisions(objA);
				AddCollision(objA, objB);

				auto collisionResponse = [&colNormal, &colDepth](auto a_physObj, auto a_gameObj)
				{
					const auto vel = a_physObj->GetVelocity();
					const auto restitution = (1.0f + a_gameObj->GetPhysicsElasticity());
					const auto colDir = vel.Dot(colNormal);
					
					// Only accept the collision if the object is moving towards the collider
					if (colDir < 0)
					{
						// Add penetration depth offset to keep objects from sinking into each other
						const auto pVec = colNormal * colDepth;
						auto incident = (colNormal * colDir * restitution) - (pVec * 1.0f);
						a_physObj->AddLinearImpulse(-incident, a_gameObj->GetPhysicsMass());
						
					}
				};

				if (auto physA = objA->GetPhysics())
				{
					collisionResponse(physA, objA);
				}
				if (auto physB = objB->GetPhysics())
				{
					collisionResponse(physB, objB);
				}
			}
		}
	}
}

void PhysicsManager::UpdatePhysicsWorld(const float& a_dt)
{
	// Step the dynamic physics sim
	const float p_dt = MathUtils::Clamp(s_minPhysicsStep, a_dt, s_maxPhysicsStep);
	for (const auto& curPhys : m_physicsWorld)
	{
		auto physObj = curPhys.get();
		auto gameObj = physObj->m_gameObject;
		const float mass = gameObj->GetPhysicsMass();
		const auto lDrag = gameObj->GetPhysicsLinearDrag();
		const auto aDrag = gameObj->GetPhysicsAngularDrag();
		if (m_type == PhysicsIntegrationType::Euler)
		{
			// Semi-implicit euler
			physObj->AddLinearImpulse(m_gravity * p_dt * Vector::Up(), mass);
			physObj->m_vel += (physObj->m_force * (1.0f / mass)) * p_dt;
			physObj->m_rot *= Quaternion(physObj->m_torque, physObj->m_inertia * p_dt * (aDrag + 1.0f));
			physObj->m_vel *= 1.0f / (1.0f + p_dt * lDrag);
			physObj->m_pos += physObj->m_vel * p_dt;
		}
		else if (m_type == PhysicsIntegrationType::Verlet)
		{
			physObj->m_lastAcc = physObj->m_acc;
			physObj->m_pos += physObj->m_vel * p_dt + (physObj->m_lastAcc * 0.5f * (p_dt * p_dt));
			physObj->m_acc = (physObj->m_force + m_gravity) / mass;
			physObj->m_avgAcc = (physObj->m_lastAcc + physObj->m_acc) * 0.5f;
			physObj->m_vel += physObj->m_avgAcc * p_dt;
		}
		else if (m_type == PhysicsIntegrationType::RungeKutta)
		{
			// TODO!
		}
	}
}

void PhysicsManager::UpdateGameObjects(const float & a_dt)
{
	for (const auto& curPhys : m_physicsWorld)
	{
		// Apply physics world transform to game object and collision state
		auto gameObj = curPhys->m_gameObject;
		Matrix gameObjMat = gameObj->GetWorldMat();
		gameObjMat.SetPos(curPhys->GetPos());
		gameObj->SetWorldMat(gameObjMat);
	}
}

void PhysicsManager::UpdateDebugRender(const float& a_dt)
{
#ifdef _RELEASE
	return;
#endif

	RenderManager& rMan = RenderManager::Get();
	FontManager& fMan = FontManager::Get();
	char pString[StringUtils::s_maxCharsPerName];
	if (DebugMenu::Get().IsPhysicsDebuggingOn())
	{
		const auto drawDebugClipping = [&rMan](const ClipType a_type, const Vector& a_pos, const Quaternion & a_rot, const Vector& a_size, const Colour& a_col)
		{
			Matrix t;
			t.SetPos(a_pos);
			a_rot.ApplyToMatrix(t);
			switch (a_type)
			{
				case ClipType::Box: rMan.AddDebugBox(t, a_size, a_col); break;
				case ClipType::Sphere: rMan.AddDebugSphere(a_pos, a_size.GetX(), a_col); break;				
				case ClipType::AxisBox: rMan.AddDebugAxisBox(a_pos, a_size, a_col); break;
				default: rMan.AddDebugTransform(t);
			}
		};

		std::unordered_map<unsigned int, bool> alreadyDrawn;
		for (const auto& curPhys : m_physicsWorld)
		{
			auto gameObj = curPhys->m_gameObject;
			drawDebugClipping(gameObj->GetClipType(), curPhys->GetPos(), curPhys->GetRot(), gameObj->GetClipSize(), sc_colourPurple);
			curPhys->GetVelocity().GetString(pString);
			fMan.DrawDebugString3D(pString, curPhys->GetPos(), sc_colourBlue);
			alreadyDrawn.insert(std::pair<unsigned int, bool>(gameObj->GetId(), true));
		}

		auto pDebugColour = sc_colourOrange;
		for (const auto& curCol : m_collisionWorld)
		{
			if (alreadyDrawn.find(curCol->GetId()) == alreadyDrawn.end())
			{
				drawDebugClipping(curCol->GetClipType(), curCol->GetPos(), curCol->GetRot(), curCol->GetClipSize(), sc_colourOrange);
			}
		}
	}
}

bool PhysicsManager::AddCollisionObject(GameObject * a_gameObj)
{
	if (a_gameObj == nullptr)
	{
		return false;
	}

	if (a_gameObj->GetClipType() == ClipType::Mesh)
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Game, "Mesh collision is no longer supported for game object %s!", a_gameObj->GetName());
		return false;
	}

	m_collisionWorld.push_back(a_gameObj);
	return m_collisionWorld.back() == a_gameObj;
}

bool PhysicsManager::AddPhysicsObject(GameObject * a_gameObj)
{
	if (a_gameObj == nullptr)
	{
		return false;
	}

	// Set up a new physics object
	if (a_gameObj->GetPhysics() == nullptr)
	{
		m_physicsWorld.push_back(make_unique<PhysicsObject>(a_gameObj));
		a_gameObj->SetPhysics(m_physicsWorld.back());
	}

	// Objects can be re-added at the game object scene position
	const auto& phys = m_physicsWorld.back();
	auto physObj = phys.get();
	physObj->m_pos = a_gameObj->GetPos();
	
	return true;
}

PhysicsObject* PhysicsManager::GetAddPhysicsObject(GameObject* a_gameObj)
{
	if (a_gameObj == nullptr)
	{
		return nullptr;
	}
	if (a_gameObj->GetPhysics() == nullptr)
	{
		AddPhysicsObject(a_gameObj);
	}
	return a_gameObj->GetPhysics();
}

bool PhysicsManager::RemoveCollisionObject(GameObject* a_gameObj)
{
	if (a_gameObj != nullptr)
	{
		ClearCollisions(a_gameObj);
		for (auto it = m_collisionWorld.begin(); it != m_collisionWorld.end();)
		{
			if (*it == a_gameObj)
			{
				it = m_collisionWorld.erase(it);
				return true;
			}
			else
			{
				++it;
			}
		}
	}
	return false;
}

bool PhysicsManager::RemovePhysicsObject(GameObject * a_gameObj)
{
	if (a_gameObj != nullptr)
	{
		auto existingPhys = a_gameObj->GetPhysics();
		for (auto it = m_physicsWorld.begin(); it != m_physicsWorld.end();)
		{
			if (it->get() == existingPhys)
			{
				it = m_physicsWorld.erase(it);
				return true;
			}
			else
			{
				++it;
			}
		}
	}
	return false;
}

bool PhysicsManager::ApplyForce(GameObject * a_gameObj, const Vector & a_force)
{
	if (a_gameObj && a_gameObj->GetPhysics() != nullptr)
	{
		if (PhysicsObject * phys = a_gameObj->GetPhysics())
		{
			phys->AddLinearImpulse(a_force, a_gameObj->GetPhysicsMass());
			return true;
		}
	}
	return false;
}

Vector PhysicsManager::GetVelocity(GameObject * a_gameObj) const
{
	if (a_gameObj && a_gameObj->GetPhysics() != nullptr)
	{
		return a_gameObj->GetPhysics()->GetVelocity();
	}
	return Vector::Zero();
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
