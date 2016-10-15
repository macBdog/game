#include <windows.h>

#include "CameraManager.h"
#include "InputManager.h"
#include "Log.h"
#include "OculusRender.h"

#include "OculusManager.h"

template<> OculusManager * Singleton<OculusManager>::s_instance = NULL;

void OculusManager::Startup(bool a_useVr)
{
	if (m_initialised == false)
	{
		// Setup the oculus render, will be used to initialise GL context in both HMD and traditional render state
		m_oculusRender = new OculusRender();
		if (m_oculusRender == NULL)
		{
			Log::Get().WriteEngineErrorNoParams("Cannot create Oculus render component!");
			return;
		}

		if (a_useVr)
		{
			m_session = new ovrSession();
			m_luid = new ovrGraphicsLuid();
			ovrResult initResult = ovr_Initialize(nullptr);
			ovrResult creatHmdResult = ovr_Create(m_session, m_luid);

			if (creatHmdResult == ovrSuccess)
			{
				ovrHmdDesc hmdDesc = ovr_GetHmdDesc(*m_session);
				ovrSizei windowSize = { hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2 };
				m_hmdWidth = windowSize.w;
				m_hmdHeight = windowSize.h;
			}
			else
			{
				Log::Get().WriteEngineErrorNoParams("Failed to find Oculus HMD device.");
				Shutdown();
				return;
			}
		}
		m_initialised = true;
	}
}

void OculusManager::StartupRendering(HWND * a_window, bool a_useVr)
{
	if (a_useVr)
	{
		if (m_session && m_oculusRender)
		{
			if (m_oculusRender->InitRendering(m_hmdWidth, m_hmdHeight, reinterpret_cast<LUID*>(&m_luid), a_window))
			{
				m_oculusRender->Startup(m_session);
			}
		}
	}
	else
	{
		if (m_oculusRender)
		{
			m_oculusRender->InitRenderingNoHMD();
		}
	}
}

void OculusManager::DrawToHMD()
{
	if (m_oculusRender)
	{
		m_oculusRender->DrawToHMD();
	}
}

void OculusManager::Shutdown()
{
	if (m_initialised)
	{
		if (m_oculusRender)
		{
			m_oculusRender->DeinitRendering();
			m_oculusRender->Shutdown();
			delete m_oculusRender;
		}
		
		if (m_session)
		{
			ovr_Destroy(*m_session);
			ovr_Shutdown();

			delete m_session;
			m_session = nullptr;
		}

		if (m_luid)
		{
			delete m_luid;
			m_luid = nullptr;
		}

		m_initialised = false;
	}
}
