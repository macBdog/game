#include "OVR_CAPI.h"

#include "CameraManager.h"
#include "Log.h"
#include "OculusCamera.h"
#include "OculusRender.h"

#include "OculusManager.h"

template<> OculusManager * Singleton<OculusManager>::s_instance = NULL;

void OculusManager::Startup()
{
	if (m_initialised == false)
	{
		ovr_Initialize();

		m_HMD = ovrHmd_Create(0);

		if (m_HMD)
		{
			// Start the sensor which provides the Rift’s pose and motion
			ovrHmd_ConfigureTracking(m_HMD, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);
			
			// Check for an oculus camera with the camera manager
			if (m_oculusCamera = new OculusCamera())
			{
				m_oculusCamera->Startup(m_HMD);
				CameraManager::Get().SetOculusCamera(m_oculusCamera);
			}

			// Setup the oculus render
			m_oculusRender = new OculusRender();
			if (m_oculusRender == NULL)
			{
				//TODO BARF
			}
		}
		else
		{
			Log::Get().WriteEngineErrorNoParams("Failed to find Oculus HMD device.");
			Shutdown();
			return;
		}

		m_initialised = true;
	}
}

void OculusManager::StartupRendering(HWND a_window)
{
	if (m_HMD)
	{
		m_oculusRender->Startup(m_HMD, a_window);
	}
}

void OculusManager::DrawToHMD()
{
	m_oculusRender->DrawToHMD();
}

void OculusManager::Shutdown()
{
	if (m_initialised)
	{
		if (m_oculusCamera)
		{
			m_oculusCamera->SetHMD(NULL);
			delete m_oculusCamera;
		}
		
		if (m_oculusRender)
		{
			m_oculusRender->SetHMD(NULL);
			delete m_oculusRender;
		}
		
		ovrHmd_Destroy(m_HMD);
		ovr_Shutdown();

		m_initialised = false;
	}
}
