#ifndef _ENGINE_SHADER_
#define _ENGINE_SHADER_
#pragma once

#include <string.h>

#include <GL/glew.h>

#include "../core/Matrix.h"

#include "StringUtils.h"

//\brief Abstraction of a simple GLSL shader framework
class Shader
{
public:

	//\brief Useful collection of uniform name, location and value data
	struct Uniform
	{
		Uniform()
			: m_output(false)
			, m_id(0)
			, m_floatValue(0.0f) { m_name[0] = '\0'; }

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
		union											///< The value to be passed or received
		{
			float m_floatValue;							///< Bit dangerous as sizeof(float) may not equal sizeof(int)
			int m_intValue;
		};
	};

	//\brief Struct to collect a standard set of data passed to and from shaders
	struct UniformData
	{
		UniformData(	float a_time,
						float a_life,
						float a_frameTime, 
						float a_viewWidth,
						float a_viewHeight,
						Matrix * a_mat)
						: m_time(a_time)
						, m_life(a_life)
						, m_frameTime(a_frameTime)
						, m_viewWidth(a_viewWidth)
						, m_viewHeight(a_viewHeight) 
						, m_mat(a_mat) { }

		float m_time;					///< How much time in seconds has passed since the app has started
		float m_life;					///< How much time in seconds has passed since the object using the shader was initialised
		float m_frameTime;				///< How much time in seconds has passed since the last frame was drawn
		float m_viewWidth;				///< Framebuffer render resolution width
		float m_viewHeight;				///< Framebuffer render resolution height
		Matrix * m_mat;					///< Pointer to matrix containing game object position
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
			m_life.Init(m_shader, "Life");
			m_frameTime.Init(m_shader, "FrameTime");
			m_viewWidth.Init(m_shader, "ViewWidth");
			m_viewHeight.Init(m_shader, "ViewHeight");
			m_objectMatrix.Init(m_shader, "ObjMat");
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
		glUseProgram(m_shader);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(m_texture.m_id, 0);
		glUniform1f(m_time.m_id, a_data.m_time);
		glUniform1f(m_life.m_id, a_data.m_life);
		glUniform1f(m_frameTime.m_id, a_data.m_frameTime);
		glUniform1f(m_viewWidth.m_id, a_data.m_viewWidth);
		glUniform1f(m_viewWidth.m_id, a_data.m_viewHeight);
		glUniformMatrix4fv(m_objectMatrix.m_id, 1, GL_FALSE, a_data.m_mat->GetValues());
	}

    ~Shader() {
        glDeleteProgram(m_shader);
        glDeleteShader(m_vertexShader);
        glDeleteShader(m_fragmentShader);
    }

private:

	//\brief Compile the shader given the source code
    unsigned int Compile(GLuint type, const char * a_src);
	
	char m_name[StringUtils::s_maxCharsPerName];	///< Name of the files minus .fsh and .vsh extensions
	GLuint m_vertexShader;							///< Program to transform vertices
	GLuint m_fragmentShader;						///< Pixel shader
	GLuint m_shader;								///< Linked program

	Uniform m_texture;								///< Standard set of uniforms follow
	Uniform m_time;
	Uniform m_life;
	Uniform m_frameTime;
	Uniform m_viewWidth;
	Uniform m_viewHeight;
	Uniform m_objectMatrix;
};

#endif // _ENGINE_RENDER_MANAGER
