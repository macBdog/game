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

	//\brief Update the camera matrix from the inputs
	virtual void Update();

	//\breif OculusManager sets the HMD resources on the camera
	inline void SetHMD(ovrHmd a_hmd) { m_HMD = a_hmd; }

	//\brief Overrides base class matrix to supply a modified matrix by the Oculus headset
	const Matrix & GetCameraMatrix() const { return m_modifiedMat; }

private:
	ovrHmd m_HMD;							///< Pointer to the HMD structure owned by the oculus manager
	Matrix m_modifiedMat;                   ///< Matrix composed of the normal world camera plus the Oculus rotation and translation
};


#endif //_ENGINE_OCULUS_CAMERA