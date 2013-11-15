#include "Shader.h"

#include "Log.h"

unsigned int Shader::Compile(GLuint type, const char * a_src)
{
    GLint compiled;
	const char ** pSrc = (const char **)&a_src;
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, pSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    
	// Print and log the shader compile error
	if (compiled == GL_FALSE) 
	{
		Log & log = Log::Get();
		GLint logErrorLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logErrorLength);
		char * compileError = NULL;
        if (compileError = (char *)malloc(logErrorLength))
		{
			glGetShaderInfoLog(shader, logErrorLength, &logErrorLength, compileError);

			if (type == GL_VERTEX_SHADER)
			{
				log.Write(LogLevel::Error, LogCategory::Engine, "Error compiling shader %s.vsh, compiler output follows:", m_name);
			}
			else if (type == GL_FRAGMENT_SHADER)
			{
				log.Write(LogLevel::Error, LogCategory::Engine, "Error compiling shader %s.fsh, compiler output follows:", m_name);
			}
			log.WriteEngineErrorNoParams(compileError);
			free(compileError);
		}
		
        return 0;
    }
    return shader;
}
