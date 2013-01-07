#ifndef _ENGINE_RENDER_MANAGER_
#define _ENGINE_RENDER_MANAGER_
#pragma once

#include "Model.h"
#include "Singleton.h"
#include "Texture.h"

#include "../core/Colour.h"
#include "../core/Matrix.h"
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
	// CH:TODO Rename from batch, it's incorrect terminology, what about Category? Layer?
	enum eBatch
	{
		eBatchNone = 0,		//< It's valid to render in the none batch
		eBatchWorld,		//< But it will be drawn over by the world
		eBatchGui,			//< GUI covers the world
		eBatchDebug2D,		//< Debug text over everything
		eBatchDebug3D,		//< Debug text over everything

		eBatchCount,
	};
	
	//\ No work done in the constructor, only Init
	RenderManager() : m_clearColour(sc_colourBlack)
					, m_renderMode(eRenderModeFull)
					, m_aspect(1.0f) {}
	~RenderManager() { Shutdown(); }

	//\brief Set clear colour buffer and depth buffer setup 
    bool Startup(Colour a_clearColour);
	bool Shutdown();

	//\brief Setup the viewport
    bool Resize(unsigned int a_viewWidth, unsigned int a_viewHeight, unsigned int a_viewBpp, bool a_fullScreen = false);

	//\brief Dump everything to the buffer after transforming to an arbitrary coordinate system
	//\param a_viewMatrix const ref to a matrix to be loaded into the modelview, usually the camera matrix
	void DrawScene(Matrix & a_viewMatrix);

	//\brief Change the render mode
	//\param a_renderMode the new mode to set
	inline void SetRenderMode(eRenderMode a_renderMode) { m_renderMode = a_renderMode; }

	//\brief Accessors for the viewport dimensions
	inline unsigned int GetViewWidth() { return m_viewWidth; }
	inline unsigned int GetViewHeight() { return m_viewHeight; }
	inline unsigned int GetViewDepth() { return m_bpp; }
	inline float GetViewAspect() { return m_aspect; }

	//\brief Set up a display list for a font character so drawing only involves calling a list
	//\param a_size is an arbitrary width to height to generate the list at
	//\param a_texCoord is the starting coordinate to draw
	//\param a_texSize is the size of the character in reference to the font texture
	//\return The uint equivalent of the GLuint that is returned from glGenLists
	unsigned int RegisterFontChar(Vector2 a_size, TexCoord a_texCoord, TexCoord a_texSize, Texture * a_texture);

	//\brief Drawing functions for lines
	//\param a_point1 is the start of the line
	//\param a_point2 is the end of the line
	//\a_tint is the colour to draw the line
	void AddLine2D(eBatch a_batch, Vector2 a_point1, Vector2 a_point2, Colour a_tint = sc_colourWhite);

	//\brief Quad drawing function with manual texture coordinates
	//\param texCoord is the top left of the texture coordinate box, with 0,0 being top left
	//\param texSize is the bounds of the texture coordinate box with 1,1 being the bottom right
	void AddQuad2D(eBatch a_batch, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, TexCoord a_texCoord, TexCoord a_texSize, Texture::eOrientation a_orient = Texture::eOrientationNormal, Colour a_tint = sc_colourWhite);
	void AddQuad2D(eBatch a_batch, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, Texture::eOrientation a_orient = Texture::eOrientationNormal, Colour a_tint = sc_colourWhite);
	void AddQuad2D(eBatch a_batch, Vector2 * a_verts, Texture * a_tex, TexCoord a_texCoord, TexCoord a_texSize, Texture::eOrientation a_orient = Texture::eOrientationNormal, Colour a_tint = sc_colourWhite);
	void AddQuad3D(eBatch a_batch, Vector * a_verts, Texture * a_tex, Colour a_tint = sc_colourWhite);

	//\brief 3D Drawing functions
	void AddLine(eBatch a_batch, Vector a_point1, Vector a_point2, Colour a_tint = sc_colourWhite);
	void AddTri(eBatch a_batch, Vector a_point1, Vector a_point2, Vector a_point3, 
								TexCoord a_txc1, TexCoord a_txc2, TexCoord a_txc3,
								Texture * a_tex, Colour a_tint = sc_colourWhite);
	
	//\brief Add a 3D model for drawing
	//\param a_batch is the rendering group to draw the model in
	//\param a_model is a pointer to the loaded model to draw
	//\param a_mat is a pointer to the position and orientation to draw the model at
	void AddModel(eBatch a_batch, Model * a_model, Matrix * a_mat);

	//\brief Add a font character for drawing
	//\param a_batch is the rendering group to draw the model in
	//\param a_fontCharId is the display list ID of the character to call
	//\param a_size is the size multiplier to use
	//\param a_pos is the position in 3D space to draw. If a 2D batch is used, the Z component will be ignored
	void AddFontChar(eBatch a_batch, unsigned int a_fontCharId, float a_size, Vector a_pos, Colour a_colour = sc_colourWhite);
	
	//\brief Add a line to the debug batch
	//\param Vector a_point1 start of the line
	//\param Vector a_point2 end of the line
	//\param Colour a_tint the colour of the line
	inline void AddDebugLine(Vector a_point1, Vector a_point2, Colour a_tint = sc_colourWhite) { AddLine(eBatchDebug3D, a_point1, a_point2, a_tint); }

	//\brief A matrix is position and orientation displayed with lines
	//\param a const ref of the matrix containing the position and orientation to display
	void AddDebugMatrix(const Matrix & a_mat);

	//\brief A sphere is a position and radius displayed with lines
	//\param a_colour optional argument for the colour of the box
	void AddDebugSphere(const Vector & a_worldPos, const float & a_radius, Colour a_colour = sc_colourWhite);

	//\brief A boc aligned to the world's axis
	//\param a_worldPos the centre of the box
	//\param a_dimensions the size of the box in x,y,z order
	//\param a_colour optional argument for the colour of the box
	void AddDebugAxisBox(const Vector & a_worldPos, const Vector & a_dimensions, Colour a_colour = sc_colourWhite);

private:

	static const float s_renderDepth2D;			// Z value for ortho rendered primitives

	//\brief Fixed size structure for queing line primitives
	struct Line
	{
		Vector m_verts[2];
		Colour m_colour;
	};

	//\brief Fixed size structure for queing render primitices
	struct Tri
	{
		Vector m_verts[3];
		TexCoord m_coords[3];
		int m_textureId;
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

	//\brief Fixed size structure for queing render models
	struct RenderModel
	{
		Model * m_model;
		Matrix * m_mat;
	};

	//\brief Fixes size structure for queing font characters that are just a display list
	struct FontChar
	{
		unsigned int m_displayListId;
		float m_size;
		Vector m_pos;
		Colour m_colour;
		bool m_2d;
	};

	Tri	 * m_tris[eBatchCount];								// Pointer to a pool of memory for tris
	Quad * m_quads[eBatchCount];							// Pointer to a pool of memory for quads
	Line * m_lines[eBatchCount];							// Lines for each batch
	RenderModel * m_models[eBatchCount];					// Models for each batch
	FontChar * m_fontChars[eBatchCount];
	unsigned int m_triCount[eBatchCount];					// Number of tris per batch per frame
	unsigned int m_quadCount[eBatchCount];					// Number of primitives in each batch per frame
	unsigned int m_lineCount[eBatchCount];					// Number of lines per frame
	unsigned int m_modelCount[eBatchCount];					// Number of models to render
	unsigned int m_fontCharCount[eBatchCount];				// Number for font characters to render
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
