#include <cstdio>
#include <cstdlib>

#include <windows.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include "core/MathUtils.h"

#include "engine/AnimationManager.h"
#include "engine/CameraManager.h"
#include "engine/DataPack.h"
#include "engine/DebugMenu.h"
#include "engine/FontManager.h"
#include "engine/GameFile.h"
#include "engine/Gui.h"
#include "engine/InputManager.h"
#include "engine/Log.h"
#include "engine/ModelManager.h"
#include "engine/OculusManager.h"
#include "engine/RenderManager.h"
#include "engine/ScriptManager.h"
#include "engine/StringUtils.h"
#include "engine/TextureManager.h"
#include "engine/WorldManager.h"
#include "engine/PhysicsManager.h"
#include "engine/SoundManager.h"

int main(int argc, char *argv[])
{
	// Single parameter to the executable is the main config file
	bool useRelativePaths = false;
	char configFilePath[StringUtils::s_maxCharsPerLine];
	char gameDataPath[StringUtils::s_maxCharsPerLine];
	configFilePath[0] = '\0';
	gameDataPath[0] = '\0';
	if (argc > 1)
	{
		memcpy(&configFilePath, argv, strlen(argv[1]));
		configFilePath[strlen(argv[1])] = '\0';
	}
	else // No argument specified, use a fallback
	{
		sprintf(configFilePath, "game.cfg");
	}

	// Make sure SDL cleans up before exit
	atexit(SDL_Quit);

	// Storage for resource paths
	char gameConfigPath[StringUtils::s_maxCharsPerLine];
	char texturePath[StringUtils::s_maxCharsPerLine];
	char fontPath[StringUtils::s_maxCharsPerLine];
	char guiPath[StringUtils::s_maxCharsPerLine];
	char modelPath[StringUtils::s_maxCharsPerLine];
	char templatePath[StringUtils::s_maxCharsPerLine];
	char scenePath[StringUtils::s_maxCharsPerLine];
	char scriptPath[StringUtils::s_maxCharsPerLine];
	char shaderPath[StringUtils::s_maxCharsPerLine];
	char soundPath[StringUtils::s_maxCharsPerLine];

	// For a release build, look for the datapack right next to the executable
#ifdef _RELEASE
	#define _DATAPACK 1
#endif
#ifdef _DATAPACK
	DataPack dataPack("datapack.dtp");
	
	if (!dataPack.IsLoaded())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load the datapack.");
		return 1;
	}

	GameFile configFile;
	DataPackEntry * configFileFromPack;
	if (configFileFromPack = dataPack.GetEntry(configFilePath))
	{
		configFile.Load(configFileFromPack);
	}
	else
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Datapack misc failure.");
	}
#else

	// Read the main config file to setup video etc
	GameFile configFile(configFilePath);
	if (!configFile.IsLoaded())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load the main configuration file at %s", configFilePath);
		return 1;
	}

	// All files read by the game will be added to the datapack in case we want to make a pack
	DataPack & dataPack = DataPack::Get();
	dataPack.AddFile(configFilePath);

	// Setup relative pathing if defined
	const char * gameDataPathFromFile = configFile.GetString("config", "gameDataPath");
	if (gameDataPathFromFile != NULL)
	{
		dataPack.AddFile(gameDataPathFromFile);
		sprintf(gameDataPath, "%s", gameDataPathFromFile);
		useRelativePaths = true;

		// Check to see if the dot operators are being used
		const char * executableName = "game.exe";
		const int partialLength = strlen(argv[0]) - strlen(executableName) - 1;
		if (strstr(gameDataPathFromFile, "..\\") != NULL)
		{
			char partialPath[StringUtils::s_maxCharsPerLine];
			partialPath[0] = '\0';
			strncpy(partialPath, argv[0], partialLength);
			partialPath[partialLength] = '\0';
			const char * lastSlash = strrchr(partialPath, '\\');
			const int pathLength = strlen(argv[0]) - strlen(executableName) - strlen(lastSlash);
			strncpy(gameDataPath, argv[0], pathLength);
			gameDataPath[pathLength] = '\0';
			sprintf(gameDataPath, "%s%s", gameDataPath, strstr(gameDataPathFromFile, "..\\") + 3);

		}
		else if (strstr(gameDataPathFromFile, ".\\") != NULL)
		{
			strncpy(gameDataPath, argv[0], partialLength);
			gameDataPath[partialLength] = '\0';
			sprintf(gameDataPath, "%s%s", gameDataPath, strstr(gameDataPathFromFile, ".\\") + 1);
		}
	}
#endif

	Log::Get().Write(LogLevel::Info, LogCategory::Engine, "GameData path is: %s", gameDataPath);

	strcpy(gameConfigPath, configFile.GetString("config", "gameConfigFile"));
	strcpy(texturePath, configFile.GetString("config", "texturePath"));
	strcpy(fontPath, configFile.GetString("config", "fontPath"));
	strcpy(guiPath, configFile.GetString("config", "guiPath"));
	strcpy(modelPath, configFile.GetString("config", "modelPath"));
	strcpy(templatePath, configFile.GetString("config", "templatePath"));
	strcpy(scenePath, configFile.GetString("config", "scenePath"));
	strcpy(scriptPath, configFile.GetString("config", "scriptPath"));
	strcpy(shaderPath, configFile.GetString("config", "shaderPath"));
	strcpy(soundPath, configFile.GetString("config", "soundPath"));

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
		if (strstr(soundPath, ":") == NULL)			{ StringUtils::PrependString(soundPath, gameDataPath); }
	}

	// Load game specific config
	GameFile gameConfig(gameConfigPath);
	if (!gameConfig.IsLoaded())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load the game specific configuration file at %s", configFilePath);
		return 1;
	}

#ifndef _DATAPACK
	dataPack.AddFile(gameConfigPath);
	dataPack.Serialize("datapack.dtp");
#endif

	// Initialize SDL video
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to init SDL");
		return 1;
	}

	// Oculus Rift init must precede render device context creation
	bool useVr = gameConfig.GetBool("render", "vr");
	if (useVr)
	{
		OculusManager::Get().Startup();
	}

	// The flags to pass to SDL_CreateWindow
	int videoFlags = SDL_WINDOW_OPENGL;

	// Set fullscreen from config, don't allow fullscreen for HMD as we are supporting DirectToRift
	bool fullScreen = configFile.GetBool("config", "fullscreen");
	if (fullScreen && !useVr)
	{
		videoFlags |= SDL_WINDOW_FULLSCREEN;
	}

	// Create a new window
	int width = configFile.GetInt("config", "width");
	int height = configFile.GetInt("config", "height");
	int bpp = configFile.GetInt("config", "bpp");

	// Override display creation for VR mode so the resoultion is correct for the hmd
	if (useVr)
	{
		width = OculusManager::s_hmdDefaultResolutionWidth;
		height = OculusManager::s_hmdDefaultResolutionHeight;
	}

	const char * gameName = configFile.GetString("config", "name");
	SDL_Window * sdlWindow = SDL_CreateWindow(gameName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, videoFlags);

	if (!sdlWindow)
    {
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to set video: %s\n", SDL_GetError());
        return 1;
    }

	// Create an OpenGL context associated with the window.
	SDL_GLContext glcontext = SDL_GL_CreateContext(sdlWindow);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	
	// Hide the mouse cursor
	SDL_SetRelativeMouseMode(SDL_TRUE);
	
	// Get window info to pass to managers that need the window handle
	SDL_SysWMinfo windowInfo;
	SDL_VERSION(&windowInfo.version);
	SDL_GetWindowWMInfo(sdlWindow, &windowInfo);

	Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Starting up subsystems");

	// Subsystem creation and startup
	MathUtils::InitialiseRandomNumberGenerator();
	RenderManager::Get().Startup(sc_colourBlack, shaderPath, useVr);
    RenderManager::Get().Resize(width, height, bpp);
	TextureManager::Get().Startup(texturePath, gameConfig.GetBool("render", "textureFilter"));
	FontManager::Get().Startup(fontPath);
	InputManager::Get().Startup(fullScreen);
	ModelManager::Get().Startup(modelPath);
	PhysicsManager::Get().Startup(gameConfig, modelPath);
	AnimationManager::Get().Startup(modelPath);
	WorldManager::Get().Startup(templatePath, scenePath);
	CameraManager::Get().Startup();
	SoundManager::Get().Startup(soundPath);
	
	// Now the rendering device context is created, it can be passed into the Oculus rendering component
	if (useVr)
	{
		OculusManager::Get().StartupRendering(windowInfo.info.win.window);
	}

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
		InputManager::Get().Update(lastFrameTimeSec);
        SDL_Event event;
        while (active && SDL_PollEvent(&event))
        {
            active = InputManager::Get().EventPump(event);
        }

		// Speed up or slow time down for debugging
		lastFrameTimeSec *= DebugMenu::Get().GetGameTimeScale();

		// Update the camera first
		CameraManager::Get().Update(lastFrameTimeSec);

		// Update world last as it clears physics collisions for the frame
		AnimationManager::Get().Update(lastFrameTimeSec);
		ModelManager::Get().Update(lastFrameTimeSec);
		PhysicsManager::Get().Update(lastFrameTimeSec);
		ScriptManager::Get().Update(lastFrameTimeSec);
		WorldManager::Get().Update(lastFrameTimeSec);
		SoundManager::Get().Update(lastFrameTimeSec);
		
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
		if (useVr)
		{
			OculusManager::Get().DrawToHMD();
		}
		else
		{
			RenderManager::Get().DrawToScreen(CameraManager::Get().GetCameraMatrix().GetInverse());
		}
		
		// Perform and post rendering tasks for subsystems
		DebugMenu::Get().PostRender();

		// Don't swap buffers when rendering to the HMD
		if (!useVr)
		{
			SDL_GL_SwapWindow(sdlWindow);
		}

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
	OculusManager::Get().Shutdown();
	DebugMenu::Get().Shutdown();
	Gui::Get().Shutdown();
	RenderManager::Get().Shutdown();
	InputManager::Get().Shutdown();
	SoundManager::Get().Shutdown();

	// Once finished with OpenGL functions, the SDL_GLContext can be deleted.
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(sdlWindow);
	SDL_Quit();

    return 0;
}

