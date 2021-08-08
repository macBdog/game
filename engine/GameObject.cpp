#include "../core/Quaternion.h"

#include "AnimationBlender.h"
#include "CollisionUtils.h"
#include "DebugMenu.h"
#include "FontManager.h"
#include "ModelManager.h"
#include "PhysicsManager.h"
#include "RenderManager.h"
#include "WorldManager.h"

#include "GameObject.h"

using namespace std;	//< For fstream operations

// String literals for the clip types
const char * GameObject::s_clipTypeStrings[static_cast<int>(ClipType::Count)] = 
{
	"none",
	"axisbox",
	"sphere",
	"box",
};

void GameObject::SetTemplateProperties()
{
	char fullTemplatePath[StringUtils::s_maxCharsPerLine];
	sprintf(fullTemplatePath, "%s%s", WorldManager::Get().GetTemplatePath(), m_template);
	GameFile templateFile;
	if (templateFile.Load(fullTemplatePath) && templateFile.IsLoaded())
	{
		if (GameFile::Object * object = templateFile.FindObject("gameObject"))
		{
			if (GameFile::Property * model = object->FindProperty("model"))
			{
				if (Model * newModel = ModelManager::Get().GetModel(model->GetString()))
				{
					SetModel(newModel);
				}
			}
			if (GameFile::Property * clipType = object->FindProperty("clipType"))
			{
				if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[static_cast<int>(ClipType::Sphere)]) != nullptr)
				{
					SetClipType(ClipType::Sphere);
				}
				else if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[static_cast<int>(ClipType::AxisBox)]) != nullptr)
				{
					SetClipType(ClipType::AxisBox);
				}
				else if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[static_cast<int>(ClipType::Box)]) != nullptr)
				{
					SetClipType(ClipType::Box);
				}
				else if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[static_cast<int>(ClipType::Mesh)]) != nullptr)
				{
					Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Mesh clip type no longer supported for object %s, defaulting to axisbox.", GetName());
					SetClipType(ClipType::AxisBox);
				}
				else
				{
					Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Unknown clip type of %s specified for object %s, defaulting to axisbox.", clipType->GetString(), GetName());
					SetClipType(ClipType::AxisBox);
				}
			}
			if (GameFile::Property * clipSize = object->FindProperty("clipSize"))
			{
				SetClipSize(clipSize->GetVector());
			}
			if (GameFile::Property * clipOffset = object->FindProperty("clipOffset"))
			{
				SetClipOffset(clipOffset->GetVector());
			}
			if (GameFile::Property* clipGroup = object->FindProperty("clipGroup"))
			{
				SetClipGroup(clipGroup->GetString(), PhysicsManager::Get().GetCollisionGroupId(clipGroup->GetString()));
			}
			// Shader 
			RenderManager & rMan = RenderManager::Get();
			if (GameFile::Property * shader = object->FindProperty("shader"))
			{
				// First try to find if the shader is already loaded
				rMan.ManageShader(this, shader->GetString());
			}
			// Physics
			if (GameFile::Property* massProp = object->FindProperty("physicsMass"))
			{			
				SetPhysicsMass(massProp->GetFloat());
			}
			if (GameFile::Property* elasticProp = object->FindProperty("physicsElasticity"))
			{
				SetPhysicsElasticity(elasticProp->GetFloat());
			}
			if (GameFile::Property* linearDragProp = object->FindProperty("physicsLinearDrag"))
			{
				SetPhysicsLinearDrag(linearDragProp->GetFloat());
			}
			if (GameFile::Property* angularDragProp = object->FindProperty("physicsAngularDrag"))
			{
				SetPhysicsAngularDrag(angularDragProp->GetFloat());
			}
		}
	}
}

bool GameObject::Update(float a_dt)
{
#ifndef _RELEASE
	// Don't update game objects while debugging
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		return true;
	}

	// Don't update scripts if time paused
	if (DebugMenu::Get().IsTimePaused())
	{
		return true;
	}

	// Monitor and reload from template
	if (HasTemplate())
	{
		FileManager::Timestamp tempTime;
		char fullTemplatePath[StringUtils::s_maxCharsPerLine];
		sprintf(fullTemplatePath, "%s%s", WorldManager::Get().GetTemplatePath(), m_template);
		FileManager::Get().GetFileTimeStamp(fullTemplatePath, tempTime);
		if (tempTime > m_templateTimeStamp)
		{
			Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in template file %s, reloading.", fullTemplatePath);
			SetTemplateProperties();
			m_templateTimeStamp = tempTime;
		}
	}

#endif
	// Early out for deactivated objects
	if (m_state == GameObjectState::Sleep)
	{
		return true;
	}

	if (m_state == GameObjectState::Death)
	{
		return false;
	}

	// Become active if ready
	if (m_state == GameObjectState::Loading && m_model != nullptr)
	{
		m_state = GameObjectState::Active;
	}

	// Tick the object's life
	m_lifeTime += a_dt;
	
	// Update animation
	if (m_blender != nullptr)
	{	
		m_blender->Update(a_dt);
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
		if (m_visible && m_model != nullptr && m_model->IsLoaded())
		{
			rMan.AddModel(RenderLayer::World, m_model, &m_finalMat, m_shader, m_shaderData, m_lifeTime);
		}
		
		// Draw the object's name, position, orientation and clip volume over the top
		if (DebugMenu::Get().IsDebugMenuEnabled() && !DebugMenu::Get().IsDebugMenuActive())
		{
			Matrix debugMat = m_worldMat;
			debugMat.SetPos(GetClipPos());
			rMan.AddDebugTransform(debugMat);

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
				case ClipType::Mesh:
				{
					// TODO: proper collision mesh drawing
					rMan.AddDebugSphere(GetClipPos(), 1.0f, sc_colourGreen); 
					break;
				}
				
				default: break;
			}

			FontManager::Get().DrawDebugString3D(m_name, GetClipPos());
		}

		return true;
	}

	return false;
}

bool GameObject::Shutdown() 
{
	// Empty collision list and remove from physics simulation
	if (m_physics != nullptr)
	{
		PhysicsManager & physMan = PhysicsManager::Get();
		physMan.RemovePhysicsObject(this);
		physMan.RemoveCollisionObject(this);
		m_physics = nullptr;
	}

	// Remove references to managed resources
	if (m_shader != nullptr)
	{
		RenderManager::Get().UnManageShader(this);
	}

	SetState(GameObjectState::Death);

	return true;
}

Quaternion GameObject::GetRot() const
{
	return Quaternion(m_worldMat);
}

Vector GameObject::GetScale() const
{
	return m_worldMat.GetScale();
}

void GameObject::SetTemplate(const char * a_templateName) 
{ 
	strncpy(m_template, a_templateName, StringUtils::s_maxCharsPerName);
	// Find the timestamp for the template file monitoring
#ifndef _RELEASE
	char fullTemplatePath[StringUtils::s_maxCharsPerLine];
	sprintf(fullTemplatePath, "%s%s", WorldManager::Get().GetTemplatePath(), a_templateName);
	FileManager::Get().GetFileTimeStamp(fullTemplatePath, m_templateTimeStamp);
#endif
}

void GameObject::SetRot(const Vector & a_rot)
{
	const Vector oldPos = m_worldMat.GetPos();
	Quaternion q(MathUtils::Deg2Rad(a_rot));
	m_worldMat = q.GetRotationMatrix();
	m_worldMat.SetPos(oldPos);
}

void GameObject::SetRot(const Quaternion & a_rot)
{
	const Vector oldPos = m_worldMat.GetPos();
	m_worldMat = m_worldMat.Multiply(a_rot.GetRotationMatrix());
	m_worldMat.SetPos(oldPos);
}

void GameObject::AddRot(const Vector & a_newRot)
{
	Quaternion q(MathUtils::Deg2Rad(a_newRot));
	const Vector oldPos = m_worldMat.GetPos();
	m_worldMat = m_worldMat.Multiply(q.GetRotationMatrix());
	m_worldMat.SetPos(oldPos);
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
	// Clip each type against type
	ClipType colClip = a_colObj->GetClipType();
	Vector colPos = { 0.0f }, colNorm = { 0.0f };
	if (m_clipType == ClipType::Sphere && colClip == ClipType::Sphere)
	{
		return CollisionUtils::IntersectSpheres(GetClipPos(), GetClipSize().GetX(), a_colObj->GetClipPos(), a_colObj->GetClipSize().GetX(), colPos, colNorm);
	} 
	else if ((m_clipType == ClipType::Sphere && colClip == ClipType::AxisBox) || (m_clipType == ClipType::AxisBox && colClip == ClipType::Sphere)) 
	{
		return CollisionUtils::IntersectAxisBoxSphere(GetClipPos(), GetClipSize().GetX(), a_colObj->GetClipPos(), a_colObj->GetClipSize(), colPos, colNorm);
	} 
	else if ((m_clipType == ClipType::Sphere && colClip == ClipType::Box) || (m_clipType == ClipType::Box && colClip == ClipType::Sphere)) 
	{
		return CollisionUtils::IntersectBoxSphere(GetClipPos(), GetClipSize().GetX(), a_colObj->GetClipPos(), a_colObj->GetClipSize(), a_colObj->GetRot(), colPos, colNorm);
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
	if (a_parent != nullptr)
	{
		// Get string versions of numeric values
		char vecBuf[StringUtils::s_maxCharsPerName];
		
		// Output all mandatory properties
		GameFile::Object * fileObject = outputFile->AddObject("gameObject", a_parent);
		outputFile->AddProperty(fileObject, "name", m_name);

		// Save template if set
		bool templated = m_template[0] != '\0';
		if (templated)
		{
			outputFile->AddProperty(fileObject, "template", m_template);
			SerialiseTemplate();
		}
		
		GetPos().GetString(vecBuf);
		outputFile->AddProperty(fileObject, "pos", vecBuf);

		GetRot().GetString(vecBuf);
		outputFile->AddProperty(fileObject, "rot", vecBuf);

		// And optional ones
		if (!templated)
		{
			if (m_clipType != ClipType::None)
			{
				// Primitive type clipping
				if (m_clipType != ClipType::Mesh) 
				{
					outputFile->AddProperty(fileObject, "clipType", s_clipTypeStrings[static_cast<int>(m_clipType)]);
					char vecBuf[StringUtils::s_maxCharsPerName];
					m_clipVolumeSize.GetString(vecBuf);
					outputFile->AddProperty(fileObject, "clipSize", vecBuf);
					m_clipVolumeOffset.GetString(vecBuf);
					outputFile->AddProperty(fileObject, "clipOffset", vecBuf);
				}
			}
			if (!m_clipGroup.IsEmpty())
			{
				outputFile->AddProperty(fileObject, "clipGroup", m_clipGroup.GetCString());
			}
			if (m_physicsMass > 0.0f)
			{
				outputFile->AddProperty(fileObject, "physicsMass", m_physicsMass);
				outputFile->AddProperty(fileObject, "physicsElasticity", m_physicsElasticity);
				outputFile->AddProperty(fileObject, "physicsLinearDrag", m_physicsLinearDrag);
				outputFile->AddProperty(fileObject, "physicsAngularDrag", m_physicsAngularDrag);
			}
			if (m_shader != nullptr)
			{
				// Make sure the shader is not a default engine shader
				if (m_shader != RenderManager::Get().GetLightingShader() &&
					m_shader != RenderManager::Get().GetColourShader())
				{
					outputFile->AddProperty(fileObject, "shader", m_shader->GetName());
				}
			}
			if (m_model != nullptr)
			{
				outputFile->AddProperty(fileObject, "model", m_model->GetName());
			}
		}
		
		// Serialise any children of this child
		GameObject * child = m_child;
		while (child != nullptr)
		{
			child->Serialise(outputFile, fileObject);
			child = child->GetChild();
		}
	}
}

void GameObject::SerialiseTemplate()
{
	GameFile * templateFile = new GameFile();
	GameFile::Object * templateObj = templateFile->AddObject("gameObject");
	if (m_model != nullptr)
	{
		templateFile->AddProperty(templateObj, "model", m_model->GetName());
	}
	if (m_clipType != ClipType::None)
	{
		if (m_clipType != ClipType::Mesh)
		{
			templateFile->AddProperty(templateObj, "clipType", s_clipTypeStrings[static_cast<int>(m_clipType)]);
			char vecBuf[StringUtils::s_maxCharsPerName];
			m_clipVolumeSize.GetString(vecBuf);
			templateFile->AddProperty(templateObj, "clipSize", vecBuf);
			m_clipVolumeOffset.GetString(vecBuf);
			templateFile->AddProperty(templateObj, "clipOffset", vecBuf);
		}
	}
	if (!m_clipGroup.IsEmpty())
	{
		templateFile->AddProperty(templateObj, "clipGroup", m_clipGroup.GetCString());
	}

	if (m_physicsMass > 0.0f)
	{
		templateFile->AddProperty(templateObj, "physicsMass", m_physicsMass);
		templateFile->AddProperty(templateObj, "physicsElasticity", m_physicsElasticity);
		templateFile->AddProperty(templateObj, "physicsLinearDrag", m_physicsLinearDrag);
		templateFile->AddProperty(templateObj, "physicsAngularDrag", m_physicsAngularDrag);
	}

	if (m_shader != nullptr)
	{
		if (m_shader != RenderManager::Get().GetLightingShader() &&
			m_shader != RenderManager::Get().GetColourShader())
		{
			templateFile->AddProperty(templateObj, "shader", m_shader->GetName());
		}
	}

	// Check the template exists, if not create it
	char templatePath[StringUtils::s_maxCharsPerLine];
	sprintf(templatePath, "%s%s", WorldManager::Get().GetTemplatePath(), m_template);
	templateFile->Write(templatePath);
}

