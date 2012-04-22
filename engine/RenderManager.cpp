#include <windows.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "Log.h"
#include "Texture.h"

#include "RenderManager.h"

template<> RenderManager * Singleton<RenderManager>::s_instance = NULL;

bool RenderManager::Init(Colour a_clearColour)
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

    // Enables Depth Testing
    glEnable(GL_DEPTH_TEST);

    // The Type Of Depth Test To Do
    glDepthFunc(GL_LEQUAL);

    // Really Nice Perspective Calculations
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// Storage for all the primitives
	bool batchAlloc = true;
	for (unsigned int i = 0; i < eBatchCount; ++i)
	{
		m_batch[i] = (Quad *)malloc(sizeof(Quad) * s_maxPrimitivesPerBatch);
		batchAlloc &= m_batch[i] != NULL;
		m_batchCount[i] = 0;
	}

	// Alert if memory allocation failed
	if (!batchAlloc)
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_CORE, "RenderManager failed to allocate batch memory!");
	}

    return batchAlloc;
}

bool RenderManager::Resize(unsigned int a_viewWidth, unsigned int a_viewHeight, unsigned int a_viewBpp, bool a_fullScreen)
{
    // Setup viewport ratio avoiding a divide by zero
    if ( a_viewHeight == 0 )
	{
		a_viewHeight = 1;
	}

    float ratio = (GLfloat)a_viewWidth / (GLfloat)a_viewHeight;

    // Setup our viewport
    glViewport(0, 0, (GLint)a_viewWidth, (GLint)a_viewHeight);

    // Change to the projection matrix and set our viewing volume
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Set our perspective
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);

    // Make sure we're chaning the model view and not the projection
    glMatrixMode(GL_MODELVIEW);

    // Reset The View
    glLoadIdentity();

    return true;
}

void RenderManager::DrawScene()
{
    // Clear the color and depth buffers in preparation for drawing
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

	for (unsigned int i = 0; i < eBatchCount; ++i)
	{
		Quad * q = m_batch[i];
		for (unsigned int j = 0; j < m_batchCount[i]; ++j)
		{
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glBindTexture(GL_TEXTURE_2D, q->m_textureId);
	
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
		m_batchCount[i] = 0;
	}

}

void RenderManager::AddQuad2D(eBatch a_batch, Vector a_topLeft, float a_width, float a_height, Texture * a_tex, Texture::eOrientation a_orient)
{
	// Copy params to next queue item
	Quad * q = m_batch[a_batch];
	q += m_batchCount[a_batch]++;
	q->m_textureId = a_tex->GetId();
	
	// Setup verts for clockwise drawing
	q->m_verts[0] = a_topLeft;
	q->m_verts[1] = Vector(a_topLeft.GetX() + a_width, a_topLeft.GetY(), a_topLeft.GetZ());
	q->m_verts[2] = Vector(a_topLeft.GetX() + a_width, a_topLeft.GetY() - a_height, a_topLeft.GetZ());
	q->m_verts[3] = Vector(a_topLeft.GetX(), a_topLeft.GetY() - a_height, a_topLeft.GetZ());

	// Set texcoords based on orientation
	switch(a_orient)
	{
		case Texture::eOrientationNormal:
		{
			q->m_coords[0] = TexCoord(0.0f, 1.0f);
			q->m_coords[1] = TexCoord(1.0f, 1.0f);
			q->m_coords[2] = TexCoord(1.0f, 0.0f);
			q->m_coords[3] = TexCoord(0.0f, 0.0f);
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