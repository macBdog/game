#ifndef _ENGINE_SHADER_
#define _ENGINE_SHADER_
#pragma once

#include <string.h>

#include <GL/glew.h>

#include "../core/Colour.h"
#include "../core/Matrix.h"
#include "../core/Quaternion.h"

#include "StringUtils.h"

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
						Matrix * a_mat)
						: m_time(a_time)
						, m_lifeTime(a_lifeTime)
						, m_frameTime(a_frameTime)
						, m_viewWidth(a_viewWidth)
						, m_viewHeight(a_viewHeight) 
						, m_mat(a_mat)
						{ }

		float m_time;					///< How much time in seconds has passed since the app has started
		float m_lifeTime;				///< How much time in seconds has passed since the object using the shader was initialised
		float m_frameTime;				///< How much time in seconds has passed since the last frame was drawn
		float m_viewWidth;				///< Framebuffer render resolution width
		float m_viewHeight;				///< Framebuffer render resolution height
		Matrix * m_mat;					///< Pointer to matrix containing game object position
		Light m_lights[s_maxLights];	///< Support for multiple lights in the scene
	};

	//\brief Shaders will compile and link from source upon creation
    Shader(const char * a_name)
		: m_vertexShader(0)
		, m_fragmentShader(0)
		, m_shader(0)
	{
		if (a_name != NULL || a_name[0] != '\0')
		{
			// Set data
			m_name[0] = '\0';
			strncpy(m_name, a_name, sizeof(char) * strlen(a_name) + 1);
		}
	}

	inline bool Init(const char * a_vertexSource, const char * a_fragmentSource)
	{
		if (m_name == NULL		|| a_vertexSource == NULL		|| a_fragmentSource == NULL ||
			m_name[0] == '\0'	|| a_vertexSource[0] == '\0'	|| a_fragmentSource[0] == '\0')
		{
			return false;
		}

		// Compile the shader components
		m_vertexShader = Compile(GL_VERTEX_SHADER, a_vertexSource);
		m_fragmentShader = Compile(GL_FRAGMENT_SHADER, a_fragmentSource);
        
		// Only proceed if the compilation of both shader components was successful
		if (m_vertexShader > 0 && m_fragmentShader > 0)
		{
			m_shader = glCreateProgram();
			glAttachShader(m_shader, m_vertexShader);
			glAttachShader(m_shader, m_fragmentShader);
			glLinkProgram(m_shader);

			// Set up the standard uniforms
			m_texture.Init(m_shader, "Texture0");
			m_time.Init(m_shader, "Time");
			m_lifeTime.Init(m_shader, "LifeTime");
			m_frameTime.Init(m_shader, "FrameTime");
			m_viewWidth.Init(m_shader, "ViewWidth");
			m_viewHeight.Init(m_shader, "ViewHeight");
			m_objectMatrix.Init(m_shader, "ObjMat");
			m_lights.Init(m_shader, "Lights");
			return true;
		}

		return false;
    }

	//\brief Accessors and mutators
	inline GLuint GetShader() { return m_shader; }
	inline const char * GetName() { return &m_name[0]; }
	inline bool IsCompiled() { return m_vertexShader > 0 && m_fragmentShader > 0 && m_shader > 0; }
	
	//\brief Bind the shader and setup the uniforms
	inline void UseShader() { glUseProgram(m_shader); glActiveTexture(GL_TEXTURE0); }
	inline void UseShader(const UniformData & a_data) 
	{
		// Write the lighting data into the static memory location for the shader to reference
		int lightValCount = 0;
		for (int i = 0; i < s_maxLights; ++i)
		{
			if (a_data.m_lights[i].m_enabled)
			{
				// Position first
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_pos.GetX();
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_pos.GetY();
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_pos.GetZ();
				// TODO Support for light direction
				// Ambient
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_ambient.GetR();
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_ambient.GetG();
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_ambient.GetB();
				// Diffuse
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_diffuse.GetR();
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_diffuse.GetG();
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_diffuse.GetB();
				//Specular
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_specular.GetR();
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_specular.GetG();
				s_lightingData[lightValCount++] = a_data.m_lights[i].m_specular.GetB();
			}
		}

		glUseProgram(m_shader);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(m_texture.m_id, 0);
		glUniform1f(m_time.m_id, a_data.m_time);
		glUniform1f(m_lifeTime.m_id, a_data.m_lifeTime);
		glUniform1f(m_frameTime.m_id, a_data.m_frameTime);
		glUniform1f(m_viewWidth.m_id, a_data.m_viewWidth);
		glUniform1f(m_viewWidth.m_id, a_data.m_viewHeight);
		glUniform1fv(m_lights.m_id, s_maxLights * s_numLightParameters * s_numFloatsPerParameter, &s_lightingData[0]); 
		glUniformMatrix4fv(m_objectMatrix.m_id, 1, GL_FALSE, a_data.m_mat->GetValues());
	}

    ~Shader() {
        glDeleteProgram(m_shader);
        glDeleteShader(m_vertexShader);
        glDeleteShader(m_fragmentShader);
    }

private:

	// Lighting parameters are written to shader as an array of floats
	static const int s_numLightParameters = 4;
	static const int s_numFloatsPerParameter = 3;

	//\brief Static data to write light parameter floats into
	static float s_lightingData[s_maxLights * s_numLightParameters * s_numFloatsPerParameter];

	//\brief Compile the shader given the source code
    unsigned int Compile(GLuint type, const char * a_src);
	
	char m_name[StringUtils::s_maxCharsPerName];	///< Name of the files minus .fsh and .vsh extensions
	GLuint m_vertexShader;							///< Program to transform vertices
	GLuint m_fragmentShader;						///< Pixel shader
	GLuint m_shader;								///< Linked program

	Uniform<int> m_texture;							///< Standard set of uniforms follow
	Uniform<float> m_time;
	Uniform<float> m_lifeTime;
	Uniform<float> m_frameTime;
	Uniform<float> m_viewWidth;
	Uniform<float> m_viewHeight;
	Uniform<Matrix> m_objectMatrix;
	Uniform<Light> m_lights;
};

#endif // _ENGINE_RENDER_MANAGER
