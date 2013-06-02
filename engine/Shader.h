#ifndef _ENGINE_SHADER_
#define _ENGINE_SHADER_
#pragma once

#include <string.h>

#include <GL/glew.h>

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
				strncpy(m_name, a_name, sizeof(char) * strlen(a_name));
				m_id = glGetUniformLocation(a_sourceShader, m_name);
				m_output = a_output;
			}
			return m_id > 0;
		}

		bool m_output;									///< If the uniform is for input or output
		unsigned int m_id;								///< Identifier for the uniform
		char m_name[StringUtils::s_maxCharsPerName];	///< Name of the variable referenced in the shader
		union											///< The value to be passed or received
		{
			float m_floatValue;							///< Bit dangerous as sizeof(float) may not equal sizeof(int)
			int m_intValue;
		};
	};

	//\brief Shaders will compile and link from source upon creation
    Shader(const char * a_name, const char * a_vertexSource, const char * a_fragmentSource) 
	{
		// Quick sanity check on inputs
		if (a_name == NULL		|| a_vertexSource == NULL		|| a_fragmentSource == NULL ||
			a_name[0] == '\0'	|| a_vertexSource[0] == '\0'	|| a_fragmentSource[0] == '\0')
		{
			return;
		}

		// Set data and compile
		m_name[0] = '\0';
		strncpy(m_name, a_name, sizeof(char) * strlen(a_name) + 1);
		m_vertexShader = Compile(GL_VERTEX_SHADER, a_vertexSource);
		m_fragmentShader = Compile(GL_FRAGMENT_SHADER, a_fragmentSource);
        
		// Only proceed if the compilation of both shader components was successful
		if (m_vertexShader + m_fragmentShader > 1)
		{
			m_shader = glCreateProgram();
			glAttachShader(m_shader, m_vertexShader);
			glAttachShader(m_shader, m_fragmentShader);
			glLinkProgram(m_shader);

			// Set up the standard uniforms
			m_texture.Init(m_shader, "tex");
			m_time.Init(m_shader, "time");
			m_frameTime.Init(m_shader, "frameTime");
		}
    }

	//\brief Accessors and mutators
	inline GLuint GetShader() { return m_shader; }
	inline const char * GetName() { return &m_name[0]; }
	
	//\brief Bind the shader and setup the uniforms
	inline void UseShader() 
	{ 
		glUseProgram(m_shader);
		
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(m_texture.m_id, 0);
		glUniform1f(m_time.m_id, 1.0f);				// TODO Use real values
		glUniform1f(m_frameTime.m_id, 0.03f);		// TODO Use real values
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
	Uniform m_frameTime;
	Uniform m_time;
};

#endif // _ENGINE_RENDER_MANAGER
