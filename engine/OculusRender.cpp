#include <stddef.h>

#include "windows.h"

#include <GL/glew.h>
#include <GL/glu.h>

#include "../core/MathUtils.h"

#include "Log.h"

#include "OculusRender.h"

void OculusRender::Startup(ovrHmd a_hmd, HWND a_window)
{
	m_HMD = a_hmd;

	if (m_HMD)
	{
		// Configure Stereo settings. 
		ovrSizei recommenedTex0Size = ovrHmd_GetFovTextureSize(a_hmd, ovrEye_Left, a_hmd->DefaultEyeFov[0], 1.0f); 
		ovrSizei recommenedTex1Size = ovrHmd_GetFovTextureSize(a_hmd, ovrEye_Right, a_hmd->DefaultEyeFov[1], 1.0f); 
		ovrSizei renderTargetSize; 
		renderTargetSize.w = recommenedTex0Size.w + recommenedTex1Size.w; 
		renderTargetSize.h = MathUtils::GetMax(recommenedTex0Size.h, recommenedTex1Size.h);
		const int eyeRenderMultisample = 1; 
		const int backBufferMultisample = 1;
		ovrSizei hmdResolution = a_hmd->Resolution;

		// Configure OpenGL.
		ovrGLConfig cfg; 
		cfg.OGL.Header.API = ovrRenderAPI_OpenGL; 
		cfg.OGL.Header.BackBufferSize = hmdResolution; 
		cfg.OGL.Header.Multisample = backBufferMultisample; 
		cfg.OGL.Window = a_window;
		cfg.OGL.DC = GetDC(a_window);

		unsigned int distortionCaps = ovrDistortionCap_Chromatic | ovrDistortionCap_TimeWarp | ovrDistortionCap_Vignette;
		
		// Create textures to be drawn to for each eye
		for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
		{
			ovrEyeType eye = m_HMD->EyeRenderOrder[eyeIndex];
			m_eyesFov[eye] = m_HMD->DefaultEyeFov[eye];
			m_eyeTexture[eye].OGL.Header.API = ovrRenderAPI_OpenGL;
			m_eyeTexture[eye].OGL.Header.TextureSize = ovrHmd_GetFovTextureSize(a_hmd, eye, a_hmd->DefaultEyeFov[eye], 1.0f);;
			m_eyeTexture[eye].OGL.Header.RenderViewport.Pos.x = 0;
			m_eyeTexture[eye].OGL.Header.RenderViewport.Pos.y = 0;

			if (SetupFrameBuffer(eye, m_eyeTexture[eye].OGL.Header.TextureSize))
			{
				m_eyeTexture[0].OGL.TexId = m_frameBuffer[eye];
			}
			else
			{
				Log::Get().WriteEngineErrorNoParams("Could not create Oculus eye frame buffers.");
				return;
			}
		}

		// Setup optional Rift caps
		int caps = ovrHmd_GetEnabledCaps(a_hmd);
		ovrHmd_SetEnabledCaps(a_hmd, caps | ovrHmdCap_LowPersistence);

		ovrBool result = ovrHmd_ConfigureRendering(a_hmd, &cfg.Config, distortionCaps, m_eyesFov, m_eyeRenderDesc);

		// Create offsets and projections for each eye
		for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
		{
			ovrEyeType eye = m_HMD->EyeRenderOrder[eyeIndex];
			m_eyeOffsets[eye] = m_eyeRenderDesc[eye].HmdToEyeViewOffset;
			m_eyeProjections[eye] = ovrMatrix4f_Projection(m_eyesFov[eye], 0.01f, 1000.0f, true);
		}

		// Direct rendering from a window handle to the Hmd
		ovrHmd_AttachToWindow(a_hmd, a_window, NULL, NULL);
	}
}

bool OculusRender::SetupFrameBuffer(const ovrEyeType & a_eye, const ovrSizei & a_textureSize)
{
	// Generate eye render target
	glGenFramebuffers(1, &m_frameBuffer[a_eye]);
	glGenTextures(1, &m_colourBuffer[a_eye]);
	glGenRenderbuffers(1, &m_depthBuffer[a_eye]);
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer[a_eye]);

	// Colour parameters
	glBindTexture(GL_TEXTURE_2D, m_colourBuffer[a_eye]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, a_textureSize.w, a_textureSize.h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colourBuffer[a_eye], 0);

	// Depth parameters
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer[a_eye]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, a_textureSize.w, a_textureSize.h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer[a_eye]);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void OculusRender::Shutdown()
{

}


void OculusRender::PreRender()
{
	static int frameIndex = 0;
	if (m_HMD)
	{
		++frameIndex;
		ovrHmd_GetEyePoses(m_HMD, frameIndex, m_eyeOffsets, m_eyeRenderPose, nullptr);
		ovrHmd_BeginFrame(m_HMD, frameIndex);
		glEnable(GL_DEPTH_TEST);
	}
}

void OculusRender::RenderFrameBuffer()
{
	if (m_HMD)
	{
		for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
		{
			ovrEyeType eye = m_HMD->EyeRenderOrder[eyeIndex];
			const ovrRecti & vp = m_eyeTexture[eye].OGL.Header.RenderViewport;

			// Bind the appropriate eye buffer and set up a viewport for that eye
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer[eye]);	
			glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
		
			ovrQuatf orientation = ovrQuatf(m_eyeRenderPose[eye].Orientation);
			ovrMatrix4f proj = ovrMatrix4f_Projection(m_eyeRenderDesc[eye].Fov, 0.01f, 10000.0f, true);
			
			// Assign quaternion result directly to view (translation is ignored). 
			//ovrMatrix4f view = ovrMatrix4f(orientation.Inverted()) * ovrMatrix4f::Translation(-WorldEyePos);
			//pRender->SetViewport(EyeRenderViewport[eye]); pRender->SetProjection(proj); 
			//pRoomScene->Render(pRender, ovrMatrix4f::Translation(m_eyeRenderDesc[eye].HmdToEyeViewOffset) * view);
		}
	}
}

void OculusRender::PostRender()
{
	if (m_HMD)
	{
		ovrHmd_EndFrame(m_HMD, m_eyeRenderPose, &m_eyeTexture[0].Texture);
	}
}