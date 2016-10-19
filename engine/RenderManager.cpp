#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>

#include <windows.h>

#include "GL/CAPI_GLE.h"
#include "OVR_CAPI_GL.h"

#include "../core/MathUtils.h"

#include "CameraManager.h"
#include "DataPack.h"
#include "DebugMenu.h"
#include "Log.h"
#include "Texture.h"
#include "WorldManager.h"

#include "RenderManager.h"

using namespace std;	//< For fstream operations

template<> RenderManager * Singleton<RenderManager>::s_instance = NULL;
const float RenderManager::s_updateFreq = 1.0f;
const float RenderManager::s_nearClipPlane = 0.01f;
const float RenderManager::s_farClipPlane = 10000.0f;
const float RenderManager::s_fovAngleY = 55.0f;
float RenderManager::s_renderDepth2D = -1.0f;

bool RenderManager::Startup(const Colour & a_clearColour, const char * a_shaderPath, const DataPack * a_dataPack, bool a_vr)
{
    // Set the clear colour
    m_clearColour = a_clearColour;

	// Enable texture mapping
	glEnable(GL_TEXTURE_2D);

    // Enable smooth shading
    glShadeModel(GL_SMOOTH);

    // Set the clear color
    glClearColor(m_clearColour.GetR(), m_clearColour.GetG(), m_clearColour.GetB(), m_clearColour.GetA());

    // Depth buffer setup
    glClearDepth(1.0f);

	// Used for debug render mode
	glLineWidth(2.0f);
	glPointSize(2.0f);
	glEnable(GL_LINE_SMOOTH);

    // Depth testing and alpha blending
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // The Type Of Depth Test To Do
    glDepthFunc(GL_LEQUAL);

    // Really Nice Perspective Calculations
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
	glEnable(GL_COLOR_MATERIAL);

	// Storage for all the primitives
	bool renderLayerAlloc = true;
	for (unsigned int i = 0; i < RenderLayer::Count; ++i)
	{
		// Tris
		m_tris[i] = (Tri *)malloc(sizeof(Tri) * s_maxPrimitivesPerrenderLayer);
		renderLayerAlloc &= m_tris[i] != NULL;
		m_triCount[i] = 0;

		// Quads
		m_quads[i] = (Quad *)malloc(sizeof(Quad) * s_maxPrimitivesPerrenderLayer);
		renderLayerAlloc &= m_quads[i] != NULL;
		m_quadCount[i] = 0;

		// Lines
		m_lines[i] = (Line *)malloc(sizeof(Line) * s_maxPrimitivesPerrenderLayer);
		renderLayerAlloc &= m_lines[i] != NULL;
		m_lineCount[i] = 0;

		// Render models
		m_models[i] = (RenderModel *)malloc(sizeof(RenderModel) * s_maxPrimitivesPerrenderLayer);
		renderLayerAlloc &= m_models[i] != NULL;
		m_modelCount[i] = 0;

		// Font characters
		m_fontChars[i] = (FontChar *)malloc(sizeof(FontChar) * s_maxPrimitivesPerrenderLayer);
		renderLayerAlloc &= m_fontChars[i] != NULL;
		m_fontCharCount[i] = 0;
	}

	// Alert if memory allocation failed
	if (!renderLayerAlloc)
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "RenderManager failed to allocate renderLayer/primitive memory!");
	}

	// Setup default shaders
	#include "Shaders\colour.vsh.inc"
	#include "Shaders\colour.fsh.inc"
	#include "Shaders\texture.vsh.inc"
	#include "Shaders\texture.fsh.inc"
	#include "Shaders\lighting.vsh.inc"
	#include "Shaders\lighting.fsh.inc"
	if (m_colourShader = new Shader("colour"))
	{
		m_colourShader->Init(colourVertexShader, colourFragmentShader);
	}
	if (m_textureShader = new Shader("texture"))
	{
		m_textureShader->Init(textureVertexShader, textureFragmentShader);
	}
	if (m_lightingShader = new Shader("lighting"))
	{
		m_lightingShader->Init(lightingVertexShader, lightingFragmentShader);
	}

	// Cache off the shader path
	if (a_shaderPath != NULL && a_shaderPath[0] != '\0')
	{
		strncpy(m_shaderPath, a_shaderPath, sizeof(char) * strlen(a_shaderPath) + 1);
	}

	// Load all shaders from the datapack if specifiied
	if (a_dataPack != NULL && a_dataPack->IsLoaded())
	{
		// Construct paths for both shader file names
		char shaderNameBuf[StringUtils::s_maxCharsPerName];
		char fragShaderPathBuf[StringUtils::s_maxCharsPerLine];

		// Start by getting all the vertex shaders from the data pack
		DataPack::EntryList shaderEntries;
		a_dataPack->GetAllEntries(".vsh", shaderEntries);
		DataPack::EntryNode * curNode = shaderEntries.GetHead();
		while (curNode != NULL)
		{
			DataPackEntry * curEntry = curNode->GetData();
			sprintf(shaderNameBuf, "%s", StringUtils::ExtractFileNameFromPath(curEntry->m_path));
			const char * shaderNameEnd = strstr(shaderNameBuf, ".");
			if (shaderNameEnd != NULL)
			{
				const int shaderNameLen = strlen(shaderNameBuf) - strlen(shaderNameEnd);
				shaderNameBuf[shaderNameLen] = '\0';
			}
			if (Shader * pNewShader = new Shader(shaderNameBuf))
			{
				bool shaderLoadSuccess = false;

				// Now look for the fragment shader matching the vertex
				strncpy(fragShaderPathBuf, curEntry->m_path, strlen(curEntry->m_path) + 1);
				char * extension = strstr(fragShaderPathBuf, ".vsh");
				if (extension != NULL)
				{
					const int extensionPos = strlen(fragShaderPathBuf) - strlen(extension);
					fragShaderPathBuf[extensionPos + 1] = 'f';
				}
				if (DataPackEntry * fragmentNode = a_dataPack->GetEntry(fragShaderPathBuf))
				{
					char * vertexSource = (char *)malloc((curEntry->m_size + 1) * sizeof(char));
					char * fragmentSource = (char *)malloc((fragmentNode->m_size + 1) * sizeof(char));
					if (vertexSource != NULL && fragmentSource != NULL)
					{
						memcpy(vertexSource, curEntry->m_data, curEntry->m_size);
						memcpy(fragmentSource, fragmentNode->m_data, fragmentNode->m_size);
						vertexSource[curEntry->m_size] = '\0';
						fragmentSource[fragmentNode->m_size] = '\0';
						shaderLoadSuccess = RenderManager::InitShaderFromMemory(vertexSource, fragmentSource, *pNewShader);
					}
					free(vertexSource);
					free(fragmentSource);
				}

				if (shaderLoadSuccess)
				{
					LinkedListNode<Shader> * shaderNode = new LinkedListNode<Shader>();
					shaderNode->SetData(pNewShader);
					m_shaders.Insert(shaderNode);
				}
				else // Compile error will be reported in the log
				{
					delete pNewShader;
				}
			}
			curNode = curNode->GetNext();
		}
		a_dataPack->CleanupEntryList(shaderEntries);
	}

	// Set flag to enable the vr rendering to be wedged in
	m_vr = a_vr;

    return renderLayerAlloc && 
			m_colourShader != NULL && 
			m_textureShader != NULL && 
			m_lightingShader != NULL &&
			m_colourShader->IsCompiled() && 
			m_textureShader->IsCompiled() && 
			m_lightingShader->IsCompiled();
}

bool RenderManager::Shutdown()
{
	// Clean up storage for all primitives
	for (unsigned int i = 0; i < RenderLayer::Count; ++i)
	{
		free(m_tris[i]);
		free(m_quads[i]);
		free(m_lines[i]);
		free(m_models[i]);
		free(m_fontChars[i]);
		m_triCount[i] = 0;
		m_quadCount[i] = 0;
		m_lineCount[i] = 0;
		m_modelCount[i] = 0;
		m_fontCharCount[i] = 0;
	}
	
	// Clean up the default shaders
	if (m_colourShader != NULL)
	{
		delete m_colourShader;
	}
	if (m_textureShader != NULL)
	{
		delete m_textureShader;
	}

	// Clean up the framebuffer resources
	for (int i = 0; i < RenderStage::Count; ++i)
	{
		glDeleteFramebuffers(1, &m_frameBuffers[i]);
		glDeleteTextures(1, &m_colourBuffers[i]);
		glDeleteRenderbuffers(1, &m_depthBuffers[i]);   
	}

	// And any managed shaders
	ManagedShaderNode * next = m_managedShaders.GetHead();
	while (next != NULL)
	{
		// Cache off next pointer
		ManagedShaderNode * cur = next;
		next = cur->GetNext();

		m_managedShaders.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	// And finally the shaders
	LinkedListNode<Shader> * nextShader = m_shaders.GetHead();
	while (nextShader != NULL)
	{
		// Cache off next pointer
		LinkedListNode<Shader> * cur = nextShader;
		nextShader = cur->GetNext();

		Shader* curShader = cur->GetData();
		m_shaders.Remove(cur);
		delete curShader;
		delete cur;
	}
	return true;
}

bool RenderManager::Update(float a_dt)
{
	m_lastRenderTime = a_dt;
	m_renderTime += a_dt;

#ifdef _RELEASE
	return true;
#endif

	DataPack & dataPack = DataPack::Get();
	if (dataPack.IsLoaded())
	{
		return true;
	}

	if (m_updateTimer < m_updateFreq)
	{
		m_updateTimer += a_dt;
		return false;
	}
	else // Due for an update, scan all shaders
	{
		m_updateTimer = 0.0f;
		bool shadersReloaded = false;

		// Test all shaders for modification
		ManagedShaderNode * next = m_managedShaders.GetHead();
		while (next != NULL)
		{
			bool curFragShaderReloaded = false;
			bool curVertShaderReloaded = false;
			FileManager::Timestamp curTimeStampFrag;
			FileManager::Timestamp curTimeStampVert;
			ManagedShader * curManShader = next->GetData();
			char fullShaderPath[StringUtils::s_maxCharsPerLine];
			Shader * pShader = curManShader->m_shaderObject != NULL ? curManShader->m_shaderObject->GetShader() : curManShader->m_shaderScene->GetShader();
			
			// Game object or scene owning this shader has passed on to the other world
			if (pShader == NULL)
			{
				// Cache off next node
				ManagedShaderNode * actualNext = next->GetNext();
				m_managedShaders.Remove(next);
				delete next->GetData();
				delete next;
				next = actualNext;
			}
			else
			{
				sprintf(fullShaderPath, "%s%s.vsh", RenderManager::Get().GetShaderPath(), pShader->GetName());
				FileManager::Get().GetFileTimeStamp(fullShaderPath, curTimeStampVert);
				if (curTimeStampVert > curManShader->m_vertexTimeStamp)
				{
					curVertShaderReloaded = true;
					curManShader->m_vertexTimeStamp = curTimeStampVert;
					Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in shader %s, reloading.", fullShaderPath);
				}

				// Now check the pixel shader
				sprintf(fullShaderPath, "%s%s.fsh", RenderManager::Get().GetShaderPath(), pShader->GetName());
				FileManager::Get().GetFileTimeStamp(fullShaderPath, curTimeStampFrag);
				if (curTimeStampFrag > curManShader->m_fragmentTimeStamp)
				{
					curFragShaderReloaded = true;
					curManShader->m_fragmentTimeStamp = curTimeStampFrag;
					Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in shader %s, reloading.", fullShaderPath);
				}

				if (curVertShaderReloaded || curFragShaderReloaded)
				{
					// Recreate the shader
					shadersReloaded = true;
					if (Shader * pReloadedShader = new Shader(pShader->GetName()))
					{
						if (InitShaderFromFile(*pReloadedShader))
						{
							// Reassign shader to source object
							if (curManShader->m_shaderObject != NULL)
							{
								curManShader->m_shaderObject->SetShader(pReloadedShader);
							}
							else if (curManShader->m_shaderScene != NULL)
							{
								curManShader->m_shaderScene->SetShader(pReloadedShader);
							}

							// Reassign the shader for any other managed objects that shared the reloaded shader
							ManagedShaderNode * sharedShader = m_managedShaders.GetHead();
							while (sharedShader != NULL)
							{
								// First check for game object owning shader
								Shader * existingShader = NULL;
								if (sharedShader->GetData()->m_shaderObject != NULL)
								{
									existingShader = sharedShader->GetData()->m_shaderObject->GetShader();
									if (existingShader != NULL && existingShader == pShader)
									{
										sharedShader->GetData()->m_shaderObject->SetShader(pReloadedShader);
										
										// Reset timestamps for shared shader resources
										if (curFragShaderReloaded)
										{
											sharedShader->GetData()->m_fragmentTimeStamp = curTimeStampFrag;
										}
										if (curVertShaderReloaded)
										{
											sharedShader->GetData()->m_vertexTimeStamp = curTimeStampVert;
										}
									}
								}
								// Now check for scene owning shader
								else if (sharedShader->GetData()->m_shaderScene != NULL)
								{
									existingShader = sharedShader->GetData()->m_shaderScene->GetShader();
									if (existingShader != NULL && existingShader == pShader)
									{
										sharedShader->GetData()->m_shaderScene->SetShader(pReloadedShader);

										// Reset timestamps for shared shader resources
										if (curFragShaderReloaded)
										{
											sharedShader->GetData()->m_fragmentTimeStamp = curTimeStampFrag;
										}
										if (curVertShaderReloaded)
										{
											sharedShader->GetData()->m_vertexTimeStamp = curTimeStampVert;
										}
									}
								}
								sharedShader = sharedShader->GetNext();
							}

							// Finally safe to delete deprecated shader
							delete pShader;
						}
					}
				}
				// Advance through the list
				next = next->GetNext();
			}
		}
		return shadersReloaded;
	}
	return false;
}

bool RenderManager::Resize(unsigned int a_viewWidth, unsigned int a_viewHeight, unsigned int a_viewBpp, bool a_fullScreen)
{
    // Setup viewport ratio avoiding a divide by zero
    if (a_viewHeight == 0)
	{
		a_viewHeight = 1;
	}

	m_viewWidth = a_viewWidth;
	m_viewHeight = a_viewHeight;
	m_aspect = (float)m_viewWidth / (float)m_viewHeight;

    // Setup our viewport
    glViewport(0, 0, (GLint)a_viewWidth, (GLint)a_viewHeight);
    glLoadIdentity();

	// Create the framebuffers for each render stage
	bool framebuffersOk = true;
	for (int i = 0; i < RenderStage::Count; ++i)
	{
		// Generate whole scene render targets
		glGenFramebuffers(1, &m_frameBuffers[i]);
		glGenTextures(1, &m_colourBuffers[i]);                                                                                               
		glGenRenderbuffers(1, &m_depthBuffers[i]);
		glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffers[i]);

		// Colour parameters
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_colourBuffers[i]);
 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_viewWidth, m_viewHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colourBuffers[i], 0);

		// Depth parameters
		glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffers[i]);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_viewWidth, m_viewHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffers[i]);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);	
		framebuffersOk |= glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}

	return framebuffersOk;
}

void RenderManager::DrawToScreen(Matrix & a_viewMatrix)
{
	// Do offscreen rendering pass to first stage framebuffer
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);         
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffers[RenderStage::Scene]);		//<< Render to first stage buffer

	RenderScene(a_viewMatrix);
	
	// Start rendering to the first render stage
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_colourBuffers[RenderStage::Scene]);			//<< Render using first stage buffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffers[RenderStage::PostFX]);		//<< Render to first stage colour

	glLoadIdentity();
	glViewport(0, 0, (GLint)m_viewWidth, (GLint)m_viewHeight);
	glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Now render with full scene shader if specified
	Matrix identityMat = Matrix::Identity();
	Matrix orthoMat = Matrix::Orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f);
	Shader::UniformData shaderData(m_renderTime, m_renderTime, m_lastRenderTime, (float)m_viewWidth, (float)m_viewHeight, Vector::Zero(), &identityMat, &identityMat, &orthoMat);
	bool bUseDefaultShader = true;
	if (Scene * pCurScene = WorldManager::Get().GetCurrentScene())
	{
		if (Shader * pSceneShader = pCurScene->GetShader())
		{
			pSceneShader->UseShader(shaderData);
			bUseDefaultShader = false;
		}
	}

#ifndef _RELEASE
	// Turn off full scene shaders for editing
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		bUseDefaultShader = true;
	}
#endif

	// Otherwise use the default
	if (bUseDefaultShader)
	{	
		m_textureShader->UseShader(shaderData);
	}

	RenderFramebuffer();

	// Now draw framebuffer to screen, buffer index 0 breaks the existing binding
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, (GLint)m_viewWidth, (GLint)m_viewHeight);
	glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Final drawing pass
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_colourBuffers[RenderStage::PostFX]);
	
	// Render once 
	m_textureShader->UseShader(shaderData);
	RenderFramebuffer();
	
	// Unbind shader
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
}

void RenderManager::RenderScene(Matrix & a_viewMatrix, bool a_flushBuffers)
{
	Matrix perspectiveMatrix = Matrix::Perspective(s_fovAngleY, 1.33334f /*m_aspect*/, s_nearClipPlane, s_farClipPlane);
	RenderScene(a_viewMatrix, perspectiveMatrix, a_flushBuffers, true);
}

void RenderManager::RenderScene(Matrix & a_viewMatrix, Matrix & a_perspectiveMat, bool a_flushBuffers, bool a_clear)
{
	// Setup fresh data to pass to shaders
	Matrix identityMatrix = Matrix::Identity();
	Matrix orthoMatrix = Matrix::Orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f);
	Shader::UniformData shaderData(m_renderTime, 0.0f, m_lastRenderTime, (float)m_viewWidth, (float)m_viewHeight, Vector::Zero(), &identityMatrix, &a_viewMatrix, &a_perspectiveMat);
	
	// Set the lights in the scene for the shader
	Scene * curScene = WorldManager::Get().GetCurrentScene();
	if (curScene->HasLights())
	{
		for (int i = 0; i < curScene->GetNumLights(); ++i)
		{
			const Light & light = curScene->GetLight(i);
			shaderData.m_lights[i] = light;
		}
	}

	// Handle different rendering modes
	switch (m_renderMode)
	{
		case RenderMode::None:
		{
			// Clear the queues as the rest of the system will continue to add primitives
			for (unsigned int i = 0; i < RenderLayer::Count; ++i)
			{
				m_triCount[i] = 0;
				m_quadCount[i] = 0;
				m_lineCount[i] = 0;
				m_modelCount[i] = 0;
				m_fontCharCount[i] = 0;
			}
			return;
		}
		case RenderMode::Wireframe:
		{
			// TODO
		}
		default: break;
	}

	if (a_clear)
	{
		// Set viewports to match render target
		glViewport(0, 0, (GLint)m_viewWidth, (GLint)m_viewHeight);

		// Clear the color and depth buffers in preparation for drawing
		glClearColor(m_clearColour.GetR(), m_clearColour.GetG(), m_clearColour.GetB(), m_clearColour.GetA());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// Draw primitives for each renderLayer
	for (unsigned int i = 0; i < RenderLayer::Count; ++i)
	{
		// Switch render mode for each renderLayer
		switch ((RenderLayer::Enum)i)
		{
			case RenderLayer::World:
#ifndef _RELEASE
			case RenderLayer::Debug3D:
#endif
			{
				// Setup projection transformation matrices for the shaders
				shaderData.m_projectionMatrix = &a_perspectiveMat;
				shaderData.m_viewMatrix = &a_viewMatrix;
				break;
			}
#ifndef _RELEASE
			case RenderLayer::Debug2D:
#endif
			case RenderLayer::Gui:
			{
				if (m_vr)
				{
					Matrix guiTransform = identityMatrix;
					Quaternion guiRotation = Quaternion(Vector(0.05f, 0.0f, 0.0f));
					guiRotation.ApplyToMatrix(guiTransform);
					shaderData.m_projectionMatrix = &a_perspectiveMat;
					shaderData.m_viewMatrix = &guiTransform;
				}
				else
				{
					shaderData.m_projectionMatrix = &orthoMatrix;
					shaderData.m_viewMatrix = &identityMatrix;
				}
				break;
			}
			default: break;
		}

		// Ensure 2D renderLayeres render on top of everything
		if ((RenderLayer::Enum)i == RenderLayer::Gui)
		{
			glClear(GL_DEPTH_BUFFER_BIT);
		}

		// Use the texture shader on world objects
		glEnable(GL_TEXTURE_2D);
		Shader * pLastShader = m_textureShader;

		// Submit the tris
		Tri * t = m_tris[i];
		for (unsigned int j = 0; j < m_triCount[i]; ++j)
		{
			glColor4f(t->m_colour.GetR(), t->m_colour.GetG(), t->m_colour.GetB(), t->m_colour.GetA());

			// Draw a tri with a texture
			if (t->m_textureId >= 0)
			{
				if (pLastShader != m_textureShader)
				{
					glEnable(GL_TEXTURE_2D);
					pLastShader = m_textureShader;
				}
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, t->m_textureId);
			}
			else // Flat colour triangles
			{
				if (pLastShader != m_colourShader)
				{
					glDisable(GL_TEXTURE_2D);
					pLastShader = m_colourShader;
				}
			}
			
			pLastShader->UseShader(shaderData);

			glBegin(GL_TRIANGLES);

			glTexCoord2f(t->m_coords[0].GetX(), t->m_coords[0].GetY()); 
			glVertex3f(t->m_verts[0].GetX(), t->m_verts[0].GetY(), t->m_verts[0].GetZ());

			glTexCoord2f(t->m_coords[1].GetX(), t->m_coords[1].GetY()); 
			glVertex3f(t->m_verts[1].GetX(), t->m_verts[1].GetY(), t->m_verts[1].GetZ());

			glTexCoord2f(t->m_coords[2].GetX(), t->m_coords[2].GetY()); 
			glVertex3f(t->m_verts[2].GetX(), t->m_verts[2].GetY(), t->m_verts[2].GetZ());

			glEnd();
			
			t++;
		}

		// Submit the quad
		Quad * q = m_quads[i];
		shaderData.m_objectMatrix = &identityMatrix;
		for (unsigned int j = 0; j < m_quadCount[i]; ++j)
		{
			glColor4f(q->m_colour.GetR(), q->m_colour.GetG(), q->m_colour.GetB(), q->m_colour.GetA());

			// Draw a quad with a texture
			if (q->m_textureId >= 0)
			{
				if (pLastShader != m_textureShader)
				{
					glEnable(GL_TEXTURE_2D);
					pLastShader = m_textureShader;
				}
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, q->m_textureId);
			}
			else // Flat colour quads
			{
				if (pLastShader != m_colourShader)
				{
					glDisable(GL_TEXTURE_2D);
					pLastShader = m_colourShader;
				}
			}

			pLastShader->UseShader(shaderData);
	
			glBegin(GL_QUADS);

			glTexCoord2f(q->m_coords[0].GetX(), q->m_coords[0].GetY()); 
			glVertex3f(q->m_verts[0].GetX(), q->m_verts[0].GetY(), q->m_verts[0].GetZ());

			glTexCoord2f(q->m_coords[1].GetX(), q->m_coords[1].GetY()); 
			glVertex3f(q->m_verts[1].GetX(), q->m_verts[1].GetY(), q->m_verts[1].GetZ());

			glTexCoord2f(q->m_coords[2].GetX(), q->m_coords[2].GetY()); 
			glVertex3f(q->m_verts[2].GetX(), q->m_verts[2].GetY(), q->m_verts[2].GetZ());

			glTexCoord2f(q->m_coords[3].GetX(), q->m_coords[3].GetY()); 
			glVertex3f(q->m_verts[3].GetX(), q->m_verts[3].GetY(), q->m_verts[3].GetZ());

			glEnd();
			
			q++;
		}

		// Draw font chars by calling their display lists
		if (pLastShader != m_textureShader && m_fontCharCount[i] > 0)
		{
			glEnable(GL_TEXTURE_2D);
			pLastShader = m_textureShader;
		}

		FontChar * fc = m_fontChars[i];
		Matrix fontCharMat = Matrix::Identity();
		for (unsigned int j = 0; j < m_fontCharCount[i]; ++j)
		{
			fontCharMat.SetIdentity();
			fontCharMat.SetPos(fc->m_pos);
			fontCharMat.SetScale(Vector(fc->m_size.GetX(), fc->m_size.GetY(), 1.0f));
			shaderData.m_objectMatrix = &fontCharMat;
			pLastShader->UseShader(shaderData);
			glColor4f(fc->m_colour.GetR(), fc->m_colour.GetG(), fc->m_colour.GetB(), fc->m_colour.GetA());
			glCallList(fc->m_displayListId);
			++fc;
		}

		// Draw models by calling their display lists
		RenderModel * rm = m_models[i];
		Shader * pLastModelShader = NULL;	
		for (unsigned int j = 0; j < m_modelCount[i]; ++j)
		{
			// Draw each object of each model with it's own material
			for (unsigned int k = 0; k < rm->m_model->GetNumObjects(); ++k)
			{
				Object * obj = rm->m_model->GetObjectAtIndex(k);
				pLastModelShader = rm->m_shader == NULL ? m_textureShader : rm->m_shader;
				shaderData.m_projectionMatrix = &a_perspectiveMat;
				shaderData.m_viewMatrix = &a_viewMatrix;
				shaderData.m_objectMatrix = rm->m_mat;
				shaderData.m_lifeTime = rm->m_lifeTime;
				shaderData.m_shaderData = rm->m_shaderData;
				pLastModelShader->UseShader(shaderData);
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, obj->GetMaterial()->m_ambient.GetValues());
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, obj->GetMaterial()->m_diffuse.GetValues());
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, obj->GetMaterial()->m_specular.GetValues());
				glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, obj->GetMaterial()->m_emission.GetValues());
				glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, obj->GetMaterial()->m_shininess);
				glCallList(obj->GetDisplayListId());
			}
			++rm;
		}
		
		// Swith to colour shader for lines as they cannot be textured
		if (m_lineCount[i] > 0)
		{
			glDisable(GL_TEXTURE_2D);
			shaderData.m_objectMatrix = &identityMatrix;
			m_colourShader->UseShader(shaderData);
		}

		// Draw lines in the current renderLayer
		Line * l = m_lines[i];
		for (unsigned int j = 0; j < m_lineCount[i]; ++j)
		{
			glColor4f(l->m_colour.GetR(), l->m_colour.GetG(), l->m_colour.GetB(), l->m_colour.GetA());
			glBegin(GL_LINES);
			glVertex3f(l->m_verts[0].GetX(), l->m_verts[0].GetY(), l->m_verts[0].GetZ());
			glVertex3f(l->m_verts[1].GetX(), l->m_verts[1].GetY(), l->m_verts[1].GetZ());
			glEnd();
			++l;
		}
		
		// Flush the renderLayers if we are not rendering more than once
		if (a_flushBuffers)
		{
			m_triCount[i] = 0;
			m_quadCount[i] = 0;
			m_lineCount[i] = 0;
			m_modelCount[i] = 0;
			m_fontCharCount[i] = 0;
		}
	}
}

void RenderManager::RenderFramebuffer()
{	
	// Draw whole screen triangle pair
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0f, 0.0f);   glVertex3f(-1.0f, -1.0f, s_renderDepth2D);
		glTexCoord2f(1.0f, 0.0f);   glVertex3f(1.0f, -1.0f, s_renderDepth2D);
		glTexCoord2f(0.0f, 1.0f);   glVertex3f(-1.0f, 1.0f, s_renderDepth2D);
		glTexCoord2f(1.0f, 1.0f);   glVertex3f(1.0f, 1.0f, s_renderDepth2D);
	glEnd();
}

unsigned int RenderManager::RegisterFontChar(Vector2 a_size, TexCoord a_texCoord, TexCoord a_texSize, Texture * a_texture)
{
	// Generate and begin compiling a new display list
	GLuint displayListId = glGenLists(1);
	glNewList(displayListId, GL_COMPILE);
	
	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, a_texture->GetId());
	
	// Draw the verts
	glBegin(GL_QUADS);
	glTexCoord2f(a_texCoord.GetX(), 1.0f - a_texCoord.GetY()); 
	glVertex3f(0.0f, 0.0f, s_renderDepth2D);

	glTexCoord2f(a_texCoord.GetX() + a_texSize.GetX(),	1.0f - a_texCoord.GetY());
	glVertex3f(a_size.GetX(), 0.0f, s_renderDepth2D);

	glTexCoord2f(a_texCoord.GetX() + a_texSize.GetX(),	1.0f - a_texSize.GetY() - a_texCoord.GetY());
	glVertex3f(a_size.GetX(), -a_size.GetY(), s_renderDepth2D);

	glTexCoord2f(a_texCoord.GetX(),					1.0f - a_texSize.GetY() - a_texCoord.GetY());
	glVertex3f(0.0f, -a_size.GetY(), s_renderDepth2D);
	glEnd();

	glEndList();

	return displayListId;
}

unsigned int RenderManager::RegisterFontChar3D(Vector2 a_size, TexCoord a_texCoord, TexCoord a_texSize, Texture * a_texture)
{
	// Generate and begin compiling a new display list
	GLuint displayListId = glGenLists(1);
	glNewList(displayListId, GL_COMPILE);
	
	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, a_texture->GetId());
	
	// Draw the verts
	glBegin(GL_QUADS);
	glTexCoord2f(a_texCoord.GetX(), 1.0f - a_texCoord.GetY()); 
	glVertex3f(0.0f, 0.0f, a_size.GetY());

	glTexCoord2f(a_texCoord.GetX() + a_texSize.GetX(),	1.0f - a_texCoord.GetY());
	glVertex3f(a_size.GetX(), 0.0f, a_size.GetY());

	glTexCoord2f(a_texCoord.GetX() + a_texSize.GetX(),	1.0f - a_texSize.GetY() - a_texCoord.GetY());
	glVertex3f(a_size.GetX(), 0.0f, -a_size.GetY());

	glTexCoord2f(a_texCoord.GetX(),						1.0f - a_texSize.GetY() - a_texCoord.GetY());
	glVertex3f(0.0f, 0.0f, -a_size.GetY());
	glEnd();

	glEndList();

	return displayListId;
}

void RenderManager::AddLine2D(RenderLayer::Enum a_renderLayer, Vector2 a_point1, Vector2 a_point2, Colour a_tint)
{
	// Set the Z depth according to 2D project and call through to 3D version
	AddLine(a_renderLayer,
			Vector(a_point1.GetX(), a_point1.GetY(), s_renderDepth2D),
		    Vector(a_point2.GetX(), a_point2.GetY(), s_renderDepth2D),
			a_tint);
}

void RenderManager::AddLine(RenderLayer::Enum a_renderLayer, Vector a_point1, Vector a_point2, Colour a_tint)
{
	// Don't add more primitives than have been allocated for
	if (m_lineCount[a_renderLayer] >= s_maxPrimitivesPerrenderLayer)
	{
		Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "Too many line primitives added, max is %d", m_lineCount);
		return;
	}

	// Copy params to next queue item
	Line * l = m_lines[a_renderLayer];
	l += m_lineCount[a_renderLayer]++;

	l->m_colour = a_tint;
		
	// Setup verts 
	l->m_verts[0] = a_point1;
	l->m_verts[1] = a_point2;
}

void RenderManager::AddQuad2D(RenderLayer::Enum a_renderLayer, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, TextureOrientation::Enum a_orient, Colour a_tint)
{
	// Create a quad with tex coord at top left
	TexCoord texPos(0.0f, 0.0f);
	TexCoord texSize(1.0f, 1.0f);
	AddQuad2D(a_renderLayer, a_topLeft, a_size, a_tex, texPos, texSize, a_orient, a_tint);
}

void RenderManager::AddQuad2D(RenderLayer::Enum a_renderLayer, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, TexCoord a_texCoord, TexCoord a_texSize, TextureOrientation::Enum a_orient, Colour a_tint)
{
	Vector2 verts[4] =	{Vector2(a_topLeft.GetX(), a_topLeft.GetY()),
						Vector2(a_topLeft.GetX() + a_size.GetX(), a_topLeft.GetY()),
						Vector2(a_topLeft.GetX() + a_size.GetX(), a_topLeft.GetY() - a_size.GetY()),
						Vector2(a_topLeft.GetX(), a_topLeft.GetY() - a_size.GetY())};
	AddQuad2D(a_renderLayer, &verts[0], a_tex, a_texCoord, a_texSize, a_orient, a_tint);
}

void RenderManager::AddQuad2D(RenderLayer::Enum a_renderLayer, Vector2 * a_verts, Texture * a_tex, TexCoord a_texCoord, TexCoord a_texSize, TextureOrientation::Enum a_orient, Colour a_tint)
{
	// Don't add more primitives than have been allocated for
	if (m_quadCount[a_renderLayer] >= s_maxPrimitivesPerrenderLayer)
	{
		Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "Too many primitives added for renderLayer %d, max is %d", a_renderLayer, s_maxPrimitivesPerrenderLayer);
		return;
	}

	// Warn about no texture
	if (a_renderLayer != RenderLayer::Debug2D && a_tex == NULL)
	{
		Log::Get().WriteOnce(LogLevel::Warning, LogCategory::Engine, "Quad submitted without a texture");
	}

	// Copy params to next queue item
	Quad * q = m_quads[a_renderLayer];
	q += m_quadCount[a_renderLayer]++;
	if (a_tex)
	{
		q->m_textureId = a_tex->GetId();
	}
	else
	{
		q->m_textureId = -1;
	}
	q->m_colour = a_tint;
	
	// Setup verts for clockwise drawing 
	q->m_verts[0] = Vector(a_verts[0].GetX(), a_verts[0].GetY(), s_renderDepth2D);
	q->m_verts[1] = Vector(a_verts[1].GetX(), a_verts[1].GetY(), s_renderDepth2D);
	q->m_verts[2] = Vector(a_verts[2].GetX(), a_verts[2].GetY(), s_renderDepth2D);
	q->m_verts[3] = Vector(a_verts[3].GetX(), a_verts[3].GetY(), s_renderDepth2D);

	// Set texcoords based on orientation
	if (a_tex)
	{
		switch(a_orient)
		{
			case TextureOrientation::Normal:
			{
				q->m_coords[0] = TexCoord(a_texCoord.GetX(),					1.0f - a_texCoord.GetY());
				q->m_coords[1] = TexCoord(a_texCoord.GetX() + a_texSize.GetX(),	1.0f - a_texCoord.GetY());
				q->m_coords[2] = TexCoord(a_texCoord.GetX() + a_texSize.GetX(),	1.0f - a_texSize.GetY() - a_texCoord.GetY());
				q->m_coords[3] = TexCoord(a_texCoord.GetX(),					1.0f - a_texSize.GetY() - a_texCoord.GetY());
				break;
			}
			case TextureOrientation::FlipVert:
			{
				// TODO
				break;
			}
			case TextureOrientation::FlipHoriz:
			{
				// TODO
				break;
			}
			case TextureOrientation::FlipBoth:
			{
				// TODO
				break;
			}

			default: break;
		}
	}
}

void RenderManager::AddQuad3D(RenderLayer::Enum a_renderLayer, Vector * a_verts, Texture * a_tex, Colour a_tint)
{
	// Don't add more primitives than have been allocated for
	if (m_quadCount[a_renderLayer] >= s_maxPrimitivesPerrenderLayer)
	{
		Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "Too many primitives added for renderLayer %d, max is %d", a_renderLayer, s_maxPrimitivesPerrenderLayer);
		return;
	}

	// Warn about no texture
	if (a_renderLayer != RenderLayer::Debug2D && a_tex == NULL)
	{
		Log::Get().WriteOnce(LogLevel::Warning, LogCategory::Engine, "3D Quad submitted without a texture");
	}

	// Copy params to next queue item
	Quad * q = m_quads[a_renderLayer];
	q += m_quadCount[a_renderLayer]++;
	if (a_tex)
	{
		q->m_textureId = a_tex->GetId();
	}
	else
	{
		q->m_textureId = -1;
	}
	q->m_colour = a_tint;
	
	// Setup verts for clockwise drawing 
	q->m_verts[0] = a_verts[0];
	q->m_verts[1] = a_verts[1];
	q->m_verts[2] = a_verts[2];
	q->m_verts[3] = a_verts[3];

	// Only one tex coord style for now
	if (a_tex)
	{
		q->m_coords[0] = TexCoord(0.0f,	1.0f);
		q->m_coords[1] = TexCoord(1.0f,	1.0f);
		q->m_coords[2] = TexCoord(1.0f,	0.0f);
		q->m_coords[3] = TexCoord(0.0f,	0.0f);
	}
}

void RenderManager::AddTri(RenderLayer::Enum a_renderLayer, Vector a_point1, Vector a_point2, Vector a_point3, TexCoord a_txc1, TexCoord a_txc2, TexCoord a_txc3, Texture * a_tex, Colour a_tint)
{
	// Don't add more primitives than have been allocated for
	if (m_triCount[a_renderLayer] >= s_maxPrimitivesPerrenderLayer)
	{
		Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "Too many tri primitives added for renderLayer %d, max is %d", a_renderLayer, s_maxPrimitivesPerrenderLayer);
		return;
	}

	// Warn about no texture
	if (a_renderLayer != RenderLayer::Debug3D && a_tex == NULL)
	{
		Log::Get().WriteOnce(LogLevel::Warning, LogCategory::Engine, "Tri submitted without a texture");
	}

	// Copy params to next queue item
	Tri * t = m_tris[a_renderLayer];
	t += m_triCount[a_renderLayer]++;
	if (a_tex)
	{
		t->m_textureId = a_tex->GetId();
	}
	else
	{
		t->m_textureId = -1;
	}
	t->m_colour = a_tint;
	
	// Setup verts for clockwise drawing
	t->m_verts[0] = a_point1;
	t->m_verts[1] = a_point2;
	t->m_verts[2] = a_point3;
	
	// Set texcoords based on orientation
	t->m_coords[0] = a_txc1;
	t->m_coords[1] = a_txc2;
	t->m_coords[2] = a_txc3;
}

void RenderManager::AddModel(RenderLayer::Enum a_renderLayer, Model * a_model, Matrix * a_mat, Shader * a_shader, const Vector & a_shaderData, float a_lifeTime)
{
	// Don't add more models than have been allocated for
	if (m_modelCount[a_renderLayer] >= s_maxPrimitivesPerrenderLayer)
	{
		Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "Too many render models added for renderLayer %d, max is %d", a_renderLayer, s_maxPrimitivesPerrenderLayer);
		return;
	}

	// If we have not generated buffers for this model
	for (unsigned int i = 0; i < a_model->GetNumObjects(); ++i)
	{
		Object * obj = a_model->GetObjectAtIndex(i);

		// Check to see if the list has already been generated but needs to be torn down
		if (obj->GetDisplayListId() > 0 && !obj->IsDisplayListGenerated())
		{
			glDeleteLists(obj->GetDisplayListId(), 1);
		}

		// Now regenerate the list
		if (!obj->IsDisplayListGenerated())
		{
			// Alias model data
			Material * modelMat = obj->GetMaterial();
			if (!modelMat)
			{
				Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "No material loaded for model with name %s",  a_model->GetName());
				return;
			}
			unsigned int numVertices = obj->GetNumVertices();
			Texture * diffuseTex = modelMat->GetDiffuseTexture();
			Texture * normalTex = modelMat->GetNormalTexture();
			Texture * specularTex = modelMat->GetSpecularTexture();
			Vector * verts = obj->GetVertices();
			TexCoord * uvs = obj->GetUvs();

			GLuint displayListId = glGenLists(1);
			glNewList(displayListId, GL_COMPILE);

			// Bind the texture and set colour
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			if (diffuseTex != NULL)
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, diffuseTex->GetId());
			}
			if (normalTex != NULL)
			{
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, normalTex->GetId());
			}
			if (specularTex != NULL)
			{
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, specularTex->GetId());
			}
			else if (diffuseTex != NULL) // No spec map, bind the diffuse instead as a default
			{
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, diffuseTex->GetId());
			}
			glBegin(GL_TRIANGLES);
			// Draw vertices in threes
			for (unsigned int i = 0; i < numVertices; i += Model::s_vertsPerTri)
			{
				glTexCoord2f(uvs[i].GetX(), uvs[i].GetY()); 
				glVertex3f(verts[i].GetX(), verts[i].GetY(), verts[i].GetZ());

				glTexCoord2f(uvs[i+1].GetX(), uvs[i+1].GetY()); 
				glVertex3f(verts[i+1].GetX(), verts[i+1].GetY(), verts[i+1].GetZ());

				glTexCoord2f(uvs[i+2].GetX(), uvs[i+2].GetY()); 
				glVertex3f(verts[i+2].GetX(), verts[i+2].GetY(), verts[i+2].GetZ());
			}
			glEnd();
			glEndList();

			obj->SetDisplayListId(displayListId);
		}
	}

	RenderModel * r = m_models[a_renderLayer];
	r += m_modelCount[a_renderLayer]++;
	r->m_model = a_model;
	r->m_mat = a_mat;
	r->m_shader = a_shader;
	r->m_lifeTime = a_lifeTime;
	r->m_shaderData = a_shaderData;

	// Show the local matrix in debug mode
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		AddDebugMatrix(*a_mat);
	}

}

void RenderManager::AddFontChar(RenderLayer::Enum a_renderLayer, unsigned int a_fontCharId, const Vector2 & a_size, Vector a_pos, Colour a_colour)
{
	// Don't add more font characters than have been allocated for
	if (m_fontCharCount[a_renderLayer] >= s_maxPrimitivesPerrenderLayer)
	{
		Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "Too many font characters added for renderLayer %d, max is %d", a_renderLayer, s_maxPrimitivesPerrenderLayer);
		return;
	}

	FontChar * fc = m_fontChars[a_renderLayer];
	fc += m_fontCharCount[a_renderLayer]++;
	fc->m_displayListId = a_fontCharId;
	fc->m_size = a_size;
	fc->m_pos = a_pos;
	fc->m_colour = a_colour;
	fc->m_2d = a_renderLayer == RenderLayer::Gui || a_renderLayer == RenderLayer::Debug2D;
}

void RenderManager::AddDebugArrow(Vector a_start, Vector a_end, Colour a_tint)
{
	Vector arrowHead = (a_start - a_end);
	const float arrowHeadSize = arrowHead.LengthSquared() * 0.1f;
	arrowHead.Normalise();
	const Vector arrowPointX = a_end + (arrowHead * arrowHeadSize) + Vector(arrowHeadSize, 0.0f, 0.0f);
	const Vector arrowPointY = a_end + (arrowHead * arrowHeadSize) + Vector(0.0f, arrowHeadSize, 0.0f);
	const Vector arrowPointZ = a_end + (arrowHead * arrowHeadSize) + Vector(0.0f, 0.0f, arrowHeadSize);
	AddLine(RenderLayer::Debug3D, a_start, a_end, a_tint);
	AddLine(RenderLayer::Debug3D, a_end, arrowPointX, a_tint);
	AddLine(RenderLayer::Debug3D, a_end, arrowPointY, a_tint);
	AddLine(RenderLayer::Debug3D, a_end, arrowPointZ, a_tint);
}

void RenderManager::AddDebugArrow2D(Vector2 a_start, Vector2 a_end, Colour a_tint)
{
	Vector2 arrowHead = (a_start - a_end);
	const float arrowHeadSize = arrowHead.LengthSquared() * 0.1f;
	arrowHead.Normalise();
	const Vector2 arrowPointX = a_end + (arrowHead * arrowHeadSize) + Vector2(arrowHeadSize, 0.0f);
	const Vector2 arrowPointY = a_end - (arrowHead * arrowHeadSize) - Vector2(arrowHeadSize, 0.0f);
	AddLine2D(RenderLayer::Debug2D, a_start, a_end, a_tint);
	AddLine2D(RenderLayer::Debug2D, a_end, arrowPointX, a_tint);
	AddLine2D(RenderLayer::Debug2D, a_end, arrowPointY, a_tint);
}

void RenderManager::AddDebugMatrix(const Matrix & a_mat)
{
#ifndef _RELEASE
	Vector startPos = a_mat.GetPos();
	AddLine(RenderLayer::Debug3D, startPos, startPos + a_mat.GetRight(), sc_colourRed);		// Red for X right axis left to right
	AddLine(RenderLayer::Debug3D, startPos, startPos + a_mat.GetLook(), sc_colourGreen);		// Green for Y axis look forward
	AddLine(RenderLayer::Debug3D, startPos, startPos + a_mat.GetUp(), sc_colourBlue);			// Blue for Z axis up
#endif
}

void RenderManager::AddDebugSphere(const Vector & a_worldPos, const float & a_radius, Colour a_colour)
{
#ifndef _RELEASE
	// Draw a wireframe sphere with three circles in each dimension
	const unsigned int numSegments = 16;
	Vector lineStart = a_worldPos;
	Vector lineEnd = a_worldPos;
	for (unsigned int axisCount = 0; axisCount < 3; ++axisCount)
	{
		for (unsigned int i = 0; i < numSegments; ++i)
		{
			const float rFactor = ((float)i / (float)numSegments)*TAU;
			const float rFactorNext = ((float)(i+1) / (float)numSegments)*TAU;
			switch (axisCount)
			{
				case 0: // X axis 
				{
					lineStart = a_worldPos + Vector(sin(rFactor), cos(rFactor), 0.0f);
					lineEnd = a_worldPos +  Vector(sin(rFactorNext), cos(rFactorNext), 0.0f);
					break;
				}
				case 1: // Y axis 
				{
					lineStart = a_worldPos + Vector(0.0f, sin(rFactor), cos(rFactor));
					lineEnd = a_worldPos +  Vector(0.0f, sin(rFactorNext), cos(rFactorNext));
					break;
				}
				case 2: // Z axis 
				{
					lineStart = a_worldPos + Vector(sin(rFactor), 0.0f, cos(rFactor));
					lineEnd = a_worldPos +  Vector(sin(rFactorNext), 0.0f, cos(rFactorNext));
					break;
				}
				default: break;
			}
			AddDebugLine(lineStart, lineEnd, a_colour);
		}
	}
#endif
}

void RenderManager::AddDebugAxisBox(const Vector & a_worldPos, const Vector & a_dimensions, Colour a_colour)
{
#ifndef _RELEASE
	// Define the corners of the box
	Vector halfSize = a_dimensions * 0.5f;
	Vector corners[8];
	corners[0] = Vector(a_worldPos.GetX() - halfSize.GetX(), a_worldPos.GetY() - halfSize.GetY(), a_worldPos.GetZ() - halfSize.GetZ());
	corners[1] = Vector(a_worldPos.GetX() - halfSize.GetX(), a_worldPos.GetY() - halfSize.GetY(), a_worldPos.GetZ() + halfSize.GetZ());
	corners[2] = Vector(a_worldPos.GetX() + halfSize.GetX(), a_worldPos.GetY() - halfSize.GetY(), a_worldPos.GetZ() + halfSize.GetZ());
	corners[3] = Vector(a_worldPos.GetX() + halfSize.GetX(), a_worldPos.GetY() - halfSize.GetY(), a_worldPos.GetZ() - halfSize.GetZ());
	
	corners[4] = Vector(a_worldPos.GetX() - halfSize.GetX(), a_worldPos.GetY() + halfSize.GetY(), a_worldPos.GetZ() - halfSize.GetZ());
	corners[5] = Vector(a_worldPos.GetX() - halfSize.GetX(), a_worldPos.GetY() + halfSize.GetY(), a_worldPos.GetZ() + halfSize.GetZ());
	corners[6] = Vector(a_worldPos.GetX() + halfSize.GetX(), a_worldPos.GetY() + halfSize.GetY(), a_worldPos.GetZ() + halfSize.GetZ());
	corners[7] = Vector(a_worldPos.GetX() + halfSize.GetX(), a_worldPos.GetY() + halfSize.GetY(), a_worldPos.GetZ() - halfSize.GetZ());
	
	// Draw the lines between each corner
	for (unsigned int i = 0; i < 3; ++i)
	{
		AddLine(RenderLayer::Debug3D, corners[i], corners[i+1], a_colour);
		AddLine(RenderLayer::Debug3D, corners[i+4], corners[i+5], a_colour);
		AddLine(RenderLayer::Debug3D, corners[i], corners[i+4], a_colour);
	}
	AddLine(RenderLayer::Debug3D, corners[3], corners[0], a_colour);
	AddLine(RenderLayer::Debug3D, corners[7], corners[4], a_colour);
	AddLine(RenderLayer::Debug3D, corners[3], corners[7], a_colour);
#endif
}

void RenderManager::AddDebugBox(const Matrix & a_worldMat, const Vector & a_dimensions, Colour a_colour)
{
#ifndef _RELEASE
	// TODO
	AddDebugAxisBox(a_worldMat.GetPos(), a_dimensions);
#endif
}

void RenderManager::ManageShader(GameObject * a_gameObject, const char * a_shaderName)
{
	if (Shader * existingShader = GetShader(a_shaderName))
	{
		// It's already in the list, link it to the game object
		a_gameObject->SetShader(existingShader);
	}
	// If not, create the shader
	else if (Shader * pNewShader = new Shader(a_shaderName))
	{
		if (RenderManager::InitShaderFromFile(*pNewShader))
		{
			LinkedListNode<Shader> * shaderNode = new LinkedListNode<Shader>();
			shaderNode->SetData(pNewShader);
			a_gameObject->SetShader(pNewShader);
			m_shaders.Insert(shaderNode);
		}
		else // Compile error will be reported in the log
		{
			delete pNewShader;
			a_gameObject->SetShader(NULL);
		}
	}

	// TODO Check this object is not already managed

	// Link shader in list to game object for management
	if (a_gameObject != NULL)
	{
		if (ManagedShader * pManShader = new ManagedShader())
		{	
			pManShader->m_shaderObject = a_gameObject;
			AddManagedShader(pManShader);
		}
	}
}

void RenderManager::ManageShader(Scene * a_scene, const char * a_shaderName)
{
	if (Shader * existingShader = GetShader(a_shaderName))
	{
		// It's already in the list, link it to the game object
		a_scene->SetShader(existingShader);
	}
	// If not, create the shader
	else if (Shader * pNewShader = new Shader(a_shaderName))
	{
		if (RenderManager::InitShaderFromFile(*pNewShader))
		{
			LinkedListNode<Shader> * shaderNode = new LinkedListNode<Shader>();
			shaderNode->SetData(pNewShader);
			a_scene->SetShader(pNewShader);
			m_shaders.Insert(shaderNode);
		}
		else // Compile error will be reported in the log
		{
			delete pNewShader;
			a_scene->SetShader(NULL);
		}
	}

	// TODO Check this scene is not already managed

	// Link shader in list to scene for management
	if (a_scene != NULL)
	{
		if (ManagedShader * pManShader = new ManagedShader())
		{	
			pManShader->m_shaderScene = a_scene;
			AddManagedShader(pManShader);
		}
	}
}

void RenderManager::UnManageShader(GameObject * a_gameObject)
{
	if (a_gameObject != NULL)
	{
		ManagedShaderNode * next = m_managedShaders.GetHead();
		while (next != NULL)
		{
			if (next->GetData()->m_shaderObject == a_gameObject)
			{
				// Delete the management resources, the object owns the shader 
				m_managedShaders.Remove(next);
				delete next->GetData();
				delete next;
				return;
			}
			next = next->GetNext();
		}
	}

	// TODO If the shader that was just unmanaged is not shared by any other object or scene then remove it from m_shaders
}

void RenderManager::UnManageShader(Scene * a_scene)
{
	if (a_scene != NULL)
	{
		ManagedShaderNode * next = m_managedShaders.GetHead();
		while (next != NULL)
		{
			if (next->GetData()->m_shaderScene == a_scene)
			{
				// Delete the management resources, the object owns the shader 
				m_managedShaders.Remove(next);
				delete next->GetData();
				delete next;
				return;
			}
		}
		next = next->GetNext();
	}

	// TODO If the shader that was just unmanaged is not shared by any other object or scene then remove it from m_shaders
}

bool RenderManager::InitShaderFromFile(Shader & a_shader_OUT)
{
	struct stat fileInfo;
	unsigned int numChars = 0;
	char * vertexSource = NULL;
	char * fragmentSource = NULL;
	char fullShaderPath[StringUtils::s_maxCharsPerLine];
	char fileLine[StringUtils::s_maxCharsPerLine];
	
	// Open file to allocate the correct size string
	sprintf(fullShaderPath, "%s%s.vsh", RenderManager::Get().GetShaderPath(), a_shader_OUT.GetName());
	if (stat(&fullShaderPath[0], &fileInfo) == 0)
	{
		if (vertexSource = (char *)malloc(fileInfo.st_size))
		{
			// Open the file and read till the file has more contents
			int numChars = 0;
			ifstream vertexShaderFile(fullShaderPath);
			if (vertexShaderFile.is_open())
			{
				while (vertexShaderFile.good())
				{
					vertexShaderFile.getline(fileLine, StringUtils::s_maxCharsPerLine);
					const int lineSize = strlen(fileLine);
					strncpy(&vertexSource[numChars], fileLine, sizeof(char) * lineSize);
					numChars += lineSize;
				}
				vertexSource[numChars++] = '\0';
			}
			vertexShaderFile.close();
		}
	}

	// Open the pixel shader file and parse each line into a string
	sprintf(fullShaderPath, "%s%s.fsh", RenderManager::Get().GetShaderPath(), a_shader_OUT.GetName());
	if (stat(&fullShaderPath[0], &fileInfo) == 0)
	{
		if (fragmentSource = (char *)malloc(fileInfo.st_size))
		{
			// Open the file and read till the file has more contents
			int numChars = 0;
			ifstream fragmentShaderFile(fullShaderPath);
			if (fragmentShaderFile.is_open())
			{
				while (fragmentShaderFile.good())
				{
					fragmentShaderFile.getline(fileLine, StringUtils::s_maxCharsPerLine);
					const int lineSize = strlen(fileLine);
					strncpy(&fragmentSource[numChars], fileLine, sizeof(char) * lineSize);
					numChars += lineSize;
				}
				fragmentSource[numChars++] = '\0';
			}
			fragmentShaderFile.close();
		}
	}

	// Init then free the memory for shader sources
	bool shadersCompiled = InitShaderFromMemory(vertexSource, fragmentSource, a_shader_OUT);
	if (vertexSource != NULL)
	{
		free(vertexSource);
	}
	if (fragmentSource != NULL)
	{
		free(fragmentSource);
	}

	return shadersCompiled;
}

bool RenderManager::InitShaderFromMemory(char * a_vertShaderSrc, char * a_fragShaderSrc, Shader & a_shader_OUT)
{
	const int vertShaderSize = strlen(a_vertShaderSrc);
	const int fragShaderSize = strlen(a_fragShaderSrc);
	if (a_vertShaderSrc == NULL || a_fragShaderSrc == NULL || vertShaderSize <= 0 || fragShaderSize <= 0)
	{
		return false;
	}

	// All scene shaders include the global inputs and outputs
	#include "Shaders\global.fsh.inc"
	#include "Shaders\global.vsh.inc"

	char * vertexSource = NULL;
	char * fragmentSource = NULL;
	size_t globalVertexSize = sizeof(char) * strlen(globalVertexShader);
	size_t globalFragmentSize = sizeof(char) * strlen(globalFragmentShader);
	if (vertexSource = (char *)malloc((globalVertexSize + vertShaderSize + 2) * sizeof(char)))
	{
		// Write a copy of the global vertex shader and insert a newline
		int numChars = globalVertexSize;
		strncpy(vertexSource, globalVertexShader, globalVertexSize);
		vertexSource[numChars++] = '\n';

		// Now write the shader source
		strncpy(&vertexSource[numChars], a_vertShaderSrc, vertShaderSize);
		numChars += vertShaderSize;
		vertexSource[numChars] = '\0';
	}

	if (fragmentSource = (char *)malloc((globalFragmentSize + fragShaderSize + 2) * sizeof(char)))
	{
		// Write a copy of the global fragment shader and insert a newline
		int numChars = globalFragmentSize;
		strncpy(fragmentSource, globalFragmentShader, globalFragmentSize);
		fragmentSource[numChars++] = '\n';

		// Now write the shader source
		strncpy(&fragmentSource[numChars], a_fragShaderSrc, fragShaderSize);
		numChars += fragShaderSize;
		fragmentSource[numChars] = '\0';
	}

	if (vertexSource == NULL || fragmentSource == NULL)
	{
		free(vertexSource);
		free(fragmentSource);
		Log::Get().WriteEngineErrorNoParams("Memory allocation failed in loading shaders!");
		return false;
	}
	a_shader_OUT.Init(vertexSource, fragmentSource);
	free(vertexSource);
	free(fragmentSource);
	return a_shader_OUT.IsCompiled();
}

void RenderManager::AddManagedShader(ManagedShader * a_newManShader)
{
	if (a_newManShader)
	{
		// Expecting a game object OR scene with a valid shader already set
		Shader * pShader = a_newManShader->m_shaderObject != NULL ? a_newManShader->m_shaderObject->GetShader() : a_newManShader->m_shaderScene->GetShader();
		if (pShader == NULL)
		{
			assert(!"AddManagedShader called without an valid shader assigned to an object or scene");
			return;
		}

		DataPack & dataPack = DataPack::Get();
		if (!dataPack.IsLoaded())
		{
			// Now add the object to the list of managed items by time stamp
			char fullShaderPath[StringUtils::s_maxCharsPerLine];
			sprintf(fullShaderPath, "%s%s.vsh", RenderManager::Get().GetShaderPath(), pShader->GetName());
			FileManager::Get().GetFileTimeStamp(fullShaderPath, a_newManShader->m_vertexTimeStamp);
			sprintf(fullShaderPath, "%s%s.fsh", RenderManager::Get().GetShaderPath(), pShader->GetName());
			FileManager::Get().GetFileTimeStamp(fullShaderPath, a_newManShader->m_fragmentTimeStamp);
		}

		// Add to the list of shaders
		if (ManagedShaderNode * pManShaderNode = new ManagedShaderNode())
		{
			pManShaderNode->SetData(a_newManShader);
			m_managedShaders.Insert(pManShaderNode);
		}
	}
}

Shader * RenderManager::GetShader(const char * a_shaderName)
{
	// Iterate through all shaders looking for the named program
	LinkedListNode<Shader> * next = m_shaders.GetHead();
	while (next != NULL)
	{
		if (strcmp(next->GetData()->GetName(), a_shaderName) == 0)
		{
			return next->GetData();
		}
		next = next->GetNext();
	}

	return NULL;
}
