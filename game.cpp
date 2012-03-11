#include <cstdio>
#include <cstdlib>

#include <windows.h>

#include <SDL.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "core/cSystemLog.h"

#include "engine/eInputManager.h"
#include "engine/eRenderManager.h"

cSystemLog m_log;
eRenderManager m_renderManager;
eInputManager m_inputManager;

int main(int argc, char *argv[])
{
    // Initialize SDL video
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Unable to init SDL: %s\n", SDL_GetError());
        return 1;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    /* the flags to pass to SDL_SetVideoMode */
    int videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
    videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */
    videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
    videoFlags |= SDL_RESIZABLE;       /* Enable window resizing */

    /* This checks to see if surfaces can be stored in memory */
    const SDL_VideoInfo * videoInfo = SDL_GetVideoInfo();

    if ( videoInfo->hw_available )
    {
        videoFlags |= SDL_HWSURFACE;
    }
    else
    {
        videoFlags |= SDL_SWSURFACE;
    }

    /* This checks if hardware blits can be done */
    if ( videoInfo->blit_hw )
    {
        videoFlags |= SDL_HWACCEL;
    }

    /* Sets up OpenGL double buffering */
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    // create a new window
    SDL_Surface* screen = SDL_SetVideoMode(1024, 768, 32, videoFlags);
    if ( !screen )
    {
        printf("Unable to set video: %s\n", SDL_GetError());
        return 1;
    }
	
    m_renderManager.Init(sc_colourBlack);
    m_renderManager.Resize(1024, 768, 32);

    // program main loop
    bool active = true;
    while (active)
    {
        // message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            active = m_inputManager.Update(event);
        }

        m_renderManager.DrawScene();

        // Cycle SDL scene
        SDL_GL_SwapBuffers();
    }

    m_log.Write(cSystemLog::LL_INFO, cSystemLog::LC_CORE, "Exited cleanly");

    return 0;
}
