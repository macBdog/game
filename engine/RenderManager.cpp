#include <windows.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "DebugMenu.h"
#include "Log.h"
#include "Texture.h"

#include "RenderManager.h"

template<> RenderManager * Singleton<RenderManager>::s_instance = NULL;
const float RenderManager::s_renderDepth2D = -10.0f;
const float RenderManager::s_nearClipPlane = 0.5f;
const float RenderManager::s_farClipPlane = 200.0f;
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
	}

	// Alert if memory allocation failed
	if (!batchAlloc)
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "RenderManager failed to allocate batch/primitive memory!");
	}

    return batchAlloc;
}

bool RenderManager::Shutdown()
{
	// Clean up storage for all primitives
	for (unsigned int i = 0; i < eBatchCount; ++i)
	{
		free(m_tris[i]);
		free(m_quads[i]);
		free(m_lines[i]);
		m_triCount[i] = 0;
		m_quadCount[i] = 0;
		m_lineCount[i] = 0;
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

void RenderManager::DrawScene()
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

	// Draw quads for each batch
	for (unsigned int i = 0; i < eBatchCount; ++i)
	{
		// Switch render mode for each batch
		switch ((eBatch)i)
		{
			case eBatchWorld:
			{
				glEnable(GL_DEPTH_TEST);
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				gluPerspective(s_fovAngleY, m_aspect, s_nearClipPlane, s_farClipPlane);
				glMatrixMode(GL_MODELVIEW);
				break;
			}
			case eBatchGui:
			case eBatchDebug:
			{
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, -100000.0, 100000.0);
				glMatrixMode(GL_MODELVIEW);
				break;
			}
			default: break;
		}

		// This may not be necessary but its a cheap catch all
		glLoadIdentity();

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
		
		m_triCount[i] = 0;
		m_quadCount[i] = 0;
		m_lineCount[i] = 0;
	}

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

void RenderManager::AddQuad2D(eBatch a_batch, Vector2 a_topLeft, Vector2 a_size, Texture * a_tex, TexCoord texCoord, TexCoord texSize, Texture::eOrientation a_orient, Colour a_tint)
{
	// Don't add more primitives than have been allocated for
	if (m_quadCount[a_batch] >= s_maxPrimitivesPerBatch)
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Too many primitives added for batch %d, max is %d", a_batch, s_maxPrimitivesPerBatch);
		return;
	}

	// Warn about no texture
	if (a_batch != eBatchDebug && a_tex == NULL)
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
	q->m_verts[0] = Vector(a_topLeft.GetX(), a_topLeft.GetY(), s_renderDepth2D);
	q->m_verts[1] = Vector(a_topLeft.GetX() + a_size.GetX(), a_topLeft.GetY(), s_renderDepth2D);
	q->m_verts[2] = Vector(a_topLeft.GetX() + a_size.GetX(), a_topLeft.GetY() - a_size.GetY(), s_renderDepth2D);
	q->m_verts[3] = Vector(a_topLeft.GetX(), a_topLeft.GetY() - a_size.GetY(), s_renderDepth2D);

	// Set texcoords based on orientation
	if (a_tex)
	{
		switch(a_orient)
		{
			case Texture::eOrientationNormal:
			{
				q->m_coords[0] = TexCoord(texCoord.GetX(),					1.0f - texCoord.GetY());
				q->m_coords[1] = TexCoord(texCoord.GetX() + texSize.GetX(),	1.0f - texCoord.GetY());
				q->m_coords[2] = TexCoord(texCoord.GetX() + texSize.GetX(),	1.0f - texSize.GetY() - texCoord.GetY());
				q->m_coords[3] = TexCoord(texCoord.GetX(),					1.0f - texSize.GetY() - texCoord.GetY());
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


void RenderManager::AddTri(RenderManager::eBatch a_batch, Vector a_point1, Vector a_point2, Vector a_point3, TexCoord a_txc1, TexCoord a_txc2, TexCoord a_txc3, Texture * a_tex, Colour a_tint)
{
	// Don't add more primitives than have been allocated for
	if (m_triCount[a_batch] >= s_maxPrimitivesPerBatch)
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Too many tri primitives added for batch %d, max is %d", a_batch, s_maxPrimitivesPerBatch);
		return;
	}

	// Warn about no texture
	if (a_batch != eBatchDebug && a_tex == NULL)
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

void RenderManager::AddModel(eBatch a_batch, Model * a_model, const Matrix & a_mat)
{
	// TODO
}

void RenderManager::AddMatrix(eBatch a_batch, const Matrix & a_mat)
{
	Vector startPos = a_mat.GetPos();
	AddLine(a_batch, startPos, a_mat.GetRight(), sc_colourRed);		// Red for X right axis left right
	AddLine(a_batch, startPos, a_mat.GetLook(), sc_colourGreen);	// Green for Y axis look
	AddLine(a_batch, startPos, a_mat.GetUp(), sc_colourBlue);		// Blue for Z axis up
}
