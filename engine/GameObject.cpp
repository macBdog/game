#include "DebugMenu.h"
#include "FontManager.h"
#include "RenderManager.h"

#include "GameObject.h"

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
		
		// Draw the object's controller over the top
		if (DebugMenu::Get().IsDebugMenuEnabled())
		{
			rMan.AddMatrix(RenderManager::eBatchWorld, m_worldMat);
			FontManager::Get().DrawDebugString3D(m_name, 1.0f, m_worldMat.GetPos());
		}

		return true;
	}

	return false;
}