#include "DebugMenu.h"
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
		rMan.AddModel(RenderManager::eBatchWorld, m_model, m_worldMat);
		
		// Draw the object's controller over the top
		if (DebugMenu::Get().IsDebugMenuEnabled())
		{
			//rMan.AddMatrix(m_worldMat);
		}

		return true;
	}

	return false;
}