#include <windows.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "RenderManager.h"

template<> RenderManager* Singleton<RenderManager>::s_instance = NULL;

bool RenderManager::Init(Colour a_clearColour)
{
    // Set the clear colour
    m_clearColour = a_clearColour;

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

    return true;
}

bool RenderManager::Resize(unsigned int a_viewWidth, unsigned int a_viewHeight, unsigned int a_viewBpp, bool a_fullScreen)
{
    // Setup viewport ratio
    if ( a_viewHeight == 0 )
	a_viewHeight = 1;
    float ratio = ( GLfloat )a_viewWidth / ( GLfloat )a_viewHeight;

    // Setup our viewport
    glViewport( 0, 0, ( GLint )a_viewWidth, ( GLint )a_viewHeight );

    // Change to the projection matrix and set our viewing volume
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );

    // Set our perspective
    gluPerspective( 45.0f, ratio, 0.1f, 100.0f );

    // Make sure we're chaning the model view and not the projection
    glMatrixMode( GL_MODELVIEW );

    // Reset The View
    glLoadIdentity( );

    return true;
}

void RenderManager::DrawScene()
{
    // Clear the color and depth buffers in preparation for drawing
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glLoadIdentity( );
}

void RenderManager::AddQuad(eBatch a_batch, Vector a_topLeft, float a_width, float a_height, Texture * a_tex, Texture::eOrientation a_orient)
{
	a_batch;
	a_topLeft;
	a_width;
	a_height;
	a_tex;
}