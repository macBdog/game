#ifndef _ENGINE_RENDER_MANAGER_
#define _ENGINE_RENDER_MANAGER_
#pragma once

#include "Singleton.h"
#include "Texture.h"

#include "../core/Colour.h"
#include "../core/Vector.h"

//\brief RenderManager separates rendering from the rest of the engine by wrapping all 
//		 calls to OpenGL with some abstract concepts like rendering quads, primitives and meshes
class RenderManager : public Singleton<RenderManager>
{
public:

	//\brief Render modes are used to change how the scene is rendered wholesale
	enum eRenderMode
	{
		eRenderModeNone = 0,
		eRenderModeWireframe,
		eRenderModeFull,

		eRenderModeCount,
	};

	//\brief A Batch is a way to specify rendering order
	// CH:TODO Rename from batch, it's incorrect terminology
	enum eBatch
	{
		eBatchNone = 0,		//< It's valid to render in the none batch
		eBatchWorld,		//< But it will be drawn over by the world
		eBatchGui,			//< GUI covers the world
		eBatchDebug,		//< Debug text over everything

		eBatchCount,
	};
	
	//\ No work done in the constructor, only Init
	RenderManager() : m_lines(NULL)
					, m_clearColour(sc_colourBlack)
					, m_renderMode(eRenderModeFull)
					, m_aspect(1.0f) {}
	~RenderManager() { Shutdown(); }

	//\brief Set clear colour buffer and depth buffer setup 
    bool Startup(Colour a_clearColour);
	bool Shutdown();

	//\brief Setup the viewport
    bool Resize(unsigned int a_viewWidth, unsigned int a_viewHeight, unsigned int a_viewBpp, bool a_fullScreen = false);

	//\brief Dump everything to the buffer 
	void DrawScene();

	//\brief Change the render mode
	//\param a_renderMode the new mode to set
	inline void SetRenderMode(eRenderMode a_renderMode) { m_renderMode = a_renderMode; }

	//\brief Accessors for the viewport dimensions
	inline unsigned int GetViewWidth() { return m_viewWidth; }
	inline unsigned int GetViewHeight() { return m_viewHeight; }
	inline unsigned int GetViewDepth() { return m_bpp; }
	inline float GetViewAspect() { return m_aspect; }

	//\brief Drawing functions
	void AddLine2D(Vector2 a_point1, Vector2 a_point2, Colour a_tint = sc_colourWhite);
	void AddQuad2D(eBatch a_batch, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, Texture::eOrientation a_orient = Texture::eOrientationNormal, Colour a_tint = sc_colourWhite);

	//\brief Quad drawing function with manual texture coordinates
	//\param texCoord is the top left of the texture coordinate box, with 0,0 being top left
	//\param texSize is the bounds of the texture coordinate box with 1,1 being the bottom right
	void AddQuad2D(eBatch a_batch, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, TexCoord texCoord, TexCoord texSize, Texture::eOrientation a_orient = Texture::eOrientationNormal, Colour a_tint = sc_colourWhite);

private:

	static const float s_renderDepth2D;			// Z value for ortho rendered primitives

	//\brief Fixed size structure for queing line primitives
	struct Line
	{
		Vector m_verts[2];
		Colour m_colour;
	};

	//\brief Fixed size structure for queing render primitives
	struct Quad
	{
		Vector m_verts[4];
		TexCoord m_coords[4];
		int m_textureId;
		Colour m_colour;
	};

	Quad * m_batch[eBatchCount];							// Pointer to a pool of memory for quads
	Line * m_lines;											// Lines are a debug feature so they aren't divided into batches
	unsigned int m_batchCount[eBatchCount];					// Number of primitives in each batch per frame
	unsigned int m_lineCount;								// Number of lines per frame
	unsigned int m_viewWidth;								// Cache of arguments passed to init
	unsigned int m_viewHeight;								// Cache of arguments passed to init
	unsigned int m_bpp;										// Cache of arguments passed to init
	float		 m_aspect;									// Calculated ratio of width to height
	Colour m_clearColour;									// Cache of arguments passed to init
	eRenderMode m_renderMode;								// How the scene is to be rendered

	static const unsigned int s_maxPrimitivesPerBatch = 64 * 1000;		// Flat storage amount for quads
	static const unsigned int s_maxLines = 1600;						// Storage amount for debug lines
	static const float s_nearClipPlane;									// Distance from the viewer to the near clipping plane (always positive) 
	static const float s_farClipPlane;									// Distance from the viewer to the far clipping plane (always positive).
	static const float s_fovAngleY;										// Field of view angle, in degrees, in the y direction.
};

#endif // _ENGINE_RENDER_MANAGER
