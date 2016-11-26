#ifndef _ENGINE_OCULUS_RENDER
#define _ENGINE_OCULUS_RENDER
#pragma once

#include "OVR_CAPI_GL.h"

namespace OVR
{
	class GLEContext;
}

struct DepthBuffer
{
	DepthBuffer(ovrSizei size);
	~DepthBuffer();
	GLuint texId;	
};

struct TextureBuffer
{
	ovrSession Session;
	ovrTextureSwapChain TextureChain;
	GLuint texId;
	GLuint fboId;
	ovrSizei texSize;

	TextureBuffer(ovrSession session, bool rendertarget, bool displayableOnHmd, ovrSizei size, int mipLevels, unsigned char * data); 		
	~TextureBuffer();
	
	ovrSizei GetSize() const;	
	void SetAndClearRenderSurface(DepthBuffer* dbuffer);
	void UnsetRenderSurface();
	void Commit();
};

//\brief OculusRender handles all the rendering and Gl specific Oculus API integration tasks
class OculusRender
{
public:

	OculusRender() 
		: m_session(nullptr)
		, m_renderInit(false)
		, m_winSizeW(0)
		, m_winSizeH(0)
		, m_fboId(0)
		, m_wglContext()
		, m_GLEContext(nullptr)
		, m_mirrorTexture(nullptr)
		, m_mirrorFBO(0) {};

	bool InitRendering(int a_winSizeW, int a_winSizeH, const LUID* a_luid, HWND * a_window);
	bool InitRenderingNoHMD();
	void Startup(ovrSession * a_session);
	void Update(float a_dt);
	
	//\brief Calls OVR's begin frame and end frame calls with RenderManager::DrawScene sandwiched in between
	bool DrawToHMD();

	void Shutdown();
	void DeinitRendering();

private:

	bool m_renderInit;										///< Has InitRendering been called successfully
	int m_winSizeW;
	int m_winSizeH;
	float m_renderTime;												///< How long the game has been rendering frames for (accumulated frame delta)
	float m_lastRenderTime;											///< How long the last frame took
	GLuint m_fboId;
	HGLRC m_wglContext;
	OVR::GLEContext * m_GLEContext;
	ovrSession * m_session;									///< Pointer to the HMD structure owned by the oculus manager
	TextureBuffer * m_eyeRenderTexture[2];
	DepthBuffer * m_eyeDepthBuffer[2];
	ovrMirrorTexture * m_mirrorTexture;
	GLuint m_mirrorFBO;

	//ovrFovPort m_eyesFov[ovrEye_Count];					///< Fov for each eye render target
	//ovrPosef m_eyeRenderPose[ovrEye_Count];				///< Pose for each eye passed into endFrame
	//ovrTexture m_eyeTexture[ovrEye_Count];				///< Texture for each eye, drawn by the render function
	//ovrEyeRenderDesc m_eyeRenderDesc[ovrEye_Count];		///< Description of the rendering required for each eye
	//ovrVector3f m_eyeOffsets[ovrEye_Count];				///< Where each of the eyes are when rendered
	//ovrMatrix4f m_eyeProjections[ovrEye_Count];			///< Transformation for each eye's projection

	//bool m_frameBufferInitialised[ovrEye_Count];
	//unsigned int m_frameBuffer[ovrEye_Count];			///< Identifier for the whole scene framebuffers for each eye
	//unsigned int m_colourBuffer[ovrEye_Count];			///< Identifier for the texture to render to for each eye
	//unsigned int m_renderBuffer[ovrEye_Count];			///< Identifier for the buffers for pixel depth per eye
};


#endif //_ENGINE_OCULUS_RENDER