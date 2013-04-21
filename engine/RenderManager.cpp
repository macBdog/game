#include <windows.h>

#include <GL/glew.h>
#include <GL/glu.h>

#include "../core/MathUtils.h"

#include "DebugMenu.h"
#include "Log.h"
#include "Texture.h"

#include "RenderManager.h"

template<> RenderManager * Singleton<RenderManager>::s_instance = NULL;
const float RenderManager::s_renderDepth2D = -10.0f;
const float RenderManager::s_nearClipPlane = 0.5f;
const float RenderManager::s_farClipPlane = 1000.0f;
const float RenderManager::s_fovAngleY = 50.0f;

bool RenderManager::Startup(Colour a_clearColour)
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
	glLineWidth(1.0f);
	glPointSize(1.0f);

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
	bool batchAlloc = true;
	for (unsigned int i = 0; i < eBatchCount; ++i)
	{
		// Tris
		m_tris[i] = (Tri *)malloc(sizeof(Tri) * s_maxPrimitivesPerBatch);
		batchAlloc &= m_tris[i] != NULL;
		m_triCount[i] = 0;

		// Quads
		m_quads[i] = (Quad *)malloc(sizeof(Quad) * s_maxPrimitivesPerBatch);
		batchAlloc &= m_quads[i] != NULL;
		m_quadCount[i] = 0;

		// Lines
		m_lines[i] = (Line *)malloc(sizeof(Line) * s_maxPrimitivesPerBatch);
		batchAlloc &= m_lines[i] != NULL;
		m_lineCount[i] = 0;

		// Render models
		m_models[i] = (RenderModel *)malloc(sizeof(RenderModel) * s_maxPrimitivesPerBatch);
		batchAlloc &= m_models[i] != NULL;
		m_modelCount[i] = 0;

		// Font characters
		m_fontChars[i] = (FontChar *)malloc(sizeof(FontChar) * s_maxPrimitivesPerBatch);
		batchAlloc &= m_fontChars[i] != NULL;
		m_fontCharCount[i] = 0;
	}

	// Alert if memory allocation failed
	if (!batchAlloc)
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "RenderManager failed to allocate batch/primitive memory!");
	}

	// Setup default shaders
	#include "Shaders\default.vsh.inc"
	#include "Shaders\default.fsh.inc"
	GLenum extensionStartup = glewInit();
	if (extensionStartup != GLEW_OK)
	{
		Log::Get().WriteEngineErrorNoParams("Initialisation of the shader extension library GLEW failed!");
		return false;
	} 
	m_defaultShader = new Shader(defaultVertexShader, defaultFragmentShader);

    return batchAlloc && m_defaultShader->GetShader() > 0;
}

bool RenderManager::Shutdown()
{
	// Clean up storage for all primitives
	for (unsigned int i = 0; i < eBatchCount; ++i)
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
	
	// Clean up the default shader
	if (m_defaultShader != NULL)
	{
		delete m_defaultShader;
	}

	return true;
}

bool RenderManager::Resize(unsigned int a_viewWidth, unsigned int a_viewHeight, unsigned int a_viewBpp, bool a_fullScreen)
{
    // Setup viewport ratio avoiding a divide by zero
    if ( a_viewHeight == 0 )
	{
		a_viewHeight = 1;
	}

	m_viewWidth = a_viewWidth;
	m_viewHeight = a_viewHeight;
	m_aspect = (float)m_viewWidth / (float)m_viewHeight;

    // Setup our viewport
    glViewport(0, 0, (GLint)a_viewWidth, (GLint)a_viewHeight);
	
	// Set up fog
	// TODO Fog and other effects (DOF, blur, colourisation etc) should be configurable in the scene file
    glFogi(GL_FOG_MODE, GL_LINEAR);                                         // Fog Mode nicest
    glFogfv(GL_FOG_COLOR, m_clearColour.GetValues());						// Fog colour matches clear colour
    glFogf(GL_FOG_DENSITY, 0.15f);                                          // How Dense Will The Fog Be
    glHint(GL_FOG_HINT, GL_DONT_CARE);                                      // Fog Hint Value
    glFogf(GL_FOG_START, 20.0f);                                            // Fog Start Depth
    glFogf(GL_FOG_END, 400.0f);                                             // Fog End Depth
    glEnable(GL_FOG);                                                       // Enables GL_FOG

    // Change to the projection matrix and set our viewing volume
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Set our perspective
    gluPerspective(s_fovAngleY, m_aspect, s_nearClipPlane, s_farClipPlane);

    // Make sure we're chaning the model view and not the projection
    glMatrixMode(GL_MODELVIEW);

    // Reset The View
    glLoadIdentity();

    return true;
}

void RenderManager::DrawScene(Matrix & a_viewMatrix)
{
	// Handle different rendering modes
	switch (m_renderMode)
	{
		case eRenderModeNone:
		{
			// Clear the queues as the rest of the system will continue to add primitives
			for (unsigned int i = 0; i < eBatchCount; ++i)
			{
				m_triCount[i] = 0;
				m_quadCount[i] = 0;
				m_lineCount[i] = 0;
				m_modelCount[i] = 0;
				m_fontCharCount[i] = 0;
			}
			return;
		}
		case eRenderModeWireframe:
		{
			// TODO
		}
		default: break;
	}

    // Clear the color and depth buffers in preparation for drawing
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

	// Use the default shader
	m_defaultShader->BindShader();

	// Draw quads for each batch
	for (unsigned int i = 0; i < eBatchCount; ++i)
	{
		// Switch render mode for each batch
		switch ((eBatch)i)
		{
			case eBatchWorld:
			case eBatchDebug3D:
			{
				// Setup projection matrix stack to transform eye space to clip coordinates
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				gluPerspective(s_fovAngleY, m_aspect, s_nearClipPlane, s_farClipPlane);

				// Setup the inverse of the camera transformation in the modelview matrix
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glLoadMatrixf(a_viewMatrix.GetValues());

				// Setup other world only rendering flags
				glEnable(GL_DEPTH_TEST);

				break;
			}
			case eBatchGui:
			case eBatchDebug2D:
			{
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, -100000.0, 100000.0);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				break;
			}
			default: break;
		}

		// Ensure debug text renders on top of everything
		if ((eBatch)i >= eBatchDebug2D)
		{
			glClear(GL_DEPTH_BUFFER_BIT);
		}

		// Submit the tris
		Tri * t = m_tris[i];
		for (unsigned int j = 0; j < m_triCount[i]; ++j)
		{
			glColor4f(t->m_colour.GetR(), t->m_colour.GetG(), t->m_colour.GetB(), t->m_colour.GetA());

			// Draw a quad with a texture
			if (t->m_textureId >= 0)
			{
				glBindTexture(GL_TEXTURE_2D, t->m_textureId);
	
				glBegin(GL_TRIANGLES);

				glTexCoord2f(t->m_coords[0].GetX(), t->m_coords[0].GetY()); 
				glVertex3f(t->m_verts[0].GetX(), t->m_verts[0].GetY(), t->m_verts[0].GetZ());

				glTexCoord2f(t->m_coords[1].GetX(), t->m_coords[1].GetY()); 
				glVertex3f(t->m_verts[1].GetX(), t->m_verts[1].GetY(), t->m_verts[1].GetZ());

				glTexCoord2f(t->m_coords[2].GetX(), t->m_coords[2].GetY()); 
				glVertex3f(t->m_verts[2].GetX(), t->m_verts[2].GetY(), t->m_verts[2].GetZ());

				glEnd();
			}

			t++;
		}

		// Submit the quad
		Quad * q = m_quads[i];
		for (unsigned int j = 0; j < m_quadCount[i]; ++j)
		{
			glColor4f(q->m_colour.GetR(), q->m_colour.GetG(), q->m_colour.GetB(), q->m_colour.GetA());

			// Draw a quad with a texture
			if (q->m_textureId >= 0)
			{
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, q->m_textureId);
			}
			else
			{
				glDisable(GL_TEXTURE_2D);
			}
	
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

		// Draw lines in the current batch
		glDisable(GL_TEXTURE_2D);
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
		glEnable(GL_TEXTURE_2D);

		// Draw font chars by calling their display lists
		FontChar * fc = m_fontChars[i];
		for (unsigned int j = 0; j < m_fontCharCount[i]; ++j)
		{
			glPushMatrix();
			glTranslatef(fc->m_pos.GetX(), fc->m_pos.GetY(), fc->m_pos.GetZ());

			if (!fc->m_2d)
			{
				glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
			}
			
			glColor4f(fc->m_colour.GetR(), fc->m_colour.GetG(), fc->m_colour.GetB(), fc->m_colour.GetA());
			glScalef(fc->m_size, fc->m_size, 0.0f);
			glCallList(fc->m_displayListId);
			glPopMatrix();
			++fc;
		}

		// Draw models by calling their display lists
		RenderModel * rm = m_models[i];
		for (unsigned int j = 0; j < m_modelCount[i]; ++j)
		{
			glPushMatrix();
			glMultMatrixf(rm->m_mat->GetValues());
			glCallList(rm->m_model->GetDisplayListId());
			glPopMatrix();
			++rm;
		}

		m_triCount[i] = 0;
		m_quadCount[i] = 0;
		m_lineCount[i] = 0;
		m_modelCount[i] = 0;
		m_fontCharCount[i] = 0;
	}
}

unsigned int RenderManager::RegisterFontChar(Vector2 a_size, TexCoord a_texCoord, TexCoord a_texSize, Texture * a_texture)
{
	// Generate and begin compiling a new display list
	GLuint displayListId = glGenLists(1);
	glNewList(displayListId, GL_COMPILE);
	
	// Bind the texture
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

void RenderManager::AddLine2D(eBatch a_batch, Vector2 a_point1, Vector2 a_point2, Colour a_tint)
{
	// Set the Z depth according to 2D project and call through to 3D version
	AddLine(a_batch,
			Vector(a_point1.GetX(), a_point1.GetY(), s_renderDepth2D),
		    Vector(a_point2.GetX(), a_point2.GetY(), s_renderDepth2D),
			a_tint);
}

void RenderManager::AddLine(eBatch a_batch, Vector a_point1, Vector a_point2, Colour a_tint)
{
	// Don't add more primitives than have been allocated for
	if (m_lineCount[a_batch] >= s_maxPrimitivesPerBatch)
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Too many line primitives added, max is %d", m_lineCount);
		return;
	}

	// Copy params to next queue item
	Line * l = m_lines[a_batch];
	l += m_lineCount[a_batch]++;

	l->m_colour = a_tint;
		
	// Setup verts 
	l->m_verts[0] = a_point1;
	l->m_verts[1] = a_point2;
}

void RenderManager::AddQuad2D(eBatch a_batch, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, Texture::eOrientation a_orient, Colour a_tint)
{
	// Create a quad with tex coord at top left
	TexCoord texPos(0.0f, 0.0f);
	TexCoord texSize(1.0f, 1.0f);
	AddQuad2D(a_batch, a_topLeft, a_size, a_tex, texPos, texSize, a_orient, a_tint);
}

void RenderManager::AddQuad2D(eBatch a_batch, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, TexCoord a_texCoord, TexCoord a_texSize, Texture::eOrientation a_orient, Colour a_tint)
{
	Vector2 verts[4] =	{Vector2(a_topLeft.GetX(), a_topLeft.GetY()),
						Vector2(a_topLeft.GetX() + a_size.GetX(), a_topLeft.GetY()),
						Vector2(a_topLeft.GetX() + a_size.GetX(), a_topLeft.GetY() - a_size.GetY()),
						Vector2(a_topLeft.GetX(), a_topLeft.GetY() - a_size.GetY())};
	AddQuad2D(a_batch, &verts[0], a_tex, a_texCoord, a_texSize, a_orient, a_tint);
}

void RenderManager::AddQuad2D(eBatch a_batch, Vector2 * a_verts, Texture * a_tex, TexCoord a_texCoord, TexCoord a_texSize, Texture::eOrientation a_orient, Colour a_tint)
{
	// Don't add more primitives than have been allocated for
	if (m_quadCount[a_batch] >= s_maxPrimitivesPerBatch)
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Too many primitives added for batch %d, max is %d", a_batch, s_maxPrimitivesPerBatch);
		return;
	}

	// Warn about no texture
	if (a_batch != eBatchDebug2D && a_tex == NULL)
	{
		Log::Get().WriteOnce(Log::LL_WARNING, Log::LC_ENGINE, "Quad submitted without a texture");
	}

	// Copy params to next queue item
	Quad * q = m_quads[a_batch];
	q += m_quadCount[a_batch]++;
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
			case Texture::eOrientationNormal:
			{
				q->m_coords[0] = TexCoord(a_texCoord.GetX(),					1.0f - a_texCoord.GetY());
				q->m_coords[1] = TexCoord(a_texCoord.GetX() + a_texSize.GetX(),	1.0f - a_texCoord.GetY());
				q->m_coords[2] = TexCoord(a_texCoord.GetX() + a_texSize.GetX(),	1.0f - a_texSize.GetY() - a_texCoord.GetY());
				q->m_coords[3] = TexCoord(a_texCoord.GetX(),					1.0f - a_texSize.GetY() - a_texCoord.GetY());
				break;
			}
			case Texture::eOrientationFlipVert:
			{
				// TODO
				break;
			}
			case Texture::eOrientationFlipHoriz:
			{
				// TODO
				break;
			}
			case Texture::eOrientationFlipBoth:
			{
				// TODO
				break;
			}

			default: break;
		}
	}
}

void RenderManager::AddQuad3D(eBatch a_batch, Vector * a_verts, Texture * a_tex, Colour a_tint)
{
	// Don't add more primitives than have been allocated for
	if (m_quadCount[a_batch] >= s_maxPrimitivesPerBatch)
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Too many primitives added for batch %d, max is %d", a_batch, s_maxPrimitivesPerBatch);
		return;
	}

	// Warn about no texture
	if (a_batch != eBatchDebug2D && a_tex == NULL)
	{
		Log::Get().WriteOnce(Log::LL_WARNING, Log::LC_ENGINE, "3D Quad submitted without a texture");
	}

	// Copy params to next queue item
	Quad * q = m_quads[a_batch];
	q += m_quadCount[a_batch]++;
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

void RenderManager::AddTri(RenderManager::eBatch a_batch, Vector a_point1, Vector a_point2, Vector a_point3, TexCoord a_txc1, TexCoord a_txc2, TexCoord a_txc3, Texture * a_tex, Colour a_tint)
{
	// Don't add more primitives than have been allocated for
	if (m_triCount[a_batch] >= s_maxPrimitivesPerBatch)
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Too many tri primitives added for batch %d, max is %d", a_batch, s_maxPrimitivesPerBatch);
		return;
	}

	// Warn about no texture
	if (a_batch != eBatchDebug3D && a_tex == NULL)
	{
		Log::Get().WriteOnce(Log::LL_WARNING, Log::LC_ENGINE, "Tri submitted without a texture");
	}

	// Copy params to next queue item
	Tri * t = m_tris[a_batch];
	t += m_triCount[a_batch]++;
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

void RenderManager::AddModel(eBatch a_batch, Model * a_model, Matrix * a_mat)
{
	// Don't add more models than have been allocated for
	if (m_modelCount[a_batch] >= s_maxPrimitivesPerBatch)
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Too many render models added for batch %d, max is %d", a_batch, s_maxPrimitivesPerBatch);
		return;
	}

	// If we have not generated buffers for this model
	if (!a_model->IsDisplayListGenerated())
	{
		// Alias model data
		unsigned int numVerts = a_model->GetNumVertices();
		Texture * diffuseTex = a_model->GetDiffuseTexture();
		Vector * verts = a_model->GetVertices();
		TexCoord * uvs = a_model->GetUvs();

		GLuint displayListId = glGenLists(1);
		glNewList(displayListId, GL_COMPILE);
		
		// Bind the texture and set colour
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		if (diffuseTex != NULL)
		{
			glBindTexture(GL_TEXTURE_2D, diffuseTex->GetId());
		}

		glBegin(GL_TRIANGLES);
		// Draw vertices in threes
		for (unsigned int i = 0; i < numVerts; i += Model::s_vertsPerTri)
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

		a_model->SetDisplayListId(displayListId);
	}

	RenderModel * r = m_models[a_batch];
	r += m_modelCount[a_batch]++;
	r->m_model = a_model;
	r->m_mat = a_mat;

	// Show the local matrix in debug mode
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		AddDebugMatrix(*a_mat);
	}

}

void RenderManager::AddFontChar(eBatch a_batch, unsigned int a_fontCharId, float a_size, Vector a_pos, Colour a_colour)
{
	// Don't add more font characters than have been allocated for
	if (m_fontCharCount[a_batch] >= s_maxPrimitivesPerBatch)
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Too many font characters added for batch %d, max is %d", a_batch, s_maxPrimitivesPerBatch);
		return;
	}

	FontChar * fc = m_fontChars[a_batch];
	fc += m_fontCharCount[a_batch]++;
	fc->m_displayListId = a_fontCharId;
	fc->m_size = a_size;
	fc->m_pos = a_pos;
	fc->m_colour = a_colour;
	fc->m_2d = a_batch == eBatchGui || a_batch == eBatchDebug2D;
}

void RenderManager::AddDebugMatrix(const Matrix & a_mat)
{
	Vector startPos = a_mat.GetPos();
	AddLine(eBatchDebug3D, startPos, startPos + a_mat.GetRight(), sc_colourRed);		// Red for X right axis left to right
	AddLine(eBatchDebug3D, startPos, startPos + a_mat.GetLook(), sc_colourGreen);		// Green for Y axis look forward
	AddLine(eBatchDebug3D, startPos, startPos + a_mat.GetUp(), sc_colourBlue);			// Blue for Z axis up
}

void RenderManager::AddDebugSphere(const Vector & a_worldPos, const float & a_radius, Colour a_colour)
{
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
}

void RenderManager::AddDebugAxisBox(const Vector & a_worldPos, const Vector & a_dimensions, Colour a_colour)
{
	// Define the corners of the cube
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
		AddLine(eBatchDebug3D, corners[i], corners[i+1], a_colour);
		AddLine(eBatchDebug3D, corners[i+4], corners[i+5], a_colour);
		AddLine(eBatchDebug3D, corners[i], corners[i+4], a_colour);
	}
	AddLine(eBatchDebug3D, corners[3], corners[0], a_colour);
	AddLine(eBatchDebug3D, corners[7], corners[4], a_colour);
	AddLine(eBatchDebug3D, corners[3], corners[7], a_colour);
}