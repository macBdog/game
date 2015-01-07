#include "OVR_CAPI.h"

#include "CameraManager.h"
#include "InputManager.h"
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
				Log::Get().WriteEngineErrorNoParams("Cannot create Oculus render component!");
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

	if (m_HMD)
	{
		// Health and Safety Warning display state. 
		ovrHSWDisplayState hswDisplayState;
		ovrHmd_GetHSWDisplayState(m_HMD, &hswDisplayState);
		if (hswDisplayState.Displayed) 
		{ 
			// Dismiss the warning if the user pressed the appropriate key or if the user // is tapping the side of the HMD. // If the user has requested to dismiss the warning via keyboard or controller input... 
			if (InputManager::Get().GetLastKey() != SDLK_UNKNOWN)
			{
				ovrHmd_DismissHSWDisplay(m_HMD);
			} 
			else 
			{ 
				// Detect a moderate tap on the side of the HMD. 
				ovrTrackingState ts = ovrHmd_GetTrackingState(m_HMD, ovr_GetTimeInSeconds());
				if (ts.StatusFlags & ovrStatus_OrientationTracked)
				{
					// Arbitrary value and representing moderate tap on the side of the DK2 Rift. 
					Vector hmdAcceleration = Vector(ts.RawSensorData.Accelerometer.x, ts.RawSensorData.Accelerometer.y, ts.RawSensorData.Accelerometer.z);
					if (hmdAcceleration.LengthSquared() > 250.f)
					{
						ovrHmd_DismissHSWDisplay(m_HMD);
					}
				}
			}
		}
	}
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
