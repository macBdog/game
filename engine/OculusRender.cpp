#include <stddef.h>

#include "windows.h"

#include "GL/CAPI_GLE.h"
#include "Extras/OVR_Math.h"
#include "OVR_CAPI_GL.h"

#include "../core/MathUtils.h"

#include "CameraManager.h"
#include "Log.h"
#include "RenderManager.h"

#include "OculusRender.h"

using namespace OVR;

DepthBuffer::DepthBuffer(ovrSizei size)
{
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLenum internalFormat = GL_DEPTH_COMPONENT24;
	GLenum type = GL_UNSIGNED_INT;
	if (GLE_ARB_depth_buffer_float)
	{
		internalFormat = GL_DEPTH_COMPONENT32F;
		type = GL_FLOAT;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.w, size.h, 0, GL_DEPTH_COMPONENT, type, NULL);
}

DepthBuffer::~DepthBuffer()
{
	if (texId)
	{
		glDeleteTextures(1, &texId);
		texId = 0;
	}
}

TextureBuffer::TextureBuffer(ovrSession session, bool rendertarget, bool displayableOnHmd, ovrSizei size, int mipLevels, unsigned char * data)
	: Session(session)
	, TextureChain(nullptr)
	, texId(0)
	, fboId(0)
	, texSize()
{
	texSize = size;

	if (displayableOnHmd)
	{
		ovrTextureSwapChainDesc desc = {};
		desc.Type = ovrTexture_2D;
		desc.ArraySize = 1;
		desc.Width = size.w;
		desc.Height = size.h;
		desc.MipLevels = 1;
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.SampleCount = 1;
		desc.StaticImage = ovrFalse;

		ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &TextureChain);

		int length = 0;
		ovr_GetTextureSwapChainLength(session, TextureChain, &length);

		if (OVR_SUCCESS(result))
		{
			for (int i = 0; i < length; ++i)
			{
				GLuint chainTexId;
				ovr_GetTextureSwapChainBufferGL(Session, TextureChain, i, &chainTexId);
				glBindTexture(GL_TEXTURE_2D, chainTexId);

				if (rendertarget)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				}
			}
		}
	}
	else
	{
		glGenTextures(1, &texId);
		glBindTexture(GL_TEXTURE_2D, texId);

		if (rendertarget)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, texSize.w, texSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}

	if (mipLevels > 1)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	glGenFramebuffers(1, &fboId);
}

TextureBuffer::~TextureBuffer()
{
	if (TextureChain)
	{
		ovr_DestroyTextureSwapChain(Session, TextureChain);
		TextureChain = nullptr;
	}
	if (texId)
	{
		glDeleteTextures(1, &texId);
		texId = 0;
	}
	if (fboId)
	{
		glDeleteFramebuffers(1, &fboId);
		fboId = 0;
	}
}

ovrSizei TextureBuffer::GetSize() const
{
	return texSize;
}

void TextureBuffer::SetAndClearRenderSurface(DepthBuffer* dbuffer)
{
	GLuint curTexId;
	if (TextureChain)
	{
		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &curIndex);
		ovr_GetTextureSwapChainBufferGL(Session, TextureChain, curIndex, &curTexId);
	}
	else
	{
		curTexId = texId;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dbuffer->texId, 0);

	glViewport(0, 0, texSize.w, texSize.h);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_FRAMEBUFFER_SRGB);
}

void TextureBuffer::UnsetRenderSurface()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
}

void TextureBuffer::Commit()
{
	if (TextureChain)
	{
		ovr_CommitTextureSwapChain(Session, TextureChain);
	}
}

bool OculusRender::InitRendering(int a_winSizeW, int a_winSizeH, const LUID* a_luid, HWND * a_window)
{
	RECT size = { 0, 0, a_winSizeW, a_winSizeH };
	AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, false);
	const UINT flags = SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW;
	if (!SetWindowPos(*a_window, nullptr, 0, 0, size.right - size.left, size.bottom - size.top, flags))
	{
		return false;
	}

	HDC hDC = wglGetCurrentDC();

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARBFunc = nullptr;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARBFunc = nullptr;
	{
		// First create a context for the purpose of getting access to wglChoosePixelFormatARB / wglCreateContextAttribsARB.
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(pfd));
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 16;
		int pf = ChoosePixelFormat(hDC, &pfd);
		if (!pf)
		{
			Log::Get().WriteEngineErrorNoParams("Failed to choose pixel format.");
		}

		if (!SetPixelFormat(hDC, pf, &pfd))
		{
			Log::Get().WriteEngineErrorNoParams("Failed to set pixel format.");
		}

		HGLRC context = wglCreateContext(hDC);
		if (!context)
		{
			Log::Get().WriteEngineErrorNoParams("wglCreateContextfailed.");
		}

		if (!wglMakeCurrent(hDC, context))
		{
			Log::Get().WriteEngineErrorNoParams("wglMakeCurrent failed.");
		}

		wglChoosePixelFormatARBFunc = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		wglCreateContextAttribsARBFunc = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

		if (!wglChoosePixelFormatARBFunc || !wglCreateContextAttribsARBFunc)
		{
			Log::Get().WriteEngineErrorNoParams("wglChoosePixelFormatARBFunc, wglCreateContextAttribsARBFunc failed.");
		}

		wglDeleteContext(context);
	}

	// Now create the real context that we will be using.
	int iAttributes[] =
	{
		// WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 16,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
		0, 0
	};

	float fAttributes[] = { 0, 0 };
	int   pf = 0;
	UINT  numFormats = 0;

	if (!wglChoosePixelFormatARBFunc(hDC, iAttributes, fAttributes, 1, &pf, &numFormats))
	{
		Log::Get().WriteEngineErrorNoParams("wglChoosePixelFormatARBFunc failed.");
	}

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	if (!SetPixelFormat(hDC, pf, &pfd))
	{
		Log::Get().WriteEngineErrorNoParams("SetPixelFormat failed.");
	}

	GLint attribs[16];
	int   attribCount = 0;
	attribs[attribCount] = 0;

	m_wglContext = wglCreateContextAttribsARBFunc(hDC, 0, attribs);
	if (!wglMakeCurrent(hDC, m_wglContext))
	{
		Log::Get().WriteEngineErrorNoParams("wglMakeCurrent failed.");
	}

	m_GLEContext = new OVR::GLEContext();
	OVR::GLEContext::SetCurrentContext(m_GLEContext);
	m_GLEContext->Init();

	glGenFramebuffers(1, &m_fboId);

	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CW);

	m_winSizeW = a_winSizeW;
	m_winSizeH = a_winSizeH;
	m_renderInit = true;
	return true;
}

void OculusRender::Startup(ovrSession * a_session)
{
	if (a_session != nullptr)
	{
		m_session = a_session;
		ovrSession & session = *m_session;
		ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);

		// Make eye render buffers
		for (int eye = 0; eye < 2; ++eye)
		{
			ovrSizei idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
			m_eyeRenderTexture[eye] = new TextureBuffer(session, true, true, idealTextureSize, 1, NULL);
			m_eyeDepthBuffer[eye] = new DepthBuffer(m_eyeRenderTexture[eye]->GetSize());

			if (!m_eyeRenderTexture[eye]->TextureChain)
			{
				Log::Get().WriteEngineErrorNoParams("Failed to create m_eyeRenderTexture.");
			}
		}

		ovrMirrorTextureDesc desc;
		memset(&desc, 0, sizeof(desc));
		desc.Width = m_winSizeW;
		desc.Height = m_winSizeH;
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

		// Create mirror texture and an FBO used to copy mirror texture to back buffer
		m_mirrorTexture = new ovrMirrorTexture();
		ovrResult result = ovr_CreateMirrorTextureGL(session, &desc, m_mirrorTexture);
		if (!OVR_SUCCESS(result))
		{
			Log::Get().WriteEngineErrorNoParams("Failed to create mirror texture.");
		}

		// Configure the mirror read buffer
		GLuint texId;
		ovr_GetMirrorTextureBufferGL(session, *m_mirrorTexture, &texId);

		glGenFramebuffers(1, &m_mirrorFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_mirrorFBO);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
		glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

		// Turn off vsync to let the compositor do its magic
		wglSwapIntervalEXT(0);

		// FloorLevel will give tracking poses where the floor height is 0
		ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
	}
}

void OculusRender::Shutdown()
{
	// Clean up any created render buffers
	if (m_mirrorFBO)
	{
		glDeleteFramebuffers(1, &m_mirrorFBO);
	}

	if (m_mirrorTexture)
	{
		ovr_DestroyMirrorTexture(*m_session, *m_mirrorTexture);
		delete m_mirrorTexture;
		m_mirrorTexture = nullptr;
	}

	for (int eye = 0; eye < 2; ++eye)
	{
		delete m_eyeRenderTexture[eye];
		delete m_eyeDepthBuffer[eye];
	}

	DeinitRendering();
}

void OculusRender::DeinitRendering()
{
	if (m_fboId)
	{
		glDeleteFramebuffers(1, &m_fboId);
		m_fboId = 0;
	}
	if (m_wglContext)
	{
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(m_wglContext);
		m_wglContext = nullptr;
	}
	m_GLEContext->Shutdown();
	delete m_GLEContext;
	m_renderInit = false;
}

void OculusRender::DrawToHMD()
{
	static int frameIndex = 0;
	++frameIndex;

	ovrSessionStatus sessionStatus;
	ovr_GetSessionStatus(*m_session, &sessionStatus);
	if (sessionStatus.ShouldQuit)
	{
		return;
	}
	if (sessionStatus.ShouldRecenter)
	{
		ovr_RecenterTrackingOrigin(*m_session);
	}

	RenderManager & renMan = RenderManager::Get();
	if (sessionStatus.IsVisible)
	{
		// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
		ovrSession & session = *m_session;
		ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
		
		ovrEyeRenderDesc eyeRenderDesc[2];
		eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
		eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

		// Get eye poses, feeding in correct IPD offset
		ovrPosef EyeRenderPose[2];
		ovrVector3f HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset, eyeRenderDesc[1].HmdToEyeOffset };

		double sensorSampleTime;    // sensorSampleTime is fed into the layer later
		ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyeOffset, EyeRenderPose, &sensorSampleTime);

		// Render Scene to Eye Buffers
		for (int eye = 0; eye < 2; ++eye)
		{
			// Switch to eye render target
			m_eyeRenderTexture[eye]->SetAndClearRenderSurface(m_eyeDepthBuffer[eye]);

			// Get view and projection matrices
			static float Yaw(3.141592f);
			Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
			Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(EyeRenderPose[eye].Orientation);
			Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
			Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
			Vector3f eyePos = rollPitchYaw.Transform(EyeRenderPose[eye].Position);

			// Set the modified matrix to be the product of the game's normal camera plus HMD movement and orientation
			Matrix modifiedView = CameraManager::Get().GetCameraMatrix();
			Vector existingPos = modifiedView.GetPos();
			modifiedView.SetPos(Vector::Zero());
			Quaternion quat = Quaternion(-EyeRenderPose[eye].Orientation.x, EyeRenderPose[eye].Orientation.z, -EyeRenderPose[eye].Orientation.y, EyeRenderPose[eye].Orientation.w);
			quat.ApplyToMatrix(modifiedView);
			modifiedView.SetPos(existingPos + Vector(eyePos.x, eyePos.y, eyePos.z));
			Matrix viewMatrix = modifiedView.GetInverse();
			//viewMatrix.SetPos(existingPos + Vector(eyePos.x, eyePos.y, eyePos.z));

			Matrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_None);
			Matrix perspective(proj.M[0][0], proj.M[1][0], proj.M[2][0], proj.M[3][0],
				proj.M[0][1], proj.M[1][1], proj.M[2][1], proj.M[3][1],
				proj.M[0][2], proj.M[1][2], proj.M[2][2], proj.M[3][2],
				proj.M[0][3], proj.M[1][3], proj.M[2][3], proj.M[3][3]);

			// Render world
			renMan.SetRenderTargetSize(m_winSizeW, m_winSizeH);
			const bool shouldClearRenderBuffers = eye == ovrEye_Count - 1;
			renMan.RenderScene(viewMatrix, perspective, shouldClearRenderBuffers, false);

			// Avoids an error when calling SetAndClearRenderSurface during next iteration.
			// Without this, during the next while loop iteration SetAndClearRenderSurface
			// would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
			// associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
			m_eyeRenderTexture[eye]->UnsetRenderSurface();

			// Commit changes to the textures so they get picked up frame
			m_eyeRenderTexture[eye]->Commit();
		}

		// Do distortion rendering, Present and flush/sync
		ovrLayerEyeFov ld;
		ld.Header.Type = ovrLayerType_EyeFov;
		ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

		for (int eye = 0; eye < 2; ++eye)
		{
			ld.ColorTexture[eye] = m_eyeRenderTexture[eye]->TextureChain;
			ld.Viewport[eye] = OVR::Recti(m_eyeRenderTexture[eye]->GetSize());
			ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
			ld.RenderPose[eye] = EyeRenderPose[eye];
			ld.SensorSampleTime = sensorSampleTime;
		}

		ovrLayerHeader* layers = &ld.Header;
		ovrResult submitResult = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);
		// exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
		if (!OVR_SUCCESS(submitResult))
		{
			Log::Get().WriteEngineErrorNoParams("ovr_SubmitFrame failed!");
		}

		frameIndex++;
	}
	else
	{
		// HMD is not visible, render the scene to clear the buffers
		renMan.RenderScene(Matrix(), Matrix(), true, false);
	}

	// Blit mirror texture to back buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_mirrorFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	GLint w = m_winSizeW;
	GLint h = m_winSizeH;
	glBlitFramebuffer(0, h, w, 0, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	HDC hDC = wglGetCurrentDC();
	SwapBuffers(hDC);
}
