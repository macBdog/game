#include <cstdio>
#include <cstdlib>

#include <windows.h>

#include <SDL.h>

#include "core/MathUtils.h"

#include "engine/CameraManager.h"
#include "engine/DebugMenu.h"
#include "engine/FontManager.h"
#include "engine/GameFile.h"
#include "engine/Gui.h"
#include "engine/InputManager.h"
#include "engine/Log.h"
#include "engine/ModelManager.h"
#include "engine/RenderManager.h"
#include "engine/StringUtils.h"
#include "engine/TextureManager.h"
#include "engine/WorldManager.h"

int main(int argc, char *argv[])
{
	// Single parameter to the executable is the main config file
	bool useRelativePaths = false;
	char configFilePath[StringUtils::s_maxCharsPerLine];
	char gameDataPath[StringUtils::s_maxCharsPerLine];
	memset(&configFilePath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	memset(&gameDataPath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
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
	GameFile configFile(configFilePath);
	if (!configFile.IsLoaded())
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Unable to load the main configuration file at %s", configFilePath);
		return 1;
	}

	// Setup relative pathing if defined
	if (configFile.GetString("config", "gameDataPath") != NULL)
	{
		sprintf(gameDataPath, "%s", configFile.GetString("config", "gameDataPath"));
		useRelativePaths = true;
	}

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
	bool fullScreen = configFile.GetBool("config", "fullscreen");
	if (fullScreen)
	{
		videoFlags |= SDL_FULLSCREEN;
	}

	// Create a new window
	int width = configFile.GetInt("config", "width");
	int height = configFile.GetInt("config", "height");
	int bpp = configFile.GetInt("config", "bpp");
    SDL_Surface* screen = SDL_SetVideoMode(width, height, bpp, videoFlags);
    if ( !screen )
    {
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Unable to set video: %s\n", SDL_GetError());
        return 1;
    }
	
	// Hide the mouse cursor
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_GrabInput(SDL_GRAB_ON);

	// Process resource paths
	char texturePath[StringUtils::s_maxCharsPerLine];
	char fontPath[StringUtils::s_maxCharsPerLine];
	char guiPath[StringUtils::s_maxCharsPerLine];
	char modelPath[StringUtils::s_maxCharsPerLine];
	char templatePath[StringUtils::s_maxCharsPerLine];
	char scenePath[StringUtils::s_maxCharsPerLine];
	char scriptPath[StringUtils::s_maxCharsPerLine];
	char shaderPath[StringUtils::s_maxCharsPerLine];

	strcpy(texturePath, configFile.GetString("config", "texturePath"));
	strcpy(fontPath, configFile.GetString("config", "fontPath"));
	strcpy(guiPath, configFile.GetString("config", "guiPath"));
	strcpy(modelPath, configFile.GetString("config", "modelPath"));
	strcpy(templatePath, configFile.GetString("config", "templatePath"));
	strcpy(scenePath, configFile.GetString("config", "scenePath"));
	strcpy(scriptPath,		configFile.GetString("config", "scriptPath"));
	strcpy(shaderPath, configFile.GetString("config", "shaderPath"));

	// Prefix paths that don't look explicit
	if (useRelativePaths)
	{
		if (strstr(texturePath, ":") == NULL)	{ StringUtils::PrependString(texturePath, gameDataPath); }
		if (strstr(fontPath, ":") == NULL)		{ StringUtils::PrependString(fontPath, gameDataPath); }
		if (strstr(guiPath, ":") == NULL)		{ StringUtils::PrependString(guiPath, gameDataPath); }
		if (strstr(modelPath, ":") == NULL)		{ StringUtils::PrependString(modelPath, gameDataPath); }
		if (strstr(templatePath, ":") == NULL)	{ StringUtils::PrependString(templatePath, gameDataPath); }
		if (strstr(scenePath, ":") == NULL)		{ StringUtils::PrependString(scenePath, gameDataPath); }
		if (strstr(scriptPath, ":") == NULL)	{ StringUtils::PrependString(scriptPath, gameDataPath); }
		if (strstr(shaderPath, ":") == NULL)	{ StringUtils::PrependString(shaderPath, gameDataPath); }
	}

	// Subsystem startup
	MathUtils::InitialiseRandomNumberGenerator();
    RenderManager::Get().Startup(sc_colourBlack, shaderPath);
    RenderManager::Get().Resize(width, height, bpp);
	TextureManager::Get().Startup(texturePath, configFile.GetBool("render", "textureFilter"));
	FontManager::Get().Startup(fontPath);
	Gui::Get().Startup(guiPath);
	InputManager::Get().Startup(fullScreen);
	ModelManager::Get().Startup(modelPath);
	WorldManager::Get().Startup(templatePath, scenePath);
	CameraManager::Get().Startup();

	// Oculus Rift support
	if (configFile.GetBool("render", "vr"))
	{
		RenderManager::Get().SetVrSupport(true);
	}

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

		// Update the camera first
		CameraManager::Get().Update(lastFrameTimeSec);

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
			FontManager::Get().DrawDebugString2D(buf, Vector2(0.85f, 1.0f));
		}

		// Drawing the scene will flush the batches
        RenderManager::Get().DrawScene(lastFrameTimeSec, CameraManager::Get().GetCameraMatrix());
		RenderManager::Get().Update(lastFrameTimeSec);

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
