#include <cstdio>
#include <cstdlib>

#include <windows.h>

#include <SDL.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "core/Log.h"

#include "engine/GameFile.h"
#include "engine/InputManager.h"
#include "engine/RenderManager.h"

Log m_log;
InputManager m_inputManager;

int main(int argc, char *argv[])
{
	// Unused arguments
	argc;
	argv;

    // Initialize SDL video
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Unable to init SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // The flags to pass to SDL_SetVideoMode */
    int videoFlags  = SDL_OPENGL;         
    videoFlags |= SDL_GL_DOUBLEBUFFER; 
    videoFlags |= SDL_HWPALETTE;
    videoFlags |= SDL_RESIZABLE;

    // This checks to see if surfaces can be stored in memory
    const SDL_VideoInfo * videoInfo = SDL_GetVideoInfo();

    if ( videoInfo->hw_available )
    {
        videoFlags |= SDL_HWSURFACE;
    }
    else
    {
        videoFlags |= SDL_SWSURFACE;
    }

    // This checks if hardware blits can be done
    if ( videoInfo->blit_hw )
    {
        videoFlags |= SDL_HWACCEL;
    }

    // Sets up OpenGL double buffering
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    // Create a new window
    SDL_Surface* screen = SDL_SetVideoMode(1024, 768, 32, videoFlags);
    if ( !screen )
    {
        printf("Unable to set video: %s\n", SDL_GetError());
        return 1;
    }
	
	// Subsystem startup
	RenderManager * m_renderManager = new RenderManager();
    m_renderManager->Init(sc_colourBlack);
    m_renderManager->Resize(1024, 768, 32);

	// Load game options
	GameFile * configFile = new GameFile();
	configFile->Load("config.cfg");

	// Render a test quad
	Texture * tex = new Texture();
	tex->Load(configFile->GetString("gameOptions", "texturePath"));
	m_renderManager->AddQuad(RenderManager::eBatchNone, Vector(0.0f, 0.0f, 0.0f), 0.5f, 0.5f, tex);

    // Game main loop
    bool active = true;
    while (active)
    {
        // message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            active = m_inputManager.Update(event);
        }

        m_renderManager->DrawScene();

        // Cycle SDL surface
        SDL_GL_SwapBuffers();
    }

    m_log.Write(Log::LL_INFO, Log::LC_CORE, "Exited cleanly");

    return 0;
}
