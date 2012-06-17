#include <cstdio>
#include <cstdlib>

#include <windows.h>

#include <SDL.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "engine/FontManager.h"
#include "engine/GameFile.h"
#include "engine/InputManager.h"
#include "engine/Log.h"
#include "engine/RenderManager.h"
#include "engine/StringUtils.h"

InputManager m_inputManager;

int main(int argc, char *argv[])
{
	// Startup logging first so any initialisation errors are reported
	Log * m_log = new Log();

	// Single parameter to the executable is the main confi file
	char configFilePath[StringUtils::s_maxCharsPerLine];
	memset(&configFilePath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	if (argc > 1)
	{
		memcpy(&configFilePath, argv, strlen(argv[1]));
		configFilePath[strlen(argv[1])] = '\0';
	}
	else // No argument specified, use a fallback
	{
		sprintf(configFilePath, "game.cfg");
	}

	// Read the main config file to setup video etc
	GameFile * configFile = new GameFile();
	configFile->Load(configFilePath);

    // Initialize SDL video
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
		Log::Get().Write(Log::LL_ERROR, Log::LC_CORE, "Unable to init SDL");
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
	int width = configFile->GetInt("config", "width");
	int height = configFile->GetInt("config", "height");
	int bpp = configFile->GetInt("config", "bpp");
    SDL_Surface* screen = SDL_SetVideoMode(width, height, bpp, videoFlags);
    if ( !screen )
    {
        printf("Unable to set video: %s\n", SDL_GetError());
        return 1;
    }
	
	// Subsystem startup
	RenderManager * m_renderManager = new RenderManager();
    RenderManager::Get().Startup(sc_colourBlack);
    RenderManager::Get().Resize(width, height, bpp);

	FontManager * m_fontManager = new FontManager();
	FontManager::Get().Startup(configFile->GetString("config", "fontPath"));

    // Game main loop
	unsigned int lastFrameTime = 0;
	float lastFrameTimeSec = 0.0f;
	unsigned int frameCount = 0;
	unsigned int lastFps = 0;
	float fps = 0.0f;

    bool active = true;
    while (active)
    {
		// Start counting time
		unsigned int startFrame = Time::GetSystemTime();

        // Message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            active = m_inputManager.Update(event);
        }

		// Refresh log drawing on screen
		Log::Get().Update(lastFrameTimeSec);
		
		// Draw FPS on screen
		char buf[32];
		sprintf(buf, "FPS: %u", lastFps);
		FontManager::Get().DrawDebugString(buf, Vector2(0.87f, 1.0f));

		// Drawing the scene will flush the batches
        RenderManager::Get().DrawScene();

        // Cycle SDL surface
        SDL_GL_SwapBuffers();

		// Finished a frame, count time and calc FPS
		lastFrameTime = Time::GetSystemTime() - startFrame;
		lastFrameTimeSec = lastFrameTime / 1000.0f;
		if (fps > 1.0f) { lastFps = frameCount; frameCount = 0; fps = 0.0f; } else { ++frameCount;	fps+=lastFrameTimeSec; }
    }

    Log::Get().Write(Log::LL_INFO, Log::LC_CORE, "Exited cleanly");

    return 0;
}
