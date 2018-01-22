#ifndef _ENGINE_VR_MANAGER
#define _ENGINE_VR_MANAGER
#pragma once

#include "Singleton.h"

class VRRender;

struct HWND__;
typedef HWND__* HWND;

//\brief VRManager handles the resources that are shared between the camera and render parts of the VR API integration
class VRManager : public Singleton<VRManager>
{
public:

	VRManager()
	: m_initialised(false)
	, m_hmdWidth(0)
	, m_hmdHeight(0)
	, m_vrRender(nullptr)
	{ }
	
	~VRManager()
	{ }

	void Startup(bool a_useVr); // Calls ovr_Initialize which must happen before render context initilization in StartupRendering 
	void StartupRendering(HWND * a_window, bool a_useVr);
	void Update(float a_dt);
	bool DrawToHMD();
	void Shutdown();

	//\brief Convenience functions for the engine to call Oculus code
	inline bool IsInitialised() const { return m_initialised; }
	inline VRRender * GetVRRender() const { return m_vrRender; }
	inline int GetHmdWidth() const { return m_hmdWidth; }
	inline int GetHmdHeight() const { return m_hmdHeight; }
	
	static const int s_hmdDefaultResolutionWidth = 1920;	// Constant width to override user set resolution for the regular 2D render target size
	static const int s_hmdDefaultResolutionHeight = 1080;	// Constant height to override user set resolution for the regular 2D render target size

private:
	bool m_initialised;
	int m_hmdWidth;
	int m_hmdHeight;
	VRRender * m_vrRender;
};

#endif //_ENGINE_VR_MANAGER
