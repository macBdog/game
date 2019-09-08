#include <stddef.h>

#include "windows.h"

#include "../core/MathUtils.h"

#include "CameraManager.h"
#include "Log.h"
#include "RenderManager.h"

#include "VRRender.h"
#include "WorldManager.h"

bool VRRender::InitRendering(int a_winSizeW, int a_winSizeH)
{
	return false;
}

bool VRRender::InitRenderingNoHMD()
{
	return false;
}

void VRRender::Startup()
{
}

void VRRender::Shutdown()
{
	DeinitRendering();
}

void VRRender::DeinitRendering()
{
	m_renderInit = false;
}

void VRRender::Update(float a_dt)
{
	m_lastRenderTime = a_dt;
	m_renderTime += a_dt;
}

bool VRRender::DrawToHMD()
{
	static int frameIndex = 0;
	++frameIndex;

	return false;
}
