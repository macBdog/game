#include <cstdio>
#include <cstdlib>

#include <windows.h>

#include <SDL.h>

#include "core/MathUtils.h"

#include "engine/AnimationManager.h"
#include "engine/CameraManager.h"
#include "engine/DebugMenu.h"
#include "engine/FontManager.h"
#include "engine/GameFile.h"
#include "engine/Gui.h"
#include "engine/InputManager.h"
#include "engine/Log.h"
#include "engine/ModelManager.h"
#include "engine/RenderManager.h"
#include "engine/ScriptManager.h"
#include "engine/StringUtils.h"
#include "engine/TextureManager.h"
#include "engine/WorldManager.h"
#include "engine/PhysicsManager.h"

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
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load the main configuration file at %s", configFilePath);
		return 1;
	}

	// Setup relative pathing if defined
	const char * gameDataPathFromFile = configFile.GetString("config", "gameDataPath");
	if (gameDataPathFromFile != NULL)
	{
		sprintf(gameDataPath, "%s", gameDataPathFromFile);
		useRelativePaths = true;

		// Check to see if the dot operator is being used
		if (strstr(gameDataPathFromFile, ".\\") != NULL)
		{
			const char * executableName = "game.exe";
			strncpy(gameDataPath, argv[0], strlen(argv[0]) - strlen(executableName));
		}
	}

	Log::Get().Write(LogLevel::Info, LogCategory::Engine, "GameData path is: %s", gameDataPath);

    // Initialize SDL video
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to init SDL");
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
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to set video: %s\n", SDL_GetError());
        return 1;
    }
	
	// Hide the mouse cursor
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_GrabInput(SDL_GRAB_ON);
	

	// Process resource paths
	char gameConfigPath[StringUtils::s_maxCharsPerLine];
	char texturePath[StringUtils::s_maxCharsPerLine];
	char fontPath[StringUtils::s_maxCharsPerLine];
	char guiPath[StringUtils::s_maxCharsPerLine];
	char modelPath[StringUtils::s_maxCharsPerLine];
	char templatePath[StringUtils::s_maxCharsPerLine];
	char scenePath[StringUtils::s_maxCharsPerLine];
	char scriptPath[StringUtils::s_maxCharsPerLine];
	char shaderPath[StringUtils::s_maxCharsPerLine];

	strcpy(gameConfigPath,	configFile.GetString("config", "gameConfigFile"));
	strcpy(texturePath,		configFile.GetString("config", "texturePath"));
	strcpy(fontPath,		configFile.GetString("config", "fontPath"));
	strcpy(guiPath,			configFile.GetString("config", "guiPath"));
	strcpy(modelPath,		configFile.GetString("config", "modelPath"));
	strcpy(templatePath,	configFile.GetString("config", "templatePath"));
	strcpy(scenePath,		configFile.GetString("config", "scenePath"));
	strcpy(scriptPath,		configFile.GetString("config", "scriptPath"));
	strcpy(shaderPath,		configFile.GetString("config", "shaderPath"));

	// Prefix paths that don't look explicit
	if (useRelativePaths)
	{
		if (strstr(gameConfigPath, ":") == NULL)	{ StringUtils::PrependString(gameConfigPath, gameDataPath); }
		if (strstr(texturePath, ":") == NULL)		{ StringUtils::PrependString(texturePath, gameDataPath); }
		if (strstr(fontPath, ":") == NULL)			{ StringUtils::PrependString(fontPath, gameDataPath); }
		if (strstr(guiPath, ":") == NULL)			{ StringUtils::PrependString(guiPath, gameDataPath); }
		if (strstr(modelPath, ":") == NULL)			{ StringUtils::PrependString(modelPath, gameDataPath); }
		if (strstr(templatePath, ":") == NULL)		{ StringUtils::PrependString(templatePath, gameDataPath); }
		if (strstr(scenePath, ":") == NULL)			{ StringUtils::PrependString(scenePath, gameDataPath); }
		if (strstr(scriptPath, ":") == NULL)		{ StringUtils::PrependString(scriptPath, gameDataPath); }
		if (strstr(shaderPath, ":") == NULL)		{ StringUtils::PrependString(shaderPath, gameDataPath); }
	}

	// Load game specific config
	GameFile gameConfig(gameConfigPath);
	if (!gameConfig.IsLoaded())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load the game specific configuration file at %s", configFilePath);
		return 1;
	}

	Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Starting up subsystems");

	// Subsystem startup
	MathUtils::InitialiseRandomNumberGenerator();
    RenderManager::Get().Startup(sc_colourBlack, shaderPath, gameConfig.GetBool("render", "vr"));
    RenderManager::Get().Resize(width, height, bpp);
	TextureManager::Get().Startup(texturePath, gameConfig.GetBool("render", "textureFilter"));
	FontManager::Get().Startup(fontPath);
	InputManager::Get().Startup(fullScreen);
	ModelManager::Get().Startup(modelPath);
	PhysicsManager::Get().Startup(gameConfig);
	AnimationManager::Get().Startup(modelPath);
	WorldManager::Get().Startup(templatePath, scenePath);
	CameraManager::Get().Startup(gameConfig.GetBool("render", "vr"));
	Gui::Get().Startup(guiPath);
	ScriptManager::Get().Startup(scriptPath);
	
    // Game main loop
	unsigned int lastFrameTime = 0;
	float lastFrameTimeSec = 0.0f;
	unsigned int frameCount = 0;
	unsigned int lastFps = 0;
	float fps = 0.0f;

    bool active = true;

	Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Starting game loop");

    while (active)
    {
		// Start counting time
		unsigned int startFrame = Time::GetSystemTime();

        // Message processing loop
        SDL_Event event;
        while (active && SDL_PollEvent(&event))
        {
            active = InputManager::Get().Update(event);
        }

		// Speed up or slow time down for debugging
		lastFrameTimeSec *= DebugMenu::Get().GetGameTimeScale();

		// Update the camera first
		CameraManager::Get().Update(lastFrameTimeSec);

		// Update world last as it clears physics collisions for the frame
		AnimationManager::Get().Update(lastFrameTimeSec);
		PhysicsManager::Get().Update(lastFrameTimeSec);
		ScriptManager::Get().Update(lastFrameTimeSec);
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

		// Drawing the scene will flush the renderLayeres
		RenderManager::Get().Update(lastFrameTimeSec);
		
		// Only swap the buffers at the end of all the rendering passes
		RenderManager::Get().DrawToScreen(CameraManager::Get().GetCameraMatrix());
		
		// Perform and post rendering tasks for subsystems
		DebugMenu::Get().PostRender();

		SDL_GL_SwapBuffers();

		// Finished a frame, count time and calc FPS
		lastFrameTime = Time::GetSystemTime() - startFrame;
		lastFrameTimeSec = lastFrameTime / 1000.0f;
		if (fps > 1.0f) { lastFps = frameCount; frameCount = 0; fps = 0.0f; } else { ++frameCount;	fps+=lastFrameTimeSec; }
    }

	// Singletons are shutdown by their destructors
    Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Exited cleanly");

	// Shut everything down
	WorldManager::Get().Shutdown();
	PhysicsManager::Get().Shutdown();
	FontManager::Get().Shutdown();
	ScriptManager::Get().Shutdown();
	ModelManager::Get().Shutdown();
	TextureManager::Get().Shutdown();
	ScriptManager::Get().Shutdown();
	CameraManager::Get().Shutdown();
	DebugMenu::Get().Shutdown();
	Gui::Get().Shutdown();
	RenderManager::Get().Shutdown();
	InputManager::Get().Shutdown();

    return 0;
}

