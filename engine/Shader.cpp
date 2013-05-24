#include "Shader.h"

#include "Log.h"

unsigned int Shader::Compile(GLuint type, const char * a_src)
{
	Log & log = Log::Get();
    GLint compiled;
	const char ** pSrc = (const char **)&a_src;
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, pSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        
	if (!compiled) 
	{
		const unsigned int maxErrorLength = 1024;
		GLint logErrorLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logErrorLength);
		char * compileError = NULL;
        if (compileError = (char *)malloc(logErrorLength))
		{
			glGetShaderInfoLog(shader, logErrorLength, &logErrorLength, &compileError[0]);
			if (type == GL_VERTEX_SHADER)
			{
				log.Write(Log::LL_ERROR, Log::LC_ENGINE, "Error compiling shader %s.vsh, compiler output follows:", m_name);
			}
			else if (type == GL_FRAGMENT_SHADER)
			{
				log.Write(Log::LL_ERROR, Log::LC_ENGINE, "Error compiling shader %s.fsh, compiler output follows:", m_name);
			}
			log.WriteEngineErrorNoParams(compileError);
		}
		free(compileError);
    
        return 0;
    }
    return shader;
}
