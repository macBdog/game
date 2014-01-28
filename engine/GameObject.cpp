#include "../core/Quaternion.h"

#include "AnimationBlender.h"
#include "CollisionUtils.h"
#include "DebugMenu.h"
#include "FontManager.h"
#include "PhysicsManager.h"
#include "RenderManager.h"

#include "GameObject.h"

using namespace std;	//< For fstream operations

bool GameObject::Update(float a_dt)
{
	// Don't update game objects while debugging
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		return true;
	}

	// Tick the object's life
	if (m_state == GameObjectState::Active)
	{
		m_lifeTime += a_dt;
	}

	// Update animation
	if (m_blender != NULL)
	{	
		m_blender->Update(a_dt);
	}

	// Update physics world
	if (m_physics != NULL)
	{
		PhysicsManager::Get().UpdateGameObject(this);
	}

	return true;
}

bool GameObject::Draw()
{
	if (m_state == GameObjectState::Active)
	{
		// Normal mesh rendering
		RenderManager & rMan = RenderManager::Get();
		m_finalMat.SetIdentity();
		Vector finalPos = m_worldMat.GetPos() + m_localMat.GetPos();
		m_finalMat = m_worldMat.Multiply(m_localMat);
		m_finalMat.SetPos(finalPos);
		if (m_model != NULL && m_model->IsLoaded())
		{
			rMan.AddModel(RenderLayer::World, m_model, &m_finalMat, m_shader, m_lifeTime);
		}
		
		// Draw the object's name, position, orientation and clip volume over the top
		if (DebugMenu::Get().IsDebugMenuEnabled() && !DebugMenu::Get().IsDebugMenuActive())
		{
			rMan.AddDebugMatrix(m_worldMat);

			// Draw different debug render shapes accoarding to clip type
			switch (m_clipType)
			{
				case ClipType::AxisBox:
				{
					rMan.AddDebugAxisBox(GetClipPos(), m_clipVolumeSize, sc_colourGrey); 
					break;
				}
				case ClipType::Box:
				{
					Matrix tempMat = m_worldMat;
					tempMat.SetPos(GetClipPos());
					rMan.AddDebugBox(tempMat, m_clipVolumeSize, sc_colourGrey); 
					break;
				}
				case ClipType::Sphere:
				{
					rMan.AddDebugSphere(GetClipPos(), m_clipVolumeSize.GetX(), sc_colourGrey); 
					break;
				}
				
				default: break;
			}

			FontManager::Get().DrawDebugString3D(m_name, 1.0f, GetClipPos());
		}

		return true;
	}

	return false;
}

Vector GameObject::GetRot() const
{
	// TODO? Maybe not even required
	return Vector::Zero();
}

Vector GameObject::GetScale() const
{
	return m_worldMat.GetScale();
}

void GameObject::SetRot(const Vector & a_rot)
{
	const Vector oldPos = m_worldMat.GetPos();
	Quaternion q(Vector(MathUtils::Deg2Rad(a_rot.GetX()), MathUtils::Deg2Rad(a_rot.GetY()), MathUtils::Deg2Rad(a_rot.GetZ())));
	m_worldMat = q.GetRotationMatrix();
	m_worldMat.SetPos(oldPos);
}

void GameObject::AddRot(const Vector & a_newRot)
{
	Quaternion q(MathUtils::Deg2Rad(a_newRot));
	m_worldMat = m_worldMat.Multiply(q.GetRotationMatrix());
}

void GameObject::SetScale(const Vector & a_newScale)
{
	m_worldMat.SetScale(a_newScale);
}

bool GameObject::CollidesWith(Vector a_worldPos)
{ 
	// Clip point against volume
	switch (m_clipType)
	{
		case ClipType::Sphere:
		{
			return CollisionUtils::IntersectPointSphere(a_worldPos + m_clipVolumeOffset, m_worldMat.GetPos(), m_clipVolumeSize.GetX());
		}
		case ClipType::Box:
		case ClipType::AxisBox:
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
	ClipType::Enum colClip = a_colObj->GetClipType();
	if (m_clipType == ClipType::Sphere && colClip == ClipType::Sphere)
	{
		return false; /* TODO: CollisionUtils::IntersectSphereSphere(GetClipPos(), GetClipSize(), a_colObj->GetClipPos(), a_colObj->GetClipSize()); */
	} 
	else if ((m_clipType == ClipType::Sphere && colClip == ClipType::AxisBox) || (m_clipType == ClipType::AxisBox && colClip == ClipType::Sphere)) 
	{
		return false; /* TODO: CollisionUtils::IntersectSphereAxisBox(GetClipPos(), GetClipSize(), a_colObj->GetClipPos(), a_colObj->GetClipSize()); */
	} 
	else if ((m_clipType == ClipType::Sphere && colClip == ClipType::Box) || (m_clipType == ClipType::Box && colClip == ClipType::Sphere)) 
	{
		return false; /* TODO: CollisionUtils::IntersectSphereBox(GetClipPos(), GetClipSize(), a_colObj->GetClipPos(), a_colObj->GetClipSize()); */
	} 
	else if (m_clipType == ClipType::AxisBox && colClip == ClipType::AxisBox) 
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
		case ClipType::Sphere:
		{
			return CollisionUtils::IntersectLineSphere(a_lineStart, a_lineEnd, m_worldMat.GetPos() + m_clipVolumeOffset, m_clipVolumeSize.GetX());
		}
		case ClipType::Box:
		case ClipType::AxisBox:
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

		// TODO: No game objects are being written out yet which is why all the rest
		//			of their optional properties are ommitted here.
		
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
	// Empty collision list and remove from physics simulation
	if (m_physics != NULL)
	{
		PhysicsManager & physMan = PhysicsManager::Get();
		physMan.ClearCollisions(this);
		physMan.RemovePhysicsObject(this);
	}

	// Remove references to managed resources
	if (m_shader != NULL)
	{
		RenderManager::Get().UnManageShader(this);
	}
}
