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

	//\Brief Shaders will compile and link from source upon creation
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
        m_shader = glCreateProgram();
        glAttachShader(m_shader, m_vertexShader);
        glAttachShader(m_shader, m_fragmentShader);
        glLinkProgram(m_shader);
    }

	//\brief Accessors and mutators
	inline GLuint GetShader() { return m_shader; }
	inline void BindShader() { glUseProgram(m_shader); }
	inline const char * GetName() { return &m_name[0]; }

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
};

#endif // _ENGINE_RENDER_MANAGER
