#ifndef _ENGINE_OCULUS_CAMERA
#define _ENGINE_OCULUS_CAMERA
#pragma once

#include "CameraManager.h"

#include "OVR.h"

class OculusCamera : public Camera
{
public:

	OculusCamera()
	: m_initialised(false)
	, m_manager(0)
	, m_HMD(0)
	{ 
		for (int i = 0; i < s_numSensors; ++i)
		{
			m_sensors[i] = 0;
			m_fusionResults[i] = NULL;
		}

		Startup(); 
	};
	
	~OculusCamera()
	{
			Shutdown();
	}

	void Startup();
	void Shutdown();
	inline bool IsInitialised() { return m_initialised; }
	inline float GetProjectionCentreOffset() { return m_stereoConfig.GetProjectionCenterOffset(); }
	inline float GetAspect() { return m_stereoConfig.GetAspect(); }
	inline float GetFOV() { return m_stereoConfig.GetYFOVDegrees(); }
	inline float GetIPD() { return m_stereoConfig.GetIPD(); }

	//\brief Update the camera matrix from the inputs
	virtual void Update();

private:

	static const int s_numSensors = 2;

	bool m_initialised;
	OVR::Ptr<OVR::DeviceManager> m_manager;
    OVR::Ptr<OVR::HMDDevice> m_HMD;
    OVR::Ptr<OVR::SensorDevice> m_sensors[s_numSensors];
	OVR::SensorFusion *	m_fusionResults[s_numSensors];
	OVR::Util::Render::StereoConfig m_stereoConfig;
};


#endif //_ENGINE_OCULUS_CAMERA