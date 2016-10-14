#include "GL/CAPI_GLE.h"
#include "OVR_CAPI_GL.h"

#include "Shader.h"

#include "Log.h"

// The size the light is drawn in the in game editor
const float Light::s_lightDrawSize = 0.1f;		

// Light data is written to shader in an array of floats
float Shader::s_lightingData[Shader::s_numLightFloats];

Shader::Shader(const char * a_name)
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

Shader::~Shader() 
{
	glDeleteProgram(m_shader);
	glDeleteShader(m_vertexShader);
	glDeleteShader(m_fragmentShader);
}

bool Shader::Init(const char * a_vertexSource, const char * a_fragmentSource)
{
	if (m_name == NULL || a_vertexSource == NULL || a_fragmentSource == NULL ||
		m_name[0] == '\0' || a_vertexSource[0] == '\0' || a_fragmentSource[0] == '\0')
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
		m_diffuseTexture.Init(m_shader, "DiffuseTexture");
		m_normalTexture.Init(m_shader, "NormalTexture");
		m_specularTexture.Init(m_shader, "SpecularTexture");
		m_time.Init(m_shader, "Time");
		m_lifeTime.Init(m_shader, "LifeTime");
		m_frameTime.Init(m_shader, "FrameTime");
		m_viewWidth.Init(m_shader, "ViewWidth");
		m_viewHeight.Init(m_shader, "ViewHeight");
		m_shaderData.Init(m_shader, "ShaderData");
		m_objectMatrix.Init(m_shader, "ObjectMatrix");
		m_viewMatrix.Init(m_shader, "ViewMatrix");
		m_projectionMatrix.Init(m_shader, "ProjectionMatrix");
		m_lights.Init(m_shader, "Lights");
		return true;
	}
	return false;
}

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

void Shader::UseShader() 
{ 
	glUseProgram(m_shader); 
	glActiveTexture(GL_TEXTURE0); 
}

void Shader::UseShader(const UniformData & a_data)
{
	// Write the lighting data into the static memory location for the shader to reference
	int lightValCount = 0;
	for (int i = 0; i < s_maxLights; ++i)
	{
		// Enabled
		s_lightingData[lightValCount++] = a_data.m_lights[i].m_enabled ? 1.0f : 0.0f;
		// Position
		s_lightingData[lightValCount++] = a_data.m_lights[i].m_pos.GetX();
		s_lightingData[lightValCount++] = a_data.m_lights[i].m_pos.GetY();
		s_lightingData[lightValCount++] = a_data.m_lights[i].m_pos.GetZ();
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
	glUseProgram(m_shader);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(m_diffuseTexture.m_id, 0);
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(m_normalTexture.m_id, 1);
	glActiveTexture(GL_TEXTURE2);
	glUniform1i(m_specularTexture.m_id, 2);
	glUniform1f(m_time.m_id, a_data.m_time);
	glUniform1f(m_lifeTime.m_id, a_data.m_lifeTime);
	glUniform1f(m_frameTime.m_id, a_data.m_frameTime);
	glUniform1f(m_viewWidth.m_id, a_data.m_viewWidth);
	glUniform1f(m_viewHeight.m_id, a_data.m_viewHeight);
	glUniform3f(m_shaderData.m_id, a_data.m_shaderData.GetX(), a_data.m_shaderData.GetY(), a_data.m_shaderData.GetZ());
	glUniform1fv(m_lights.m_id, s_maxLights * s_numLightFloats, &s_lightingData[0]);
	glUniformMatrix4fv(m_objectMatrix.m_id, 1, GL_TRUE, a_data.m_objectMatrix->GetValues());
	glUniformMatrix4fv(m_viewMatrix.m_id, 1, GL_TRUE, a_data.m_viewMatrix->GetValues());
	glUniformMatrix4fv(m_projectionMatrix.m_id, 1, GL_TRUE, a_data.m_projectionMatrix->GetValues());
}
