#ifndef _ENGINE_RENDER_MANAGER_
#define _ENGINE_RENDER_MANAGER_
#pragma once

#include "FileManager.h"
#include "Model.h"
#include "Singleton.h"
#include "Shader.h"
#include "Texture.h"

#include "../core/Colour.h"
#include "../core/LinkedList.h"
#include "../core/Matrix.h"
#include "../core/Vector.h"

class DataPack;
class GameObject;
class Scene;

//\brief Render modes are used to change how the scene is rendered wholesale
namespace RenderMode
{
	enum Enum
	{
		None = 0,
		Wireframe,
		Full,
		Count,
	};
}

//\brief A render layer is a way to specify rendering order
namespace RenderLayer
{
	enum Enum
	{
		None = 0,		//< It's valid to render in the none renderLayer
		World,			//< But it will be drawn over by the world
		Gui,			//< GUI covers the world
		Debug2D,		//< Debug text over everything
		Debug3D,		//< Debug text over everything
		Count,
	};
}

//\brief Render stages define full size target buffers for post processing
namespace RenderStage
{
	enum Enum
	{
		Scene = 0,		//< Full scene rendered to source
		PostFX,			//< Render scene with a different shader
		Count,
	};
}

//\brief RenderManager separates rendering from the rest of the engine by wrapping all 
//		 calls to OpenGL with some abstract concepts like rendering quads, primitives and meshes
class RenderManager : public Singleton<RenderManager>
{
public:

	//\brief No work done in the constructor, only Init
	RenderManager(float a_updateFreq = s_updateFreq) 
					: m_renderTime(0.0f)
					, m_lastRenderTime(0.0f)
					, m_clearColour(sc_colourBlack)
					, m_renderMode(RenderMode::Full)
					, m_vr(false)
					, m_colourShader(NULL)
					, m_textureShader(NULL)
					, m_lightingShader(NULL)
					, m_frameBuffer(0)
					, m_colourBuffer(0)
					, m_depthBuffer(0)
					, m_viewWidth(0)
					, m_viewHeight(0)
					, m_bpp(0)
					, m_aspect(1.0f) 
					, m_updateFreq(a_updateFreq)
					, m_updateTimer(0.0f) 
	{ 
		m_shaderPath[0] = '\0'; 
		for (int i = 0; i < RenderStage::Count; ++i)
		{
			m_frameBuffers[i] = 0;
			m_colourBuffers[i] = 0;
			m_depthBuffers[i] = 0;
		}
	}
	~RenderManager() { Shutdown(); }

	//\brief Set clear colour buffer and depth buffer setup 
	bool Startup(const Colour & a_clearColour, const char * a_shaderPath, const DataPack * a_dataPack, bool a_vr = false);
	bool Shutdown();

	//\brief Update managed texture
	bool Update(float a_dt);

	//\brief Setup the viewport
    bool Resize(unsigned int a_viewWidth, unsigned int a_viewHeight, unsigned int a_viewBpp, bool a_fullScreen = false);

	//\brief Dump everything to the buffer after transforming to an arbitrary coordinate system
	//\param a_viewMatrix const ref to a matrix to be loaded into the modelview, usually the camera matrix
	void DrawToScreen(Matrix & a_viewMatrix);
	void RenderScene(Matrix & a_viewMatrix, bool a_flushBuffers = true);
	void RenderFramebuffer();
	
	//\brief Change the render mode
	//\param a_renderMode the new mode to set
	inline void SetRenderMode(RenderMode::Enum a_renderMode) { m_renderMode = a_renderMode; }

	//\brief Accessors for the viewport dimensions and render properties
	inline unsigned int GetViewWidth() { return m_viewWidth; }
	inline unsigned int GetViewHeight() { return m_viewHeight; }
	inline unsigned int GetViewDepth() { return m_bpp; }
	inline float GetViewAspect() { return m_aspect; }
	inline const char * GetShaderPath() { return m_shaderPath; }
	inline bool GetVrSupport() { return m_vr; }
	inline Shader * GetColourShader() { return m_colourShader; }
	inline Shader * GetTextureShader() { return m_textureShader; }
	inline Shader * GetLightingShader() { return m_lightingShader; }

	//\brief Set up a display list for a font character so drawing only involves calling a list
	//\param a_size is an arbitrary width to height to generate the list at
	//\param a_texCoord is the starting coordinate to draw
	//\param a_texSize is the size of the character in reference to the font texture
	//\return The uint equivalent of the GLuint that is returned from glGenLists
	unsigned int RegisterFontChar(Vector2 a_size, TexCoord a_texCoord, TexCoord a_texSize, Texture * a_texture);
	unsigned int RegisterFontChar3D(Vector2 a_size, TexCoord a_texCoord, TexCoord a_texSize, Texture * a_texture);

	//\brief Drawing functions for lines
	//\param a_point1 is the start of the line
	//\param a_point2 is the end of the line
	//\a_tint is the colour to draw the line
	void AddLine2D(RenderLayer::Enum a_layer, Vector2 a_point1, Vector2 a_point2, Colour a_tint = sc_colourWhite);

	//\brief Quad drawing function with manual texture coordinates
	//\param texCoord is the top left of the texture coordinate box, with 0,0 being top left
	//\param texSize is the bounds of the texture coordinate box with 1,1 being the bottom right
	void AddQuad2D(RenderLayer::Enum a_layer, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, TexCoord a_texCoord, TexCoord a_texSize, TextureOrientation::Enum a_orient = TextureOrientation::Normal, Colour a_tint = sc_colourWhite);
	void AddQuad2D(RenderLayer::Enum a_layer, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, TextureOrientation::Enum a_orient = TextureOrientation::Normal, Colour a_tint = sc_colourWhite);
	void AddQuad2D(RenderLayer::Enum a_layer, Vector2 * a_verts, Texture * a_tex, TexCoord a_texCoord, TexCoord a_texSize, TextureOrientation::Enum a_orient = TextureOrientation::Normal, Colour a_tint = sc_colourWhite);
	void AddQuad3D(RenderLayer::Enum a_layer, Vector * a_verts, Texture * a_tex, Colour a_tint = sc_colourWhite);

	//\brief 3D Drawing functions
	void AddLine(RenderLayer::Enum a_layer, Vector a_point1, Vector a_point2, Colour a_tint = sc_colourWhite);
	void AddTri(RenderLayer::Enum a_layer, Vector a_point1, Vector a_point2, Vector a_point3, 
								TexCoord a_txc1, TexCoord a_txc2, TexCoord a_txc3,
								Texture * a_tex, Colour a_tint = sc_colourWhite);
	
	//\brief Add a 3D model for drawing
	//\param a_renderLayer is the rendering group to draw the model in
	//\param a_model is a pointer to the loaded model to draw
	//\param a_mat is a pointer to the position and orientation to draw the model at
	//\param a_shader is a pointer to a shader to render the model with if any
	//\param a_life is how old the object that owns the model is, in seconds
	void AddModel(RenderLayer::Enum a_layer, Model * a_model, Matrix * a_mat, Shader * a_shader, const Vector & a_shaderData, float a_lifeTime);

	//\brief Add a font character for drawing
	//\param a_renderLayer is the rendering group to draw the model in
	//\param a_fontCharId is the display list ID of the character to call
	//\param a_size is the size multiplier to use
	//\param a_pos is the position in 3D space to draw. If a 2D renderLayer is used, the Z component will be ignored
	void AddFontChar(RenderLayer::Enum a_layer, unsigned int a_fontCharId, const Vector2 & a_size, Vector a_pos, Colour a_colour = sc_colourWhite);
	
	//\brief Add a line to the debug renderLayer
	//\param Vector a_point1 start of the line
	//\param Vector a_point2 end of the line
	//\param Colour a_tint the colour of the line
	inline void AddDebugLine(Vector a_point1, Vector a_point2, Colour a_tint = sc_colourWhite) { AddLine(RenderLayer::Debug3D, a_point1, a_point2, a_tint); }
	inline void AddDebugLine2D(Vector2 a_point1, Vector2 a_point2, Colour a_tint = sc_colourWhite) { AddLine2D(RenderLayer::Debug2D, a_point1, a_point2, a_tint); }
	inline void AddDebugQuad2D(Vector2 a_topLeft, Vector2 a_size, Colour a_tint = sc_colourWhite) { AddQuad2D(RenderLayer::Debug2D, a_topLeft, a_size, NULL, TextureOrientation::Normal, a_tint); }
	
	//\brief Add a arrow point from a source point to destination
	void AddDebugArrow(Vector a_start, Vector a_end, Colour a_tint = sc_colourWhite);
	void AddDebugArrow2D(Vector2 a_start, Vector2 a_end, Colour a_tint = sc_colourWhite);

	//\brief A matrix is position and orientation displayed with lines
	//\param a const ref of the matrix containing the position and orientation to display
	void AddDebugMatrix(const Matrix & a_mat);

	//\brief A sphere is a position and radius displayed with lines
	//\param a_colour optional argument for the colour of the box
	void AddDebugSphere(const Vector & a_worldPos, const float & a_radius, Colour a_colour = sc_colourWhite);

	//\brief A box aligned to the world's axis
	//\param a_worldPos the centre of the box
	//\param a_dimensions the size of the box in x,y,z order
	//\param a_colour optional argument for the colour of the box
	void AddDebugAxisBox(const Vector & a_worldPos, const Vector & a_dimensions, Colour a_colour = sc_colourWhite);
	void AddDebugBox(const Matrix & a_worldMat, const Vector & a_dimensions, Colour a_colour = sc_colourWhite);

	//\brief Add and remove a shader to the list for hotloading on file modification
	void ManageShader(GameObject * a_gameObject, const char * a_shaderName);
	void ManageShader(Scene * a_scene, const char * a_shaderName);
	void UnManageShader(GameObject * a_gameObject);
	void UnManageShader(Scene * a_scene);

	//\brief Helper function to setup a new shader based on the contents of files and the global preamble
	//\param a_shaderToCreate_OUT is pointer to a shader that will be allocated
	//\param a_shaderFileName pointer to a cstring containing the path to the shaders with .fsh and .vsh extensions assumed
	//\return true if the shader was compiled and allocated successfully
	static bool InitShaderFromFile(Shader & a_shader_OUT);

	//\brief Helper function to setup a new shader based on the contents of an in memory buffer
	//\param a_vertShaderSrc is a pointer to a cstring containing the vertex shader source
	//\param a_fragShaderSrc is a pointer to a cstring containing the fragment shader source
	//\param a_shader_OUT is pointer to a shader that will be allocated
	//\return true if the shader was compiled and allocated successfully
	static bool InitShaderFromMemory(char * a_vertShaderSrc, char * a_fragShaderSrc, Shader & a_shader_OUT);

private:

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
		Shader * m_shader;
		Material * m_material;
		float m_lifeTime;
		Vector m_shaderData;
	};

	//\brief Fixes size structure for queing font characters that are just a display list
	struct FontChar
	{
		unsigned int m_displayListId;
		Vector2 m_size;
		Vector m_pos;
		Colour m_colour;
		bool m_2d;
	};

	//\brief A managed shader contains a reference to the shader and the file to reload on change for hot reloading
	struct ManagedShader
	{
		ManagedShader() 
			: m_shaderScene(NULL)
			, m_shaderObject(NULL)
			, m_vertexTimeStamp()
			, m_fragmentTimeStamp() { }
		Scene * m_shaderScene;										///< Pointer the scene that references the shader
		GameObject * m_shaderObject;								///< Pointer to the object that references the shader
		FileManager::Timestamp m_vertexTimeStamp;					///< Datestamp for checking a newer version of the geom shader
		FileManager::Timestamp m_fragmentTimeStamp;					///< Datestamp for checking a newer version of the pixel shader
	};
	
	//\brief Add a shader to the list of managed shaders
	//\param a_newManShader is a pointer to the manager shader struct pre-filled by the rendermanager
	void AddManagedShader(ManagedShader * a_newManShader);
	Shader * GetShader(const char * a_shaderName);

	static const int s_maxPrimitivesPerrenderLayer = 64 * 1024;		///< Flat storage amount for quads
	static const int s_maxLines = 1600;								///< Storage amount for debug lines
	static const float s_renderDepth2D;								///< Z value for ortho rendered primitives
	static const float s_updateFreq;								///< How often the render manager should check for shader updates
	static const float s_nearClipPlane;								///< Distance from the viewer to the near clipping plane (always positive) 
	static const float s_farClipPlane;								///< Distance from the viewer to the far clipping plane (always positive).
	static const float s_fovAngleY;									///< Field of view angle, in degrees, in the y direction

	float m_renderTime;												///< How long the game has been rendering frames for (accumulated frame delta)
	float m_lastRenderTime;											///< How long the last frame took

	Tri	 * m_tris[RenderLayer::Count];								///< Pointer to a pool of memory for tris
	Quad * m_quads[RenderLayer::Count];								///< Pointer to a pool of memory for quads
	Line * m_lines[RenderLayer::Count];								///< Lines for each renderLayer
	RenderModel * m_models[RenderLayer::Count];						///< Models for each renderLayer
	FontChar * m_fontChars[RenderLayer::Count];
	unsigned int m_triCount[RenderLayer::Count];					///< Number of tris per renderLayer per frame
	unsigned int m_quadCount[RenderLayer::Count];					///< Number of primitives in each renderLayer per frame
	unsigned int m_lineCount[RenderLayer::Count];					///< Number of lines per frame
	unsigned int m_modelCount[RenderLayer::Count];					///< Number of models to render
	unsigned int m_fontCharCount[RenderLayer::Count];				///< Number for font characters to render

	unsigned int m_frameBuffer;										///< Identifier for the whole scene framebuffers for each stage
	unsigned int m_colourBuffer;									///< Identifier for the texture to render to
	unsigned int m_depthBuffer;										///< Identifier for the buffers for pixel depth per stage

	unsigned int m_frameBuffers[RenderStage::Count];				///< Identifier for the whole scene framebuffers for each stage
	unsigned int m_colourBuffers[RenderStage::Count];				///< Identifier for the texture to render to
	unsigned int m_depthBuffers[RenderStage::Count];				///< Identifier for the buffers for pixel depth per stage

	Shader * m_colourShader;										///< Vertex and pixel shader used when no shader is specified in a scene or model
	Shader * m_textureShader;										///< Shader for textured objects when no shader specified
	Shader * m_lightingShader;										///< Shader for objects in scenes with lights specified

	unsigned int m_viewWidth;										///< Cache of arguments passed to init
	unsigned int m_viewHeight;										///< Cache of arguments passed to init
	unsigned int m_bpp;												///< Cache of arguments passed to init
	float m_aspect;													///< Calculated ratio of width to height
	Colour m_clearColour;											///< Cache of arguments passed to init
	RenderMode::Enum m_renderMode;									///< How the scene is to be rendered

	bool m_vr;														///< If the oculus rendering component will be wedged in between the normal rendering calls

	char m_shaderPath[StringUtils::s_maxCharsPerLine];				///< Path to the shader files

	typedef LinkedListNode<ManagedShader> ManagedShaderNode;		///< Alias for a linked list node that points to a managed shader
	typedef LinkedList<ManagedShader> ManagedShaderList;			///< Alias for a linked list of managed shaders

	ManagedShaderList m_managedShaders;								///< List of managed shaders that are scanned for hot loading
	LinkedList<Shader> m_shaders;									///< List of shaders the game references

	float m_updateFreq;												///< How often the render manager should check for changes to shaders
	float m_updateTimer;											///< If we are due for a scan and update of shaders
};

#endif // _ENGINE_RENDER_MANAGER
