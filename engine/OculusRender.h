#ifndef _ENGINE_OCULUS_RENDER
#define _ENGINE_OCULUS_RENDER
#pragma once

#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"

//\brief OculusRender handles all the rendering and Gl specific Oculus API integration tasks
class OculusRender
{
public:

	OculusRender() 
		: m_HMD(NULL)
	{
		//TODO memset all members
	}

	void Startup(ovrHmd a_hmd, HWND a_window);
	void Shutdown();

	//\brief Calls OVR's begin frame and end frame calls with RenderManager::DrawScene sandwiched in between
	void DrawToHMD();

	//\breif OculusManager sets the HMD resources on the camera
	inline void SetHMD(ovrHmd a_hmd) { m_HMD = a_hmd; }

private:

	//\brief Helper function to call the GL specific functions to generate render FBOs for each eye
	//\param Eye index into the m_*Buffer members arrays to write into
	//\param Size of the fbo to create
	//\return true if the buffers were setup correctly
	bool SetupFrameBuffer(const ovrEyeType & a_eye, const ovrSizei & a_textureSize);

	ovrHmd m_HMD;										///< Pointer to the HMD structure owned by the oculus manager
	ovrFovPort m_eyesFov[ovrEye_Count];					///< Fov for each eye render target
	ovrPosef m_eyeRenderPose[ovrEye_Count];				///< Pose for each eye passed into endFrame
	ovrTexture m_eyeTexture[ovrEye_Count];				///< Texture for each eye, drawn by the render function
	ovrEyeRenderDesc m_eyeRenderDesc[ovrEye_Count];		///< Description of the rendering required for each eye
	ovrVector3f m_eyeOffsets[ovrEye_Count];				///< Where each of the eyes are when rendered
	ovrMatrix4f m_eyeProjections[ovrEye_Count];			///< Transformation for each eye's projection

	bool m_frameBufferInitialised[ovrEye_Count];
	unsigned int m_frameBuffer[ovrEye_Count];			///< Identifier for the whole scene framebuffers for each eye
	unsigned int m_colourBuffer[ovrEye_Count];			///< Identifier for the texture to render to for each eye
	unsigned int m_renderBuffer[ovrEye_Count];			///< Identifier for the buffers for pixel depth per eye
};


#endif //_ENGINE_OCULUS_RENDER