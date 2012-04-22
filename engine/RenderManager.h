#ifndef _ENGINE_RENDER_MANAGER_
#define _ENGINE_RENDER_MANAGER_
#pragma once

#include "Singleton.h"
#include "Texture.h"

#include "../core/Colour.h"
#include "../core/Vector.h"

class RenderManager : public Singleton<RenderManager>
{
public:

	//\brief A Batch is a way to specify rendering order
	enum eBatch
	{
		eBatchNone = 0,		//< It's valid to render in the none batch
		eBatchWorld,		//< But it will be drawn over by the world
		eBatchGui,			//< GUI covers the world
		eBatchDebug,		//< Debug text over everything

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
	void AddQuad2D(eBatch a_batch, Vector a_topLeft, float a_width, float a_height, Texture * a_tex, Texture::eOrientation a_orient = Texture::eOrientationNormal);

private:

	//\brief Fixed size structure for queing render primitives
	struct Quad
	{
		Vector m_verts[4];
		TexCoord m_coords[4];
		unsigned int m_textureId;
	};

	Quad * m_batch[eBatchCount];							// Pointer to a pool of memory for quads
	unsigned int m_batchCount[eBatchCount];					// Number of primitives in each batch per frame
	unsigned int m_viewWidth;								// Cache of arguments passed to init
	unsigned int m_viewHeight;								// Cache of arguments passed to init
	unsigned int m_bpp;										// Cache of arguments passed to init
	Colour m_clearColour;									// Cache of arguments passed to init

	static const unsigned int s_maxPrimitivesPerBatch = 64 * 1000;
};

#endif // _ENGINE_RENDER_MANAGER
