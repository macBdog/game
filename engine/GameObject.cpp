#include "CollisionUtils.h"
#include "DebugMenu.h"
#include "FontManager.h"
#include "PhysicsManager.h"
#include "RenderManager.h"

#include "GameObject.h"

using namespace std;	//< For fstream operations

bool GameObject::Update(float a_dt)
{
	// Tick the object's life
	if (m_state == eGameObjectState_Active)
	{
		m_lifeTime += a_dt;
	}

	// Update physics world
	if (m_physics != NULL)
	{
		PhysicsManager::Get().UpdateGameObject(this);
	}

	// Update components
	for (unsigned int i = 0; i < Component::eComponentTypeCount; ++i)
	{
		Component * curComp = m_components[(unsigned int)i];
		if (curComp != NULL)
		{
			curComp->Update(a_dt);
		}
	}

	return true;
}

bool GameObject::Draw()
{
	if (m_state == eGameObjectState_Active)
	{
		// Normal mesh rendering
		RenderManager & rMan = RenderManager::Get();

		if (m_model != NULL && m_model->IsLoaded())
		{
			rMan.AddModel(RenderManager::eBatchWorld, m_model, &m_worldMat, m_shader);
		}
		
		// Draw the object's name, position, orientation and clip volume over the top
		if (DebugMenu::Get().IsDebugMenuEnabled() && !DebugMenu::Get().IsDebugMenuActive())
		{
			rMan.AddDebugMatrix(m_worldMat);

			// Draw different debug render shapes accoarding to clip type
			switch (m_clipType)
			{
				case eClipTypeAxisBox:
				{
					rMan.AddDebugAxisBox(m_worldMat.GetPos() + m_clipVolumeOffset, m_clipVolumeSize, sc_colourGrey); 
					break;
				}
				case eClipTypeBox:
				{
					//rMan.AddDebugBox(m_worldMat.GetPos() + m_clipVolumeOffset, sc_colourGrey);
					break;
				}
				case eClipTypeSphere:
				{
					rMan.AddDebugSphere(m_worldMat.GetPos() + m_clipVolumeOffset, m_clipVolumeSize.GetX(), sc_colourGrey); 
					break;
				}
				
				default: break;
			}

			FontManager::Get().DrawDebugString3D(m_name, 1.0f, m_worldMat.GetPos());
		}

		return true;
	}

	return false;
}

Vector GameObject::GetRot() const
{
	// TODO
	return Vector::Zero();
}

void GameObject::SetRot(const Vector & a_newRot)
{
	// TODO
	//m_worldMat.SetR
}

bool GameObject::AddCollider(GameObject * a_colObj)
{
	if (a_colObj == NULL)
	{
		return false;
	}

	// Make sure we aren't adding the same collider twice
	Collider * collider = new Collider();
	collider->SetData(a_colObj);
	if (m_colliders.IsEmpty() || m_colliders.Find(a_colObj) == NULL)
	{
		m_colliders.Insert(collider);
		return true;
	}
	else // No insert, cleanup
	{
		delete collider;
		return false;
	}
}

bool GameObject::RemoveCollider(GameObject * a_colObj)
{
	if (m_colliders.IsEmpty() || a_colObj == NULL)
	{
		return false;
	}

	// Iterate through colliders in linked list
	Collider * curColObj = m_colliders.GetHead();
	while (curColObj != NULL)
	{
		// Cache off next pointer
		Collider * next = curColObj->GetNext();

		// Test for the collider we are looking for and remove it
		if (curColObj->GetData()->GetId() == a_colObj->GetId())
		{
			m_colliders.Remove(curColObj);
			delete curColObj;
			return true;
		}

		curColObj = next;
	}
	
	return false;
}

bool GameObject::GetCollisions(CollisionList & a_list_OUT)
{
	// Early out for simple case
	if (!m_clipping || m_colliders.IsEmpty())
	{
		return false;
	}

	// Furnish the list of collisions
	bool addedCollision = false;
	Collider * curColObj = m_colliders.GetHead();
	while (curColObj != NULL)
	{
		// Test for the collider we are looking for and add it to the list
		if (GameObject * collider = curColObj->GetData())
		{
			if (CollidesWith(collider))
			{
				Collider * collision = new Collider();
				collision->SetData(collider);
				a_list_OUT.Insert(collision);
				addedCollision = true;
			}
		}
		curColObj = curColObj->GetNext();
	}
	return addedCollision;
}

bool GameObject::CleanupCollisionList(CollisionList & a_colList_OUT)
{
	if (a_colList_OUT.IsEmpty())
	{
		return false;
	}

	// Iterate through all objects in this file and clean up memory
	Collider * cur = a_colList_OUT.GetHead();
	while(cur != NULL)
	{
		// Cache off next pointer and remove and deallocate memory
		Collider * next = cur->GetNext();
		a_colList_OUT.Remove(cur);
		delete cur;

		cur = next;
	}
	return true;
}

bool GameObject::CollidesWith(Vector a_worldPos)
{ 
	// Clip point against volume
	switch (m_clipType)
	{
		case eClipTypeSphere:
		{
			return CollisionUtils::IntersectPointSphere(a_worldPos + m_clipVolumeOffset, m_worldMat.GetPos(), m_clipVolumeSize.GetX());
		}
		case eClipTypeAxisBox:
		{
			return CollisionUtils::IntersectPointAxisBox(a_worldPos + m_clipVolumeOffset, m_worldMat.GetPos(), m_clipVolumeSize);
		}
		default: return false;
	}
}

bool GameObject::CollidesWith(GameObject * a_colObj)
{ 
	// TODO: This is SUPER branchy code, change clip type to a bit set and re-do

	// Clip each type against type
	eClipType colClip = a_colObj->GetClipType();
	if (m_clipType == eClipTypeSphere && colClip == eClipTypeSphere)
	{
		return false; /* TODO: CollisionUtils::IntersectSphereSphere(GetClipPos(), GetClipSize(), a_colObj->GetClipPos(), a_colObj->GetClipSize()); */
	} 
	else if ((m_clipType == eClipTypeSphere && colClip == eClipTypeAxisBox) || (m_clipType == eClipTypeAxisBox && colClip == eClipTypeSphere)) 
	{
		return false; /* TODO: CollisionUtils::IntersectSphereAxisBox(GetClipPos(), GetClipSize(), a_colObj->GetClipPos(), a_colObj->GetClipSize()); */
	} 
	else if ((m_clipType == eClipTypeSphere && colClip == eClipTypeBox) || (m_clipType == eClipTypeBox && colClip == eClipTypeSphere)) 
	{
		return false; /* TODO: CollisionUtils::IntersectSphereBox(GetClipPos(), GetClipSize(), a_colObj->GetClipPos(), a_colObj->GetClipSize()); */
	} 
	else if (m_clipType == eClipTypeAxisBox && colClip == eClipTypeAxisBox) 
	{
		// Early out if it's impossible to collide
		Vector colCentre = a_colObj->GetClipPos();
		Vector myCentre = m_worldMat.GetPos() + m_clipVolumeOffset;
		if ((colCentre - myCentre).LengthSquared() > m_clipVolumeSize.LengthSquared() + a_colObj->GetClipSize().LengthSquared())
		{
			return false;
		}
		else // TODO: remove this! Need proper AABB test
		{
			return true;
		}
	}
	return false;
}

bool GameObject::CollidesWith(Vector a_lineStart, Vector a_lineEnd)
{ 
	// Clip line against volume
	Vector clipPoint(0.0f);
	switch (m_clipType)
	{
		case eClipTypeSphere:
		{
			return CollisionUtils::IntersectLineSphere(a_lineStart, a_lineEnd, m_worldMat.GetPos() + m_clipVolumeOffset, m_clipVolumeSize.GetX());
		}
		case eClipTypeAxisBox:
		{
			return CollisionUtils::IntersectLineAxisBox(a_lineStart, a_lineEnd, m_worldMat.GetPos() + m_clipVolumeOffset, m_clipVolumeSize, clipPoint);
		}
		default: return false;
	}
}

void GameObject::Serialise(GameFile * outputFile, GameFile::Object * a_parent)
{
	if (a_parent != NULL)
	{
		// Get string versions of numeric values
		char posBuf[StringUtils::s_maxCharsPerName];
		m_worldMat.GetPos().GetString(posBuf);

		// Output all mandatory properties
		GameFile::Object * fileObject = outputFile->AddObject("gameObject", a_parent);
		outputFile->AddProperty(fileObject, "name", m_name);
		outputFile->AddProperty(fileObject, "template", m_template);
		outputFile->AddProperty(fileObject, "pos", posBuf);

		// And optional ones
		if (m_shader != NULL)
		{
			outputFile->AddProperty(fileObject, "shader", m_shader->GetName());
		}
		
		// Serialise any children of this child
		GameObject * child = m_child;
		while (child != NULL)
		{
			child->Serialise(outputFile, fileObject);
			child = child->GetChild();
		}
	}
}

void GameObject::Destroy() 
{
	// Clean up physics
	if (m_physics != NULL)
	{
		PhysicsManager::Get().RemovePhysicsObject(this);
	}

	// Clean up components
	RemoveAllComponents();

	// Remove references to managed resources
	if (m_shader != NULL)
	{
		RenderManager::Get().UnManageShader(this);
		delete m_shader;
	}

	// Empty collision list
	if (!m_colliders.IsEmpty())
	{
		Collider * curColObj = m_colliders.GetHead();
		while (curColObj != NULL)
		{
			// Cache off next pointer
			Collider * next = curColObj->GetNext();
			m_colliders.Remove(curColObj);
			delete curColObj;

			curColObj = next;
		}
	}
}
