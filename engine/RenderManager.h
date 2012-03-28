#ifndef _ENGINE_RENDER_MANAGER_
#define _ENGINE_RENDER_MANAGER_
#pragma once

#include "Singleton.h"
#include "Texture.h"

#include "../core/Colour.h"
#include "../core/Vector.h"

class RenderManager : Singleton<RenderManager>
{

public:

	//\brief A Batch is a way to specify rendering order
	enum eBatch
	{
		eBatchNone = 0,		//< It's valid to render in the none batch
		eBatchWorld,		//< But it will be drawn over by the world
		eBatchGui,			//< GUI covers the world
		eBatchDebug,		//< Devbug text over everything

		eBatchCount,
	};
	
	//\ No work done in the constructor, only Init
	RenderManager() : m_clearColour(sc_colourBlack) {}

	//\brief Set clear colour buffer and depth buffer setup 
    bool Init(Colour a_clearColour);

	//\brief Setup the viewport
    bool Resize(unsigned int a_viewWidth, unsigned int a_viewHeight, unsigned int a_viewBpp, bool a_fullScreen = false);

	//\brief Dump everything to the buffer 
	void DrawScene();

	//\brief Accessors for the viewport dimensions
	inline unsigned int GetViewWidth() { return m_viewWidth; }
	inline unsigned int GetViewHeight() { return m_viewHeight; }

	//\brief Drawing functions
	void AddTri(eBatch a_batch, Vector a_p1, Vector a_p2, Vector a_p3, Colour a_colour);
	void AddQuad(eBatch a_batch, Vector a_topLeft, float a_width, float a_height, Texture * a_tex, Texture::eOrientation a_orient = Texture::eOrientationNormal);

private:
	unsigned int m_viewWidth;
	unsigned int m_viewHeight;
	unsigned int m_bpp;
	Colour m_clearColour;

};

#endif // _ENGINE_RENDER_MANAGER
