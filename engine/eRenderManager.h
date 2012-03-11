#ifndef _ENGINE_RENDER_MANAGER_
#define _ENGINE_RENDER_MANAGER_
#pragma once

#include "../core/cColour.h"

class eRenderManager
{

public:
	eRenderManager();
	~eRenderManager();

    bool Init(cColour a_clearColour);
    bool Resize(unsigned int a_viewWidth, unsigned int a_viewHeight, unsigned int a_viewBpp, bool a_fullScreen = false);
	void DrawScene();

	inline unsigned int GetViewWidth() { return m_viewWidth; }
	inline unsigned int GetViewHeight() { return m_viewHeight; }

private:
	unsigned int m_viewWidth;
	unsigned int m_viewHeight;
	unsigned int m_bpp;
	cColour m_clearColour;

};

#endif // _ENGINE_RENDER_MANAGER
