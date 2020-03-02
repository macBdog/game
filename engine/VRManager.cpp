#include <windows.h>

#include "CameraManager.h"
#include "InputManager.h"
#include "Log.h"
#include "VRRender.h"

#include "VRManager.h"

template<> VRManager * Singleton<VRManager>::s_instance = nullptr;

void VRManager::Startup(bool a_useVr)
{
	if (m_initialised == false)
	{
		// Setup the oculus render, will be used to initialise GL context in both HMD and traditional render state
		m_vrRender = new VRRender();
		if (m_vrRender == nullptr)
		{
			Log::Get().WriteEngineErrorNoParams("Cannot create Oculus render component!");
			return;
		}

		if (a_useVr)
		{
			
		}
	}
}

void VRManager::StartupRendering(HWND * a_window, bool a_useVr)
{
	if (a_useVr)
	{
		if (m_vrRender)
		{
			if (m_vrRender->InitRendering(m_hmdWidth, m_hmdHeight))
			{
				m_vrRender->Startup();
			}
		}
	}
	else
	{
		if (m_vrRender)
		{
			m_vrRender->InitRenderingNoHMD();
		}
	}
}

void VRManager::Update(float a_dt)
{
	if (m_vrRender)
	{
		m_vrRender->Update(a_dt);
	}
}

bool VRManager::DrawToHMD()
{
	if (m_vrRender)
	{
		return m_vrRender->DrawToHMD();
	}

	return false;
}

void VRManager::Shutdown()
{
	if (m_initialised)
	{
		if (m_vrRender)
		{
			m_vrRender->Shutdown();
			delete m_vrRender;
		}
		
		m_initialised = false;
	}
}
