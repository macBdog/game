#ifndef _ENGINE_SHADER_
#define _ENGINE_SHADER_
#pragma once

#include <GL/glew.h>

#include "StringUtils.h"

//\brief Abstraction of a simple GLSL shader framework
class Shader
{
public:

	//\Brief Shaders will compile and link from source upon creation
    Shader(const char * a_vertexSource, const char * a_fragmentSource) 
	{
        m_vertexShader = Compile(GL_VERTEX_SHADER, a_vertexSource);
        m_fragmentShader = Compile(GL_FRAGMENT_SHADER, a_fragmentSource);
        m_shader = glCreateProgram();
        glAttachShader(m_shader, m_vertexShader);
        glAttachShader(m_shader, m_fragmentShader);
        glLinkProgram(m_shader);
    }

	inline GLuint GetShader() { return m_shader; }
	inline void BindShader() { glUseProgram(m_shader); }

    ~Shader() {
        glDeleteProgram(m_shader);
        glDeleteShader(m_vertexShader);
        glDeleteShader(m_fragmentShader);
    }

private:

	//\brief Compile the shader given the source code
    unsigned int Compile(GLuint type, const char * a_src);
	
	GLuint m_vertexShader;			///< Program to transform vertices
	GLuint m_fragmentShader;		///< Pixel shader
	GLuint m_shader;				///< Linked program
};

#endif // _ENGINE_RENDER_MANAGER
