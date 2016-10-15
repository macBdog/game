#include <cstdio>
#include <cstdlib>

#include <windows.h>

#include <SDL.h>
#include <SDL_syswm.h>

#ifndef _RELEASE
#include "Remotery.h"
#endif

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
	char titleConfigFilePath[StringUtils::s_maxCharsPerLine];
	char gameDataPath[StringUtils::s_maxCharsPerLine];
	titleConfigFilePath[0] = '\0';
	gameDataPath[0] = '\0';
	if (argc > 1)
	{
		memcpy(&titleConfigFilePath, argv, strlen(argv[1]));
		titleConfigFilePath[strlen(argv[1])] = '\0';
	}
	else // No argument specified, use a fallback
	{
		sprintf(titleConfigFilePath, "game.cfg");
	}

	// Make sure SDL cleans up before exit
	atexit(SDL_Quit);

	// Figure out where the exe is being run from to relative paths to the exe can be used
	const char * executableName = "game.exe";
	const int partialLength = strlen(argv[0]) - strlen(executableName) - 1;
	char partialPath[StringUtils::s_maxCharsPerLine];
	strncpy(partialPath, argv[0], partialLength);
	partialPath[partialLength] = '\0';

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
	DataPack & dataPack = DataPack::Get();
	dataPack.SetRelativePath(partialPath);

#ifdef _RELEASE
	#define _DATAPACK 1
#endif
#ifdef _DATAPACK
	dataPack.Load(DataPack::s_defaultDataPackPath);
	
	if (!dataPack.IsLoaded())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load the datapack.");
		return 1;
	}

	GameFile titleConfigFile;
	DataPackEntry * titleConfigFileFromPack;
	if (titleConfigFileFromPack = dataPack.GetEntry(titleConfigFilePath))
	{
		titleConfigFile.Load(titleConfigFileFromPack);
	}
	else
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Datapack misc failure.");
	}
#else

	// Read the main config file to setup video etc
	GameFile titleConfigFile(titleConfigFilePath);
	if (!titleConfigFile.IsLoaded())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load the main configuration file at %s", titleConfigFilePath);
		return 1;
	}

	// Setup relative pathing if defined
	const char * gameDataPathFromFile = titleConfigFile.GetString("config", "gameDataPath");
	if (gameDataPathFromFile != NULL)
	{
		sprintf(gameDataPath, "%s", gameDataPathFromFile);
		useRelativePaths = true;

		// Check to see if the dot operators are being used
		if (strstr(gameDataPathFromFile, "..\\") != NULL)
		{
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

	// All files read by the game will be added to the datapack in case we want to make a pack
	dataPack.SetRelativePath(gameDataPath);
	dataPack.AddFile(titleConfigFilePath);
#endif

	Log::Get().Write(LogLevel::Info, LogCategory::Engine, "GameData path is: %s", gameDataPath);

	strncpy(gameConfigPath, titleConfigFile.GetString("config", "gameConfigFile"), StringUtils::s_maxCharsPerLine);
	strncpy(texturePath, titleConfigFile.GetString("config", "texturePath"), StringUtils::s_maxCharsPerLine);
	strncpy(fontPath, titleConfigFile.GetString("config", "fontPath"), StringUtils::s_maxCharsPerLine);
	strncpy(guiPath, titleConfigFile.GetString("config", "guiPath"), StringUtils::s_maxCharsPerLine);
	strncpy(modelPath, titleConfigFile.GetString("config", "modelPath"), StringUtils::s_maxCharsPerLine);
	strncpy(templatePath, titleConfigFile.GetString("config", "templatePath"), StringUtils::s_maxCharsPerLine);
	strncpy(scenePath, titleConfigFile.GetString("config", "scenePath"), StringUtils::s_maxCharsPerLine);
	strncpy(scriptPath, titleConfigFile.GetString("config", "scriptPath"), StringUtils::s_maxCharsPerLine);
	strncpy(shaderPath, titleConfigFile.GetString("config", "shaderPath"), StringUtils::s_maxCharsPerLine);
	strncpy(soundPath, titleConfigFile.GetString("config", "soundPath"), StringUtils::s_maxCharsPerLine);

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
#ifdef _DATAPACK
	GameFile gameConfig(dataPack.GetEntry(gameConfigPath));
	if (!gameConfig.IsLoaded())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load the game specific configuration from the data pack.");
		return 1;
	}
#else
	GameFile gameConfig(gameConfigPath);
	if (!gameConfig.IsLoaded())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load the game specific configuration file at %s", gameConfigPath);
		return 1;
	}

	// Add all resources to the datapack so all files are written if the user hits the create datapack debug menu button
	dataPack.AddFile(gameConfigPath);
	dataPack.AddFolder(texturePath, ".tga");
	dataPack.AddFolder(fontPath, ".tga,.fnt");
	dataPack.AddFolder(guiPath, ".cfg,.mnu");
	dataPack.AddFolder(modelPath, ".obj,.mtl,.fbx");
	dataPack.AddFolder(templatePath, ".tmp");
	dataPack.AddFolder(scenePath, ".scn");
	dataPack.AddFolder(scriptPath, ".lua");
	dataPack.AddFolder(shaderPath, ".fsh,.vsh");
	dataPack.AddFolder(soundPath, ".wav,.mp3");
#endif

	// Initialize SDL video
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to init SDL");
		return 1;
	}

	// Oculus Rift init must precede render device context creation
	bool useVr = gameConfig.GetBool("render", "vr");
	OculusManager::Get().Startup(useVr);

	// The flags to pass to SDL_CreateWindow
	int videoFlags = SDL_WINDOW_OPENGL;

	// Set fullscreen from config, don't allow fullscreen for HMD as we are supporting DirectToRift
	bool fullScreen = titleConfigFile.GetBool("config", "fullscreen");
	if (fullScreen && !useVr)
	{
		videoFlags |= SDL_WINDOW_FULLSCREEN;
	}

	// Create a new window
	int width = titleConfigFile.GetInt("config", "width");
	int height = titleConfigFile.GetInt("config", "height");
	int bpp = titleConfigFile.GetInt("config", "bpp");

	// Override display creation for VR mode so the resoultion is correct for the hmd
	if (useVr)
	{
		const OculusManager& oculusMan = OculusManager::Get();
		if (oculusMan.IsInitialised())
		{
			width = oculusMan.GetHmdWidth();
			height = oculusMan.GetHmdHeight();
		}
		else
		{
			width = OculusManager::s_hmdDefaultResolutionWidth;
			height = OculusManager::s_hmdDefaultResolutionHeight;
		}
	}

	const char * gameName = titleConfigFile.GetString("config", "name");
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

	// Now the rendering device context is created, it can be passed into the Oculus rendering component
	OculusManager::Get().StartupRendering(&windowInfo.info.win.window, useVr);

#ifdef _DATAPACK
	RenderManager::Get().Startup(sc_colourBlack, shaderPath, &dataPack, useVr);
    RenderManager::Get().Resize(width, height, bpp);
	TextureManager::Get().Startup(texturePath, &dataPack, gameConfig.GetBool("render", "textureFilter"));
	FontManager::Get().Startup(fontPath, &dataPack);
	InputManager::Get().Startup(fullScreen);
	ModelManager::Get().Startup(modelPath, &dataPack);
	PhysicsManager::Get().Startup(gameConfig, modelPath, &dataPack);
	AnimationManager::Get().Startup(modelPath, &dataPack);
	WorldManager::Get().Startup(templatePath, scenePath, &dataPack);
	CameraManager::Get().Startup();
	SoundManager::Get().Startup(soundPath, &dataPack);
	Gui::Get().Startup(guiPath, &dataPack);
	ScriptManager::Get().Startup(scriptPath, &dataPack);
#else
	RenderManager::Get().Startup(sc_colourBlack, shaderPath, NULL, useVr);
	RenderManager::Get().Resize(width, height, bpp);
	TextureManager::Get().Startup(texturePath, gameConfig.GetBool("render", "textureFilter"));
	FontManager::Get().Startup(fontPath);
	InputManager::Get().Startup(fullScreen);
	ModelManager::Get().Startup(modelPath, NULL);
	PhysicsManager::Get().Startup(gameConfig, modelPath, NULL);
	AnimationManager::Get().Startup(modelPath, NULL);
	WorldManager::Get().Startup(templatePath, scenePath, NULL);
	CameraManager::Get().Startup();
	SoundManager::Get().Startup(soundPath, NULL);
	Gui::Get().Startup(guiPath, NULL);
	ScriptManager::Get().Startup(scriptPath, NULL);
#endif

	// Profiling macros do nothing in release builds
#ifdef _RELEASE									
#define PROFILE_STARTUP
#define PROFILE_SHUTDOWN
#define PROFILE_BEGIN
#define UPDATE_AND_PROFILE(System) System::Get().Update(lastFrameTimeSec);
#else
#define PROFILE_STARTUP									\
	Remotery * remoteProfile;							\
	rmt_CreateGlobalInstance(&remoteProfile);			\
	rmt_BindOpenGL();									\

#define PROFILE_SHUTDOWN								\
	rmt_UnbindOpenGL();									\
	rmt_DestroyGlobalInstance(remoteProfile);			

#define PROFILE_BEGIN									\
	rmt_ScopedCPUSample(GameLoop); 

#define UPDATE_AND_PROFILE(System)						\
	rmt_BeginCPUSample(System);							\
	System::Get().Update(lastFrameTimeSec);				\
	rmt_EndCPUSample();
#endif

	PROFILE_STARTUP;

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
		PROFILE_BEGIN;
		unsigned int startFrame = Time::GetSystemTime();
		
        // Message processing loop
		UPDATE_AND_PROFILE(InputManager);

        SDL_Event event;
        while (active && SDL_PollEvent(&event))
        {
            active = InputManager::Get().EventPump(event);
        }

		// Speed up or slow time down for debugging
		lastFrameTimeSec *= DebugMenu::Get().GetGameTimeScale();

		// Update the camera first
		UPDATE_AND_PROFILE(CameraManager);

		// Update world last as it clears physics collisions for the frame
		UPDATE_AND_PROFILE(AnimationManager);
		UPDATE_AND_PROFILE(ModelManager);
		UPDATE_AND_PROFILE(PhysicsManager);
		UPDATE_AND_PROFILE(ScriptManager);
		UPDATE_AND_PROFILE(WorldManager);
		UPDATE_AND_PROFILE(SoundManager);
		
		// Draw the Gui
		UPDATE_AND_PROFILE(Gui);
		
		// Draw the debug menu
		UPDATE_AND_PROFILE(DebugMenu);

		// Update the texture manager so it can do it's auto refresh of textures
		UPDATE_AND_PROFILE(TextureManager);

		// Draw log entries on top of the gui
		UPDATE_AND_PROFILE(Log);

		// Draw FPS on top of everything
		if (DebugMenu::Get().IsDebugMenuEnabled())
		{
			char buf[32];
			sprintf(buf, "FPS: %u", lastFps);
			FontManager::Get().DrawDebugString2D(buf, Vector2(0.85f, 1.0f));
		}

		// Drawing the scene will flush the renderLayers
		UPDATE_AND_PROFILE(RenderManager);
		
#ifndef _RELEASE
		rmt_ScopedOpenGLSample(OpenGL);
		rmt_BeginCPUSample(Draw);
		rmt_BeginOpenGLSample(DrawToScreen);
#endif

		// Only swap the buffers at the end of all the rendering passes
		if (useVr)
		{
			OculusManager::Get().DrawToHMD();
		}
		else
		{
			RenderManager::Get().DrawToScreen(CameraManager::Get().GetCameraMatrix().GetInverse());
		}

#ifndef _RELEASE
		rmt_EndOpenGLSample();
		rmt_EndCPUSample();
#endif

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

	PROFILE_SHUTDOWN

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
	SoundManager::Get().Shutdown();

	if (useVr)
	{
		OculusManager::Get().Shutdown();
	}

	// Once finished with OpenGL functions, the SDL_GLContext can be deleted.
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(sdlWindow);
	SDL_Quit();

	// Singletons are shutdown by their destructors
	Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Exited cleanly");
	Log::Get().Shutdown();

    return 0;
}
