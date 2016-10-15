#ifndef _ENGINE_OCULUS_MANAGER
#define _ENGINE_OCULUS_MANAGER
#pragma once

#include "OVR_CAPI_GL.h"

#include "Singleton.h"

class OculusRender;

struct HWND__;
typedef HWND__* HWND;

//\brief OculusManager handles the resources that are shared between the camera and render parts of the Oculus API integration
class OculusManager : public Singleton<OculusManager>
{
public:

	OculusManager()
	: m_initialised(false)
	, m_session(nullptr)
	, m_luid(nullptr)
	, m_hmdWidth(0)
	, m_hmdHeight(0)
	, m_oculusRender(nullptr)
	{ }
	
	~OculusManager()
	{ }

	void Startup(bool a_useVr); // Calls ovr_Initialize which must happen before render context initilization in StartupRendering 
	void StartupRendering(HWND * a_window, bool a_useVr);
	void DrawToHMD();
	void Shutdown();

	//\brief Convenience functions for the engine to call Oculus code
	inline bool IsInitialised() const { return m_initialised; }
	inline OculusRender * GetOculusRender() const { return m_oculusRender; }
	inline int GetHmdWidth() const { return m_hmdWidth; }
	inline int GetHmdHeight() const { return m_hmdHeight; }
	
	static const int s_hmdDefaultResolutionWidth = 1920;	// Constant width to override user set resolution for the regular 2D render target size
	static const int s_hmdDefaultResolutionHeight = 1080;	// Constant height to override user set resolution for the regular 2D render target size

private:
	bool m_initialised;
	ovrSession * m_session;
	ovrGraphicsLuid * m_luid;
	int m_hmdWidth;
	int m_hmdHeight;
	OculusRender * m_oculusRender;
};

#endif //_ENGINE_OCULUS_MANAGER
