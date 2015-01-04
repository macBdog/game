#ifndef _ENGINE_OCULUS_CAMERA
#define _ENGINE_OCULUS_CAMERA
#pragma once

#include "CameraManager.h"

struct ovrHmdDesc_;
typedef struct ovrHmdDesc_ ovrHmdDesc;
typedef const ovrHmdDesc * ovrHmd;

class OculusCamera : public Camera
{
public:

	OculusCamera() : m_HMD(NULL) { }

	void Startup(ovrHmd a_hmd);
	void Shutdown();

	inline float GetProjectionCentreOffset() { return 0.0f; }
	inline float GetAspect() { return 0.4333f; }
	inline float GetFOV() { return 100.0f; }
	inline float GetIPD() { return 2.72f; }

	//\brief Update the camera matrix from the inputs
	virtual void Update();

	//\breif OculusManager sets the HMD resources on the camera
	inline void SetHMD(ovrHmd a_hmd) { m_HMD = a_hmd; }

private:
	ovrHmd m_HMD;					// Pointer to the HMD structure owned by the oculus manager
};


#endif //_ENGINE_OCULUS_CAMERA