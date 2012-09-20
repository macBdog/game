#include <cstdio>
#include <cstdlib>

#include <windows.h>

#include <SDL.h>

#include "engine/DebugMenu.h"
#include "engine/FontManager.h"
#include "engine/GameFile.h"
#include "engine/Gui.h"
#include "engine/InputManager.h"
#include "engine/Log.h"
#include "engine/RenderManager.h"
#include "engine/StringUtils.h"
#include "engine/TextureManager.h"
#include "engine/WorldManager.h"

int main(int argc, char *argv[])
{
	// Single parameter to the executable is the main config file
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
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Unable to init SDL");
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

	// Set fullscreen from config
	bool fullScreen = configFile->GetBool("config", "fullscreen");
	if (fullScreen)
	{
		videoFlags |= SDL_FULLSCREEN;
	}

	// Create a new window
	int width = configFile->GetInt("config", "width");
	int height = configFile->GetInt("config", "height");
	int bpp = configFile->GetInt("config", "bpp");
    SDL_Surface* screen = SDL_SetVideoMode(width, height, bpp, videoFlags);
    if ( !screen )
    {
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Unable to set video: %s\n", SDL_GetError());
        return 1;
    }
	
	// Hide the mouse cursor
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_GrabInput(SDL_GRAB_ON);

	// Subsystem startup
    RenderManager::Get().Startup(sc_colourBlack);
    RenderManager::Get().Resize(width, height, bpp);
	TextureManager::Get().Startup(configFile->GetString("config", "texturePath"), configFile->GetBool("render", "textureFilter"));
	FontManager::Get().Startup(configFile->GetString("config", "fontPath"));
	Gui::Get().Startup(configFile->GetString("config", "guiPath"));
	InputManager::Get().Startup(fullScreen);
	WorldManager::Get().Startup(configFile->GetString("config", "templatePath"));

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
            active = InputManager::Get().Update(event);
        }

		// Update the world first, other systems rely on object positions/states etc
		WorldManager::Get().Update(lastFrameTimeSec);
		
		// Draw the Gui
		Gui::Get().Update(lastFrameTimeSec);
		
		// Draw the debug menu
		DebugMenu::Get().Update(lastFrameTimeSec);

		// Update the texture manager so it can do it's auto refresh of textures
		TextureManager::Get().Update(lastFrameTimeSec);

		// Draw log entries on top of the gui
		Log::Get().Update(lastFrameTimeSec);

		// Draw FPS on top of everything
		if (DebugMenu::Get().IsDebugMenuEnabled())
		{
			char buf[32];
			sprintf(buf, "FPS: %u", lastFps);
			FontManager::Get().DrawDebugString(buf, Vector2(0.87f, 1.0f));
		}
		
		// Drawing the scene will flush the batches
        RenderManager::Get().DrawScene();

        // Cycle SDL surface
        SDL_GL_SwapBuffers();

		// Finished a frame, count time and calc FPS
		lastFrameTime = Time::GetSystemTime() - startFrame;
		lastFrameTimeSec = lastFrameTime / 1000.0f;
		if (fps > 1.0f) { lastFps = frameCount; frameCount = 0; fps = 0.0f; } else { ++frameCount;	fps+=lastFrameTimeSec; }
    }

	// Singletons are shutdown by their destructors
    Log::Get().Write(Log::LL_INFO, Log::LC_ENGINE, "Exited cleanly");

    return 0;
}
