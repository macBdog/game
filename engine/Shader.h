#ifndef _ENGINE_SHADER_
#define _ENGINE_SHADER_
#pragma once

#include <string.h>

#include "../core/Colour.h"
#include "../core/Matrix.h"
#include "../core/Quaternion.h"

#include "StringUtils.h"

typedef unsigned int GLuint;

//\brief Lighting data passed between the game config files and the standard lighting shader
struct Light
{
	Light() 
		: m_enabled(false)
		, m_pos(0.0f)
		, m_dir(0.0f)
		, m_ambient(0.0f)
		, m_diffuse(0.0f)
		, m_specular(0.0f) 
		{ 
			m_name[0] = '\0'; 
		}
	bool m_enabled;									///< If the light is currently on
	char m_name[StringUtils::s_maxCharsPerName];	///< Name of the light is used for attaching lights to objects
	Vector m_pos;									///< World position of the light
	Quaternion m_dir;								///< Direction the light is facing, zero means its an omni light
	Colour m_ambient;								///< Ambient light colour is an aproximation of all reflected light
	Colour m_diffuse;								///< Colour of light scattered equally off a lit surface
	Colour m_specular;								///< Colour of light reflected in one direction off a lit surface

	static const float s_lightDrawSize;				///< The size the light is drawn in the in game editor
};

//\brief Abstraction of a simple GLSL shader framework
class Shader
{
public:

	static const int s_maxLights = 4; ///< Maximum amount of light data passed to lighting shaders

	//\brief Useful collection of uniform name, location and value data
	template <typename TDataType>
	struct Uniform
	{
		Uniform()
			: m_output(false)
			, m_id(0)
		{
			m_name[0] = '\0';
		}

		// Initialise values from graphics API on construction
		bool Init(GLuint a_sourceShader, const char * a_name, bool a_output = false)
		{
			if (a_name != NULL && a_name[0] != '\0')
			{
				strncpy(m_name, a_name, sizeof(char) * strlen(a_name) + 1);
				m_id = glGetUniformLocation(a_sourceShader, m_name);
				m_output = a_output;
			}
			return m_id >= 0;
		}

		bool m_output;									///< If the uniform is for input or output
		int m_id;										///< Identifier for the uniform
		char m_name[StringUtils::s_maxCharsPerName];	///< Name of the variable referenced in the shader
		TDataType m_data;								///< Data that is referenced by name
	};

	//\brief Struct to collect a standard set of data passed to and from shaders
	struct UniformData
	{
		UniformData(	float a_time,
						float a_lifeTime,
						float a_frameTime, 
						float a_viewWidth,
						float a_viewHeight,
						Vector a_shaderData,
						Matrix * a_objectMatrix,
						Matrix * a_viewMatrix,
						Matrix * a_projectionMatrix)
						: m_diffuseTextureId(0)
						, m_normalTextureId(0)
						, m_specularTextureId(0)
						, m_depthBuffer(0)
						, m_materialShininess(1024)
						, m_time(a_time)
						, m_lifeTime(a_lifeTime)
						, m_frameTime(a_frameTime)
						, m_viewWidth(a_viewWidth)
						, m_viewHeight(a_viewHeight)
						, m_materialAmbient(1.0f)
						, m_materialDiffuse(1.0f)
						, m_materialSpecular(1.0f)
						, m_materialEmission(1.0f)
						, m_shaderData(a_shaderData)
						, m_objectMatrix(a_objectMatrix)
						, m_viewMatrix(a_viewMatrix)
						, m_projectionMatrix(a_projectionMatrix)
						{
							for (int i = 0; i < 6; ++i)
							{
								m_gBufferIds[i] = 0;
							}	
						}

		unsigned int m_diffuseTextureId;
		unsigned int m_normalTextureId;
		unsigned int m_specularTextureId;
		unsigned int m_gBufferIds[6];
		unsigned int m_depthBuffer;
		int m_materialShininess;
		float m_time;					///< How much time in seconds has passed since the app has started
		float m_lifeTime;				///< How much time in seconds has passed since the object using the shader was initialised
		float m_frameTime;				///< How much time in seconds has passed since the last frame was drawn
		float m_viewWidth;				///< Framebuffer render resolution width
		float m_viewHeight;				///< Framebuffer render resolution height
		Vector m_materialAmbient;
		Vector m_materialDiffuse;
		Vector m_materialSpecular;
		Vector m_materialEmission;
		Vector m_shaderData;			///< 3 generic floats to pass to the shader
		Matrix * m_objectMatrix;		///< Pointer to matrix containing game object position
		Matrix * m_viewMatrix;			///< Pointer to matrix containing game object position
		Matrix * m_projectionMatrix;
		Light m_lights[s_maxLights];	///< Support for multiple lights in the scene
	};

	//\brief Shaders will compile and link from source upon creation
	Shader(const char * a_name);
	~Shader();

	bool Init(const char * a_vertexSource, const char * a_fragmentSource, const char * a_geometrySource = nullptr);

	//\brief Accessors and mutators
	inline GLuint GetShader() { return m_shader; }
	inline const char * GetName() { return &m_name[0]; }
	inline bool IsCompiled() { return m_vertexShader > 0 && m_fragmentShader > 0 && m_shader > 0; }
	
	//\brief Bind the shader and setup the uniforms
	void UseShader();
	void UseShader(const UniformData & a_data);

private:

	// Lighting parameters are written to shader as an array of floats
	static const int s_numLightParameters = 4;
	static const int s_numFloatsPerParameter = 3;
	static const int s_numLightFloats = s_maxLights * ((s_numLightParameters * s_numFloatsPerParameter) + 1);

	//\brief Static data to write light parameter floats into
	static float s_lightingData[s_numLightFloats];

	//\brief Compile the shader given the source code
    unsigned int Compile(GLuint type, const char * a_src);
	
	char m_name[StringUtils::s_maxCharsPerName];	///< Name of the files minus .fsh and .vsh extensions
	GLuint m_vertexShader;							///< Program to transform vertices
	GLuint m_fragmentShader;						///< Pixel shader
	GLuint m_geometryShader;						///< Geometry shader
	GLuint m_shader;								///< Linked program

	Uniform<int> m_diffuseTexture;					///< Standard set of uniforms follow
	Uniform<int> m_normalTexture;
	Uniform<int> m_specularTexture;
	Uniform<int> m_gBuffer1;
	Uniform<int> m_gBuffer2;
	Uniform<int> m_gBuffer3;
	Uniform<int> m_gBuffer4;
	Uniform<int> m_gBuffer5;
	Uniform<int> m_gBuffer6;
	Uniform<int> m_depthBuffer;
	Uniform<int> m_materialShininess;
	Uniform<int> m_numActiveLights;
	Uniform<float> m_time;
	Uniform<float> m_lifeTime;
	Uniform<float> m_frameTime;
	Uniform<float> m_viewWidth;
	Uniform<float> m_viewHeight;
	Uniform<Vector> m_materialAmbient;
	Uniform<Vector> m_materialDiffuse;
	Uniform<Vector> m_materialSpecular;
	Uniform<Vector> m_materialEmission;
	Uniform<Vector> m_shaderData;
	Uniform<Matrix> m_objectMatrix;
	Uniform<Matrix> m_viewMatrix;
	Uniform<Matrix> m_projectionMatrix;
	Uniform<Light> m_lights;
};

#endif // _ENGINE_RENDER_MANAGER
