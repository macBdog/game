#include "CollisionUtils.h"
#include "DebugMenu.h"
#include "FontManager.h"
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
			rMan.AddModel(RenderManager::eBatchWorld, m_model, &m_worldMat);
		}
		
		// Draw the object's name, position, orientation and clip volume over the top
		if (DebugMenu::Get().IsDebugMenuEnabled())
		{
			rMan.AddDebugMatrix(m_worldMat);

			// Draw different debug render shapes accoarding to clip type
			switch (m_clipType)
			{
				case eClipTypeSphere:
				{
					rMan.AddDebugSphere(m_worldMat.GetPos(), m_clipVolumeSize.GetX(), sc_colourPink); 
					break;
				}
				case eClipTypeAABB:
				{
					rMan.AddDebugAABB(m_worldMat.GetPos(), m_clipVolumeSize, sc_colourPink); 
				}
				default: break;
			}

			FontManager::Get().DrawDebugString3D(m_name, 1.0f, m_worldMat.GetPos());
		}

		return true;
	}

	return false;
}

bool GameObject::CollidesWith(Vector a_worldPos)
{ 
	// Clip point against volume
	switch (m_clipType)
	{
		case eClipTypeSphere:
		{
			return CollisionUtils::IntersectPointSphere(a_worldPos, m_worldMat.GetPos(), m_clipVolumeSize.GetX());
		}
		case eClipTypeAABB:
		{
			return CollisionUtils::IntersectPointAABB(a_worldPos, m_worldMat.GetPos(), m_clipVolumeSize);
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

		// Output all properties
		GameFile::Object * fileObject = outputFile->AddObject("gameObject", a_parent);
		outputFile->AddProperty(fileObject, "name", m_name);
		outputFile->AddProperty(fileObject, "template", m_template);
		outputFile->AddProperty(fileObject, "pos", posBuf);

		// Serialise any children of this child
		GameObject * child = m_child;
		while (child != NULL)
		{
			child->Serialise(outputFile, fileObject);
			child = child->GetChild();
		}
	}
}
