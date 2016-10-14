#ifndef _ENGINE_OCULUS_CAMERA
#define _ENGINE_OCULUS_CAMERA
#pragma once

#include "OVR_CAPI_GL.h"

#include "CameraManager.h"

class OculusCamera : public Camera
{
public:

	OculusCamera() { }

	void Startup();
	void Shutdown();

	//\brief Update the camera matrix from the inputs
	virtual void Update(ovrSession * a_session);

private:

};


#endif //_ENGINE_OCULUS_CAMERA