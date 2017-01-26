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
#include "../core/Range.h"
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

//\brief Type of objects to draw
namespace RenderObjectType
{
	enum Enum
	{
		Tris = 0,
		Quads,
		Lines,
		DebugBoxes,
		DebugSpheres,
		DebugTransforms,
		Models,
		FontChars,
		Count,
	};
}

//\brief RenderManager separates rendering from the rest of the engine by wrapping all 
//		 calls to OpenGL with some abstract concepts like rendering quads, primitives and meshes
class RenderManager : public Singleton<RenderManager>
{
public:

	//\brief No work done in the constructor, only Init
	explicit RenderManager(float a_updateFreq = s_updateFreq) 
					: m_renderTime(0.0f)
					, m_lastRenderTime(0.0f)
					, m_clearColour(sc_colourBlack)
					, m_renderMode(RenderMode::Full)
					, m_vr(false)
					, m_postShader(nullptr)
					, m_colourShader(nullptr)
					, m_textureShader(nullptr)
					, m_lightingShader(nullptr)
					, m_particleShader(nullptr)
					, m_fullscreenQuad()
					, m_numParticleEmitters(0)
					, m_debugBoxBuffer()
					, m_debugSphereBuffer()
					, m_viewWidth(0)
					, m_viewHeight(0)
					, m_bpp(0)
					, m_aspect(1.0f) 
					, m_updateFreq(a_updateFreq)
					, m_updateTimer(0.0f) 
	{ 
		m_shaderPath[0] = '\0'; 

		for (int i = 0; i < RenderLayer::Count; ++i)
		{
			m_tris[i] = nullptr;
			m_quads[i] = nullptr;
			m_lines[i] = nullptr;
			m_debugBoxes[i] = nullptr;
			m_debugSpheres[i] = nullptr;
			m_debugTransforms[i] = nullptr;
			m_models[i] = nullptr;
			m_fontChars[i] = nullptr;
			for (int j = 0; j < RenderObjectType::Count; ++j)
			{
				m_objectCount[i][j] = 0;
			}
		}

		for (int i = 0; i < RenderStage::Count; ++i)
		{
			m_frameBuffers[i] = 0;
			m_colourBuffers[i] = 0;
			m_depthBuffers[i] = 0;
		}

		for (int i = 0; i < s_numRenderTargets; ++i)
		{
			m_renderTargets[i] = 0;
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
	inline void SetRenderTargetSize(unsigned int a_viewWidth, unsigned int a_viewHeight) { m_viewWidth = a_viewWidth; m_viewHeight = a_viewHeight; }
	inline void Set2DRenderDepth(float a_newDepth) { s_renderDepth2D = a_newDepth; }

	//\brief Dump everything to the buffer after transforming to an arbitrary coordinate system
	//\param a_viewMatrix const ref to a matrix to be loaded into the modelview, usually the camera matrix
	void DrawToScreen(Matrix & a_viewMatrix);
	void RenderScene(Matrix & a_viewMatrix, bool a_flushBuffers = true);
	void RenderScene(Matrix & a_viewMatrix, Matrix & a_perspectiveMat, bool a_flushBuffers, bool a_clear);
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
	inline Shader * GetPostShader() { return m_postShader; }
	inline Shader * GetColourShader() { return m_colourShader; }
	inline Shader * GetTextureShader() { return m_textureShader; }
	inline Shader * GetLightingShader() { return m_lightingShader; }
	inline unsigned int GetGBufferIndex(unsigned int a_id) { return m_renderTargets[a_id]; }

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
	void AddFontChar(RenderLayer::Enum a_renderLayer, const Vector2& a_charSize, const TexCoord & a_texSize, const TexCoord & a_texCoord, Texture * a_texture, const Vector2 & a_size, Vector a_pos, Colour a_colour = sc_colourWhite);					 
	
	//\brief Add a particle emitter
	//\param a_numParticles the maximum particles that can be alive at once for this emitter
	//\param a_emissionRate how many particles should be emitted per second
	//\param a_lifeTime the life of the emitter
	int AddParticleEmitter(int a_numParticles, float a_emissionRate, float a_lifeTime/*, const ParticleDefinition & a_def*/);
	void RemoveAllParticleEmitters();

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

	//\brief A transform is position and orientation displayed with coloured lines
	//\param a const ref of the matrix containing the position and orientation to display
	void AddDebugTransform(const Matrix & a_mat);

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

	static const int s_numRenderTargets = 6;						///< Number of screen sized gbuffers for general use

private:

	//\brief Data set for passing into a vertex buffer
	struct Vertex
	{
		Vertex()
			: m_pos(0.0f, 0.0f, 0.0f)
			, m_colour(0.0f, 0.0f, 0.0f, 0.0f)
			, m_uv(0.0f, 0.0f)
			, m_normal(0.0f, 0.0f, 0.0f) {}
		Vector m_pos;
		Colour m_colour;
		TexCoord m_uv;
		Vector m_normal;
	};

	struct VertexBuffer
	{
		VertexBuffer()
			: m_vertexArrayId(0)
			, m_vertexBufferId(0)
			, m_indexBufferId(0)
			, m_textureId(0)
			, m_verts(nullptr)
			, m_indicies(nullptr)
			, m_numVerts(0) {}
		VertexBuffer(unsigned int a_numVerts)
			: m_vertexArrayId(0)
			, m_vertexBufferId(0)
			, m_indexBufferId(0) 
			, m_textureId(-1)
			, m_verts(nullptr)
			, m_indicies(nullptr)
			, m_numVerts(a_numVerts)
		{
			Alloc(a_numVerts);
		}
		~VertexBuffer() 
		{ 
			if (m_numVerts > 0) 
			{ 
				Dealloc(); 
			} 
		}
		inline void Alloc(unsigned int a_numVerts)
		{
			if (m_numVerts > 0)
			{
				Dealloc();
			}
			m_numVerts = a_numVerts;
			m_verts = new Vertex[a_numVerts];
			m_indicies = new unsigned int[a_numVerts];
		}
		void Dealloc()
		{
			m_numVerts = 0;
			delete m_verts;
			delete m_indicies;
			m_verts = nullptr;
			m_indicies = nullptr;
		}
		inline void Realloc(unsigned int a_numVerts)
		{
			Dealloc();
			Alloc(a_numVerts);
		}
		void Bind();
		void Rebind();
		void Unbind();
		void SetVert(unsigned int a_index, const Vector& a_pos, const Colour& a_colour, const TexCoord& a_uv, const Vector& a_normal)
		{
			m_verts[a_index].m_pos = a_pos;
			m_verts[a_index].m_colour = a_colour;
			m_verts[a_index].m_uv = a_uv;
			m_verts[a_index].m_normal = a_normal;
			m_indicies[a_index] = a_index;
		}
		void SetVert2D(unsigned int a_index, const Vector& a_pos, const Colour& a_colour, const TexCoord& a_uv)
		{
			m_verts[a_index].m_pos = a_pos;
			m_verts[a_index].m_colour = a_colour;
			m_verts[a_index].m_uv = a_uv;
			m_verts[a_index].m_normal = Vector(0.0, 0.0, -1.0f);
			m_indicies[a_index] = a_index;
		}
		void SetVertBasic(unsigned int a_index, const Vector& a_pos, const Colour& a_colour)
		{
			m_verts[a_index].m_pos = a_pos;
			m_verts[a_index].m_colour = a_colour;
			m_verts[a_index].m_uv = Vector2(0.0f, 0.0f);
			m_verts[a_index].m_normal = Vector(0.0, 0.0, -1.0f);
			m_indicies[a_index] = a_index;
		}
		unsigned int m_vertexArrayId;
		unsigned int m_vertexBufferId;
		unsigned int m_indexBufferId;
		int m_textureId;
		Vertex* m_verts;
		unsigned int* m_indicies;
		int m_numVerts;
	};

	//\brief Fixed size structure for queing line primitives
	struct Line : VertexBuffer
	{
		Line() : VertexBuffer(2) {}
	};

	//\brief Fixed size structure for queing render primitices
	struct Tri : VertexBuffer
	{
		Tri() : VertexBuffer(3) {}
	};

	//\brief Fixed size structure for queing render primitives
	struct Quad : VertexBuffer
	{
		Quad() : VertexBuffer(4) {}
	};

	struct DebugBox
	{
		DebugBox() : m_pos(0.0f), m_scale(0.0f) {}
		Vector m_pos;
		Vector m_scale;
	};

	struct DebugSphere
	{
		DebugSphere() : m_pos(0.0f), m_scale(0.0f) {}
		Vector m_pos;
		Vector m_scale;
	};

	struct DebugTransform
	{
		DebugTransform() : m_mat() {}
		Matrix m_mat;
	};

	// Buffer data for a model
	struct RenderModelBuffer : VertexBuffer
	{
		RenderModelBuffer()
			: m_model(nullptr)
			, m_object(nullptr) {}
		Model * m_model;
		Object * m_object;
	};

	//\brief Fixed size structure for queing render models
	struct RenderModel
	{
		RenderModel()
			: m_buffer(nullptr)
			, m_mat(nullptr)
			, m_shader(nullptr)
			, m_material(nullptr)
			, m_lifeTime(0.0f)
			, m_shininess(1024)
			, m_ambient(1.0f)
			, m_diffuse(1.0f)
			, m_specular(1.0f)
			, m_emission(1.0f)
			, m_shaderData(0.0f, 0.0f, 0.0f)
			, m_diffuseTexId(0)
			, m_normalTexId(0)
			, m_specularTexId(0) {}
		
		RenderModelBuffer * m_buffer;
		Matrix * m_mat;
		Shader * m_shader;
		Material * m_material;
		float m_lifeTime;
		int m_shininess;
		Vector m_ambient;
		Vector m_diffuse;
		Vector m_specular;
		Vector m_emission;
		Vector m_shaderData;
		unsigned int m_diffuseTexId;
		unsigned int m_normalTexId;
		unsigned int m_specularTexId;
	};

	//\brief Fixes size structure for queing font characters that are just a display list
	struct FontChar : VertexBuffer
	{
		FontChar() : VertexBuffer(4) {}
		Vector m_pos;
		Vector m_scale;
	};

	//\brief Data that the user authors for each particle system
	struct ParticleDefinition
	{
		ParticleDefinition()
			: m_lifeTime(-1.0f)
			, m_startSize(-1.0f)
			, m_endSize(-1.0f)
			, m_startColour(Colour(0.0f, 0.0f, 0.0f, 0.0f))
			, m_endColour(Colour(0.0f, 0.0f, 0.0f, 0.0f))
			, m_startVel(Vector(0.0f, 0.0f, 0.0f))
			, m_endVel(Vector(0.0f, 0.0f, 0.0f))
		{}

		inline void SetDefault()
		{
			m_lifeTime = Range<float>(0.5f, 2.0f);
			m_startSize = Range<float>(0.25f, 0.5f);
			m_endSize = Range<float>(9.0f, 10.0f);
			m_startColour = Range<Colour>(Colour(1.0f, 1.0f, 1.0f, 0.75f), Colour(1.0f, 1.0f, 1.0f, 1.0f));
			m_endColour = Range<Colour>(Colour(0.0f, 1.0f, 1.0f, 1.0f));
			m_startVel = Range<Vector>(Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 1.0f, 1.0f));
			m_endVel = Range<Vector>(Vector(0.0f, 0.0f, 0.0f), Vector(0.25f, 0.25f, 0.25f));
		}

		inline void Set(const ParticleDefinition & a_def)
		{
			if (a_def.m_lifeTime.IsValid())				{ m_lifeTime = a_def.m_lifeTime; }
			if (a_def.m_startSize.IsValid())			{ m_startSize = a_def.m_startSize; }
			if (a_def.m_endSize.IsValid())				{ m_endSize = a_def.m_endSize; }
			if (a_def.m_startColour.IsValid())			{ m_startColour = a_def.m_startColour; }
			if (a_def.m_endColour.IsValid())			{ m_endColour = a_def.m_endColour; }
			if (a_def.m_startVel.IsValid())				{ m_startVel = a_def.m_startVel; }
			if (a_def.m_endVel.IsValid())				{ m_endVel = a_def.m_endVel; }
		}

		Range<float> m_lifeTime;
		Range<float> m_startSize;
		Range<float> m_endSize;
		Range<Colour> m_startColour;
		Range<Colour> m_endColour;
		Range<Vector> m_startVel;
		Range<Vector> m_endVel;
	};

	//\brief Data associated with each particle owned by an emitter
	struct Particle
	{
		Particle()
			: m_pos(0.0f, 0.0f, 0.0f)
			, m_colour(0.0f, 0.0f, 0.0f, 0.0f)
			, m_life(0.0f)
			, m_velocity(0.0f, 0.0f, 0.0f)
		{}
		Vector m_pos;
		Colour m_colour;
		float m_life;
		Vector m_velocity;
	};

	// Owns a vertex buffer with each vert being a particle, system is on the GPU
	struct ParticleEmitter
	{
		ParticleEmitter() 
			: m_lifeTime(0.0f)
			, m_vertexArrayId(0)
			, m_vertexBufferId(0)
			, m_indexBufferId(0)
			, m_textureId(0)
			, m_particles(nullptr)
			, m_indicies(nullptr)
			, m_numParticles(0)
			, m_particleDef()
		{}

		inline void Alloc(unsigned int a_numParticles)
		{
			if (m_numParticles > 0)
			{
				Dealloc();
			}
			m_numParticles = a_numParticles;
			m_particles = new Particle[a_numParticles];
			m_indicies = new unsigned int[a_numParticles];
		}
		void Dealloc()
		{
			m_numParticles = 0;
			delete m_particles;
			delete m_indicies;
			m_particles = nullptr;
			m_indicies = nullptr;
		}
		inline void Realloc(unsigned int a_numVerts)
		{
			Dealloc();
			Alloc(a_numVerts);
		}
		void Bind();
		void Rebind();
		void Unbind();

		float m_lifeTime;
		unsigned int m_vertexArrayId;
		unsigned int m_vertexBufferId;
		unsigned int m_indexBufferId;
		int m_textureId;
		Particle * m_particles;
		unsigned int* m_indicies;
		int m_numParticles;
		ParticleDefinition m_particleDef;
	};

	//\brief Container for sorting render models before drawing
	struct SortedRenderModel
	{
		SortedRenderModel() : m_id(0), m_distance(0.0f) {}
		SortedRenderModel(int a_id, float a_distance) : m_id(a_id), m_distance(a_distance) {}
		bool operator > (const SortedRenderModel & a_rhs) { return m_distance > a_rhs.m_distance; }
		bool operator < (const SortedRenderModel & a_rhs) { return m_distance < a_rhs.m_distance; }
		int m_id;
		float m_distance;
	};
	typedef LinkedListNode<SortedRenderModel> SortedRenderNode;

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

	static const int s_maxObjects[(int)RenderObjectType::Count];	///< The amount of storage amount for all types of primitives
	static const int s_maxModelBuffers = 512;						///< Storage for vertex buffers across all layers, unique meshes share buffers
	static const int s_maxParticleEmitters = 256;					///< Storage for VBOs that have particles in them
	static const int s_maxParticles = 1024 * 4;						///< The number of verts per particle buffer
	static const int s_numDebugBoxVerts = 8;
	static const int s_numDebugSphereVerts = 96;
	static const int s_numDebugTransformVerts = 6;
	static const float s_updateFreq;								///< How often the render manager should check for shader updates
	static const float s_nearClipPlane;								///< Distance from the viewer to the near clipping plane (always positive) 
	static const float s_farClipPlane;								///< Distance from the viewer to the far clipping plane (always positive).
	static const float s_fovAngleY;									///< Field of view angle, in degrees, in the y direction
	static float s_renderDepth2D;									///< Z value for ortho rendered primitives

	SortedRenderModel * m_sortedRenderModelPool;					///< Storage for pointers to objects and their render distances
	SortedRenderNode * m_sortedRenderNodePool;						///< Storage for the above in a list for sorting

	float m_renderTime;												///< How long the game has been rendering frames for (accumulated frame delta)
	float m_lastRenderTime;											///< How long the last frame took

	Tri	 * m_tris[RenderLayer::Count];								///< Pointer to a pool of memory for tris
	Quad * m_quads[RenderLayer::Count];								///< Pointer to a pool of memory for quads
	Line * m_lines[RenderLayer::Count];								///< Lines for each renderLayer
	DebugBox * m_debugBoxes[RenderLayer::Count];					///< Debug boxes made of lines
	DebugSphere * m_debugSpheres[RenderLayer::Count];				///< Debug spheres made of lines
	DebugTransform * m_debugTransforms[RenderLayer::Count];			///< Debug transforms made of 3 coloured lines
	RenderModel * m_models[RenderLayer::Count];						///< Models for each renderLayer
	FontChar * m_fontChars[RenderLayer::Count];						///< Characters of a display string
	ParticleEmitter m_particleEmitters[s_maxParticleEmitters];		///< BUffer containing a vert for each particle
	int m_numParticleEmitters;
	int m_objectCount[RenderLayer::Count][(int)RenderObjectType::Count];	///< How many of each object are batched, resets every frame

	unsigned int m_renderTargets[s_numRenderTargets];				///< Identifiers for the general use targets
	unsigned int m_mrtAttachments[s_numRenderTargets + 1];			///< Array of identifies for all colour attachments
	unsigned int m_frameBuffers[RenderStage::Count];				///< Identifier for the whole scene framebuffers for each stage
	unsigned int m_colourBuffers[RenderStage::Count];				///< Identifier for the texture to render to
	unsigned int m_depthBuffers[RenderStage::Count];				///< Identifier for the buffers for pixel depth per stage

	Quad m_fullscreenQuad;											///< Used for drawing full screen buffers
	Quad m_particleBuffer;											///< Single VBO for all world particles
	VertexBuffer m_debugBoxBuffer;									///< One set of vertices for all boxes
	VertexBuffer m_debugSphereBuffer;								///< Same concept for spheres
	VertexBuffer m_debugTransformBuffer;							///< Three coloured lines for transforms
	RenderModelBuffer m_modelBuffers[s_maxModelBuffers];			///< Storage for model vertex data, one per unique mesh across all render stages

	Matrix m_shaderOrthoMat;
	Matrix m_shaderIdentityMat;
	
	Shader * m_postShader;											///< Shader used to draw the gbuffers each frame
	Shader * m_colourShader;										///< Vertex and pixel shader used when no shader is specified in a scene or model
	Shader * m_textureShader;										///< Shader for textured objects when no shader specified
	Shader * m_lightingShader;										///< Shader for objects in scenes with lights specified
	Shader * m_particleShader;										///< Shader that updates the position, scale, colour and lifetime of particles and renders them

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
