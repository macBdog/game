#ifndef _ENGINE_OCULUS_MANAGER
#define _ENGINE_OCULUS_MANAGER
#pragma once

#include "Singleton.h"

class OculusCamera;
class OculusRender;
struct ovrHmdDesc_;
typedef struct ovrHmdDesc_ ovrHmdDesc;
typedef const ovrHmdDesc * ovrHmd; 

//\brief OculusManager handles the resources that are shared between the camera and render parts of the Oculus API integration
class OculusManager : public Singleton<OculusManager>
{
public:

	OculusManager()
	: m_initialised(false)
	, m_oculusCamera(NULL)
	, m_oculusRender(NULL)
	{ }
	
	~OculusManager()
	{ }

	void Startup(); // Calls ovr_Initialize which must happen before render context initilization in StartupRendering 
	void StartupRendering(HWND a_window);
	void DrawToHMD();
	void Shutdown();

	//\brief Convenience functions for the engine to call Oculus code
	inline bool IsInitialised() { return m_initialised; }
	inline OculusCamera * GetOculusCamera() { return m_oculusCamera; }
	inline OculusRender * GetOculusRender() { return m_oculusRender; }
	
private:
	bool m_initialised;
	OculusCamera * m_oculusCamera;
	OculusRender * m_oculusRender;
	ovrHmd m_HMD;					// Actually a pointer to an ovrHmdDesc containing information about the HMD and it's capabilities
};

#endif //_ENGINE_OCULUS_MANAGER
