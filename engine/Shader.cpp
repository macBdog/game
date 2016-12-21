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
	glDetachShader(m_shader, m_vertexShader);
	glDetachShader(m_shader, m_fragmentShader);
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

	unsigned int glErrorEnum = glGetError();

	// Compile the shader components
	m_vertexShader = Compile(GL_VERTEX_SHADER, a_vertexSource);
	m_fragmentShader = Compile(GL_FRAGMENT_SHADER, a_fragmentSource);

	// Only proceed if the compilation of both shader components was successful
	if (m_vertexShader > 0 && m_fragmentShader > 0)
	{
		m_shader = glCreateProgram();
		glAttachShader(m_shader, m_vertexShader);
		glAttachShader(m_shader, m_fragmentShader);

		glBindAttribLocation(m_shader, 0, "VertexPosition");
		glBindAttribLocation(m_shader, 1, "VertexColour");
		glBindAttribLocation(m_shader, 2, "VertexUV");
		glBindAttribLocation(m_shader, 3, "VertexNormal");

		glLinkProgram(m_shader);

		// Set up the standard uniforms
		m_diffuseTexture.Init(m_shader, "DiffuseTexture");
		m_normalTexture.Init(m_shader, "NormalTexture");
		m_specularTexture.Init(m_shader, "SpecularTexture");
		m_gBuffer1.Init(m_shader, "GBuffer1");
		m_gBuffer2.Init(m_shader, "GBuffer2");
		m_gBuffer3.Init(m_shader, "GBuffer3");
		m_gBuffer4.Init(m_shader, "GBuffer4");
		m_gBuffer5.Init(m_shader, "GBuffer5");
		m_gBuffer6.Init(m_shader, "GBuffer6");
		m_depthBuffer.Init(m_shader, "DepthBuffer");
		m_materialShininess.Init(m_shader, "MaterialShininess");
		m_numActiveLights.Init(m_shader, "NumActiveLights");
		m_time.Init(m_shader, "Time");
		m_lifeTime.Init(m_shader, "LifeTime");
		m_frameTime.Init(m_shader, "FrameTime");
		m_viewWidth.Init(m_shader, "ViewWidth");
		m_viewHeight.Init(m_shader, "ViewHeight");
		m_materialAmbient.Init(m_shader, "MaterialAmbient");
		m_materialDiffuse.Init(m_shader, "MaterialDiffuse");
		m_materialSpecular.Init(m_shader, "MaterialSpecular");
		m_materialEmission.Init(m_shader, "MaterialEmission");
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
	int numActiveLights = 0;
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

		numActiveLights += a_data.m_lights[i].m_enabled ? 1 : 0;
	}
	glUseProgram(m_shader);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(m_diffuseTexture.m_id, 0);
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(m_normalTexture.m_id, 1);
	glActiveTexture(GL_TEXTURE2);
	glUniform1i(m_specularTexture.m_id, 2);
	
	glActiveTexture(GL_TEXTURE3);
	glUniform1i(m_gBuffer1.m_id, a_data.m_gBufferIds[0]);
	glActiveTexture(GL_TEXTURE4);
	glUniform1i(m_gBuffer2.m_id, a_data.m_gBufferIds[1]);
	glActiveTexture(GL_TEXTURE5);
	glUniform1i(m_gBuffer3.m_id, a_data.m_gBufferIds[2]);
	glActiveTexture(GL_TEXTURE6);
	glUniform1i(m_gBuffer4.m_id, a_data.m_gBufferIds[3]);
	glActiveTexture(GL_TEXTURE7);
	glUniform1i(m_gBuffer5.m_id, a_data.m_gBufferIds[4]);
	glActiveTexture(GL_TEXTURE8);
	glUniform1i(m_gBuffer6.m_id, a_data.m_gBufferIds[5]);
	glActiveTexture(GL_TEXTURE9);
	glUniform1i(m_depthBuffer.m_id, a_data.m_depthBuffer);

	glUniform1i(m_materialShininess.m_id, a_data.m_materialShininess);
	glUniform1i(m_numActiveLights.m_id, numActiveLights);
	glUniform1f(m_time.m_id, a_data.m_time);
	glUniform1f(m_lifeTime.m_id, a_data.m_lifeTime);
	glUniform1f(m_frameTime.m_id, a_data.m_frameTime);
	glUniform1f(m_viewWidth.m_id, a_data.m_viewWidth);
	glUniform1f(m_viewHeight.m_id, a_data.m_viewHeight);
	glUniform3f(m_materialAmbient.m_id, a_data.m_materialAmbient.GetX(), a_data.m_materialAmbient.GetY(), a_data.m_materialAmbient.GetZ());
	glUniform3f(m_materialDiffuse.m_id, a_data.m_materialDiffuse.GetX(), a_data.m_materialDiffuse.GetY(), a_data.m_materialDiffuse.GetZ());
	glUniform3f(m_materialSpecular.m_id, a_data.m_materialSpecular.GetX(), a_data.m_materialSpecular.GetY(), a_data.m_materialSpecular.GetZ());
	glUniform3f(m_materialEmission.m_id, a_data.m_materialEmission.GetX(), a_data.m_materialEmission.GetY(), a_data.m_materialEmission.GetZ());
	glUniform3f(m_shaderData.m_id, a_data.m_shaderData.GetX(), a_data.m_shaderData.GetY(), a_data.m_shaderData.GetZ());
	glUniform1fv(m_lights.m_id, s_maxLights * s_numLightFloats, &s_lightingData[0]);
	glUniformMatrix4fv(m_objectMatrix.m_id, 1, GL_TRUE, a_data.m_objectMatrix->GetValues());
	glUniformMatrix4fv(m_viewMatrix.m_id, 1, GL_TRUE, a_data.m_viewMatrix->GetValues());
	glUniformMatrix4fv(m_projectionMatrix.m_id, 1, GL_TRUE, a_data.m_projectionMatrix->GetValues());
}
