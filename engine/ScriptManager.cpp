#ifdef _WIN32
#include <windows.h>
#endif

#include "../core/Vector.h"
#include "../core/Matrix.h"
#include "../core/Quaternion.h"

#include "AnimationManager.h"
#include "CameraManager.h"
#include "DataPack.h"
#include "DebugMenu.h"
#include "FontManager.h"
#include "InputManager.h"
#include "LuaScript.h"
#include "ModelManager.h"
#include "VRManager.h"
#include "VRRender.h"
#include "PhysicsManager.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "TextureManager.h"
#include "WorldManager.h"

#include "ScriptManager.h"

template<> ScriptManager * Singleton<ScriptManager>::s_instance = nullptr;

const float ScriptManager::s_updateFreq = 1.0f;								///< How often the script manager should check for updates to scripts
const char * ScriptManager::s_mainScriptName = "game.lua";					///< Constant name of the main game script file

// Registration of GUI functions
const luaL_Reg ScriptManager::s_guiFuncs[] = {
	{"GetValue", GUIGetValue},
	{"SetValue", GUISetValue},
	{"SetFont", GUISetFont},
	{"SetFontSize", GUISetFontSize},
	{"SetColour", GUISetColour},
	{"SetSize", GUISetSize},
	{"SetScissor", GUISetScissor},
	{"SetTexture", GUISetTexture},
	{"Show", GUIShowWidget},
	{"Hide", GUIHideWidget},
	{"Activate", GUIActivateWidget},
	{"GetSelected", GUIGetSelectedWidget},
	{"SetSelected", GUISetSelectedWidget},
	{"SetActiveMenu", GUISetActiveMenu},
	{"EnableMouse", GUIEnableMouse},
	{"DisableMouse", GUIDisableMouse},
	{"GetMouseClipPosition", GUIGetMouseClipPosition},
	{"GetMouseScreenPosition", GUIGetMouseScreenPosition},
	{"GetMouseDirection", GUIGetMouseDirection},
	{"SetMousePosition", GUISetMousePosition},
	{nullptr, nullptr}
};

// Registration of game object functions
const luaL_Reg ScriptManager::s_gameObjectFuncs[] = {
	{"Create", CreateGameObject},
	{"Get", GetGameObject},
	{nullptr, nullptr}
};

// Registration of game object class member functions
const luaL_Reg ScriptManager::s_gameObjectMethods[] = {
	{"GetId", GetGameObjectId},
	{"GetName", GetGameObjectName},
	{"SetName", SetGameObjectName},
	{"SetModel", SetGameObjectModel},
	{"SetShaderData", SetGameObjectShaderData},
	{"GetPosition", GetGameObjectPosition},
	{"SetPosition", SetGameObjectPosition},
	{"GetRotation", GetGameObjectRotation},
	{"SetRotation", SetGameObjectRotation},
	{"SetLookAt", SetGameObjectLookAt},
	{"SetAttachedTo", SetAttachedTo },
	{"SetAttachedToCamera", SetAttachedToCamera },
	{"GetScale", GetGameObjectScale},
	{"SetScale", SetGameObjectScale},
	{"MultiplyScale", MultiplyGameObjectScale },
	{"ResetScale", ResetGameObjectScale},
	{"GetLifeTime", GetGameObjectLifeTime},
	{"SetLifeTime", SetGameObjectLifeTime},
	{"SetSleeping", SetGameObjectSleeping},
	{"SetActive", SetGameObjectActive},
	{"IsSleeping", GetGameObjectSleeping},
	{"IsActive", GetGameObjectActive},
	{"SetDiffuseTexture", SetGameObjectDiffuseTexture},
	{"SetNormalTexture", SetGameObjectNormalTexture},
	{"SetSpecularTexture", SetGameObjectSpecularTexture},
	{"SetVisible", SetGameObjectVisibility},
	{"EnableCollision", EnableGameObjectCollision},
	{"DisableCollision", DisableGameObjectCollision},
	{"AddToCollisionWorld", AddGameObjectToCollisionWorld},
	{"SetClipSize", SetGameObjectClipSize},
	{"AddToPhysicsWorld", AddGameObjectToPhysicsWorld},
	{"ApplyPhysicsForce", ApplyGameObjectPhysicsForce},
	{"GetVelocity", GetGameObjectVelocity},
	{"HasCollisions", TestGameObjectCollisions},
	{"GetCollisions", GetGameObjectCollisions},
	{"GetRayCollision", RayCollisionTest},
	{"PlayAnimation", PlayGameObjectAnimation},
	{"GetTransformedPos", GetGameObjectTransformedPos},
	{"Destroy", DestroyGameObject},
	{nullptr, nullptr}
};

// Registration of render functions
const luaL_Reg ScriptManager::s_renderFuncs[] = {
	{ "SetShader", RenderSetShader },
	{ "SetShaderData", RenderSetShaderData },
	{ "Quad", RenderQuad },
	{ "Tri", RenderTri },
	{ nullptr, nullptr }
};

bool ScriptManager::Startup(const char * a_scriptPath, const DataPack * a_dataPack)
{
	// As this method is called when scripts reload, make sure the global state is dead
	if (m_globalLua != nullptr || m_managedScripts.GetLength() > 0)
	{
		Shutdown();
	}
	
	// Open Lua and load the standard libraries
	if (m_globalLua = luaL_newstate())
	{
		luaL_requiref(m_globalLua, "base", luaopen_base, 1);
		luaL_requiref(m_globalLua, "coroutine", luaopen_coroutine, 1);
		luaL_requiref(m_globalLua, "io", luaopen_io, 1);
		luaL_requiref(m_globalLua, "os", luaopen_os, 1);
		luaL_requiref(m_globalLua, "table", luaopen_table, 1);
		luaL_requiref(m_globalLua, "string", luaopen_string, 1);
		luaL_requiref(m_globalLua, "math", luaopen_math, 1);
		luaL_requiref(m_globalLua, "package", luaopen_package, 1);
		luaL_requiref(m_globalLua, "debug", luaopen_debug, 1);

		// Register C++ functions made accessible to LUA
		if (a_dataPack != nullptr)
		{
			lua_register(m_globalLua, "require", DataPackRequire);
		}
		lua_register(m_globalLua, "Quit", Quit);
		lua_register(m_globalLua, "GetFrameDelta", GetFrameDelta);
		lua_register(m_globalLua, "CreateGameObject", CreateGameObject);
		lua_register(m_globalLua, "IsVR", IsVR);
		lua_register(m_globalLua, "GetVRLookDirection", GetVRLookDirection);
		lua_register(m_globalLua, "GetVRLookPosition", GetVRLookPosition);
		lua_register(m_globalLua, "IsKeyDown", IsKeyDown);
		lua_register(m_globalLua, "IsGamePadConnected", IsGamePadConnected);
		lua_register(m_globalLua, "IsGamePadButtonDown", IsGamePadButtonDown);
		lua_register(m_globalLua, "GetGamePadLeftStick", GetGamePadLeftStick);
		lua_register(m_globalLua, "GetGamePadRightStick", GetGamePadRightStick);
		lua_register(m_globalLua, "GetGamePadLeftTrigger", GetGamePadLeftTrigger);
		lua_register(m_globalLua, "GetGamePadRightTrigger", GetGamePadRightTrigger);
		lua_register(m_globalLua, "IsMouseLeftButtonDown", IsMouseLeftButtonDown);
		lua_register(m_globalLua, "IsMouseRightButtonDown", IsMouseRightButtonDown);
		lua_register(m_globalLua, "IsMouseMiddleButtonDown", IsMouseMiddleButtonDown);
		lua_register(m_globalLua, "SetCameraPosition", SetCameraPosition);
		lua_register(m_globalLua, "SetCameraRotation", SetCameraRotation);
		lua_register(m_globalLua, "SetCameraFOV", SetCameraFOV);
		lua_register(m_globalLua, "SetCameraTarget", SetCameraTarget);
		lua_register(m_globalLua, "MoveCamera", MoveCamera);
		lua_register(m_globalLua, "RotateCamera", RotateCamera);
		lua_register(m_globalLua, "GetRayCollision", RayCollisionTest);
		lua_register(m_globalLua, "NewScene", NewScene);
		lua_register(m_globalLua, "SetScene", SetScene);
		lua_register(m_globalLua, "SetLightAmbient", SetLightAmbient);
		lua_register(m_globalLua, "SetLightDiffuse", SetLightDiffuse);
		lua_register(m_globalLua, "SetLightSpecular", SetLightSpecular);
		lua_register(m_globalLua, "SetLightPosition", SetLightPosition);
		lua_register(m_globalLua, "CreateParticleEmitter", CreateParticleEmitter);
		lua_register(m_globalLua, "MoveParticleEmitter", MoveParticleEmitter);
		lua_register(m_globalLua, "DestroyParticleEmitter", DestroyParticleEmitter);

		lua_register(m_globalLua, "Draw3DText", Draw3DText);

		lua_register(m_globalLua, "GetNumCPUCores", GetNumCPUCores);
		lua_register(m_globalLua, "GetStorageDrives", GetStorageDrives);
		lua_register(m_globalLua, "GetDirectoryListing", GetDirectoryListing);
		lua_register(m_globalLua, "GetFileBytes", GetFileBytes);

		lua_register(m_globalLua, "PlaySound", PlaySoundFX);
		lua_register(m_globalLua, "PlaySound3D", PlaySoundFX3D);
		lua_register(m_globalLua, "PlayMusic", PlayMusic);
		lua_register(m_globalLua, "StopMusic", StopMusic);
		lua_register(m_globalLua, "SetMusicVolume", SetMusicVolume);
		lua_register(m_globalLua, "SetListenerPosition", SetListenerPosition);
		lua_register(m_globalLua, "StopAllSoundsAndMusic", StopAllSoundsAndMusic);
		lua_register(m_globalLua, "Yield", YieldLuaEnvironment);
		lua_register(m_globalLua, "DebugPrint", DebugPrint);
		lua_register(m_globalLua, "DebugLog", DebugLog);
		lua_register(m_globalLua, "DebugLine", DebugLine);
		lua_register(m_globalLua, "DebugPoint", DebugPoint);
		lua_register(m_globalLua, "IsDebugMenuActive", IsDebugMenuActive);

		// Register C++ functions available on the global GUI
		lua_newtable(m_globalLua);
		luaL_setfuncs(m_globalLua, s_guiFuncs, 0);
		lua_setglobal(m_globalLua, "GUI");

		// Register C++ functions available on the global GameObject
		lua_newtable(m_globalLua);
		luaL_setfuncs(m_globalLua, s_gameObjectFuncs, 0);
		lua_setglobal(m_globalLua, "GameObject");

		// Register C++ functions available on the global Render table
		lua_newtable(m_globalLua);
		luaL_setfuncs(m_globalLua, s_renderFuncs, 0);
		lua_setglobal(m_globalLua, "Render");
		
		// Register metatable for user data in registry
		luaL_newmetatable(m_globalLua, "GameObject.Registry");
		lua_pushstring(m_globalLua, "__index");
		lua_pushvalue(m_globalLua, -2);
		lua_settable(m_globalLua, -3);

		// Register C++ methods for game object members referenced with : in LUA
		luaL_setfuncs(m_globalLua, s_gameObjectMethods, 0);
		
		// Cache off path and look for the main game lua file
		strncpy(m_scriptPath, a_scriptPath, sizeof(char) * strlen(a_scriptPath) + 1);

		// Construct a string for the main script file 
		char gameScriptPath[StringUtils::s_maxCharsPerLine];
		gameScriptPath[0] = '\0';
		sprintf(gameScriptPath, "%s%s", m_scriptPath, s_mainScriptName);

		// Register the game's script folder in the lua package path so requires work correctly
		lua_getglobal(m_globalLua, "package");
		lua_getfield(m_globalLua, -1, "path");

		char newLuaPath[StringUtils::s_maxCharsPerLine * 2];
		const char * currentPackagePath = lua_tostring(m_globalLua, -1);
		sprintf(newLuaPath, "%s;%s\?.lua", currentPackagePath, m_scriptPath);
		lua_pop(m_globalLua, 1);
		lua_pushstring(m_globalLua, &newLuaPath[0]);
		lua_setfield(m_globalLua, -2, "path");
		lua_pop(m_globalLua, 1);

		// Load the main script from disk or the data pack
		m_gameLua = lua_newthread(m_globalLua);
		const bool readFromDataPack = a_dataPack != nullptr && a_dataPack->IsLoaded();
		if (readFromDataPack)
		{
			if (DataPackEntry * mainScriptEntry = a_dataPack->GetEntry(gameScriptPath))
			{
				luaL_loadbuffer(m_gameLua, mainScriptEntry->m_data, mainScriptEntry->m_size, gameScriptPath);
			}
		}
		else
		{
			luaL_loadfile(m_gameLua, gameScriptPath);
		}

		// Kick it off and wait for a yield
		int yieldResult = lua_resume(m_gameLua, nullptr, 0);
		if (yieldResult != LUA_YIELD)
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Game, "Fatal script error: %s\n", lua_tostring(m_gameLua, -1));
			Shutdown();
			return false;
		}

		if (readFromDataPack)
		{
			// Add all scripts in the pack?
		}
		else
		{
			// Scan all the scripts in the dir for changes to trigger a reload
			FileManager & fileMan = FileManager::Get();
			FileManager::FileList scriptFiles;
			if (!fileMan.FillFileList(m_scriptPath, scriptFiles, ".lua"))
			{
				m_scriptPath[0] = '\0';
			}

			// Add each script in the directory
			FileManager::FileListNode * curNode = scriptFiles.GetHead();
			while(curNode != nullptr)
			{
				// Get a fresh timestamp on the new script
				char fullPath[StringUtils::s_maxCharsPerLine];
				sprintf(fullPath, "%s%s", m_scriptPath, curNode->GetData()->m_name);
				FileManager::Timestamp curTimeStamp;
				FileManager::Get().GetFileTimeStamp(fullPath, curTimeStamp);

				// Add managed script to the list
				ManagedScript * manScript = new ManagedScript(fullPath, curTimeStamp);
				ManagedScriptNode * manScriptNode = new ManagedScriptNode();
				if (manScript != nullptr && manScriptNode != nullptr)
				{
					manScriptNode->SetData(manScript);
					m_managedScripts.Insert(manScriptNode);
				}
				curNode = curNode->GetNext();
			}

			// Clean up the list of fonts
			fileMan.CleanupFileList(scriptFiles);
		}
	}

	return m_globalLua != nullptr;
}

bool ScriptManager::Shutdown()
{
	// Clean up global lua object which will clean up the game thread
	if (m_globalLua != nullptr)
	{
		lua_close(m_globalLua);
		m_globalLua = nullptr;
		m_gameLua = nullptr;
	}

	// And any managed scripts
	m_managedScripts.ForeachAndDelete([&](auto cur)
	{
		m_managedScripts.Remove(cur);
	});
	
	return true;
}

bool ScriptManager::Update(float a_dt)
{
	// Cache off last delta
	m_lastFrameDelta = a_dt;

#ifdef _RELEASE
	// Call back to LUA main thread
	if (m_gameLua != nullptr)
	{
		lua_resume(m_gameLua, nullptr, 0);
		return true;
	}
#endif

	// Check if a script needs updating
	if (m_updateTimer < m_updateFreq)
	{
		m_updateTimer += a_dt;
	}
	else // Due for an update, scan all scripts
	{
		m_updateTimer = 0.0f;
		bool scriptsReloaded = false;

		// Test all scripts for modification
		FileManager & fileMan = FileManager::Get();
		ManagedScriptNode * next = m_managedScripts.GetHead();
		if (m_managedScripts.GetLength() == 0 && strlen(m_scriptPath) > 0 && fileMan.CheckFilePath(m_scriptPath))
		{
			// There are no scripts to manage, we must have bailed out while starting up, try again
			Startup(m_scriptPath, nullptr);
		}
		else
		{
			while (next != nullptr)
			{
				// Get a fresh timestamp and test it against the stored timestamp
				FileManager::Timestamp curTimeStamp;
				ManagedScript * curScript = next->GetData();
				fileMan.GetFileTimeStamp(curScript->m_path, curTimeStamp);
				if (curTimeStamp > curScript->m_timeStamp || m_forceReloadScripts)
				{
					if (m_forceReloadScripts)
					{
						Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Reloading scripts.");
					}
					else
					{
						Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in script %s, reloading.", curScript->m_path);
					}

					m_forceReloadScripts = false;
					scriptsReloaded = true;
					curScript->m_timeStamp = curTimeStamp;

					// Clean up any script-owned objects including physics
					WorldManager::Get().DestroyAllScriptOwnedObjects();

					// Stop any music that has been playing
					SoundManager::Get().StopAllSoundsAndMusic();

					// Remove anything with state from rendering
					RenderManager::Get().DestroyAllParticleEmitters();

					Gui::Get().ReloadMenus();

					// Kick the script VM in the guts
					Shutdown();
					Startup(m_scriptPath, nullptr);
					return true;
				}

				next = next->GetNext();
			}
		}
	}

	// Don't update scripts if time paused
	if (DebugMenu::Get().IsTimePaused())
	{
		return true;
	}

	// Don't update scripts while debugging
	if (DebugMenu::Get().IsDebugMenuEnabled() && DebugMenu::Get().IsTimePaused())
	{
		return true;
	}

	// Call back to LUA main thread
	if (m_gameLua != nullptr)
	{
		int yieldResult = lua_resume(m_gameLua, nullptr, 0);
		if (yieldResult != LUA_YIELD)
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Game, "Fatal script error: %s\n", lua_tostring(m_gameLua, -1));
			ReloadScripts();
			return false;
		}
		return true;
	}
	return false;
}

void ScriptManager::LogScriptError(lua_State * a_luaState, const char * a_callingFunctionName, const char * a_message)
{
	// Extract the source LUA file and line number to tell the user
	lua_Debug ar;
	lua_getstack(a_luaState, 1, &ar);
	lua_getinfo(a_luaState, "nSl", &ar);
	Log::Get().Write(LogLevel::Error, LogCategory::Game, "Script error in %s:%d, %s %s", ar.short_src, ar.currentline, a_callingFunctionName, a_message);
}

void ScriptManager::DestroyObjectScriptBindings(GameObject * a_gameObject)
{
	// Called when a GameObject is destroyed by the engine
	if (a_gameObject->GetScriptReference() > 0) 
	{
		lua_pushinteger(m_globalLua, a_gameObject->GetScriptReference());
		lua_pushnil(m_globalLua);
		lua_settable(m_globalLua, LUA_REGISTRYINDEX);
		a_gameObject->SetScriptReference(-1);
	}
}

bool ScriptManager::OnWidgetAction(Widget * a_widget)
{
	lua_getglobal(m_gameLua, a_widget->GetScriptFuncName());
	lua_call(m_gameLua, 0, 0);
	return true;
}

int ScriptManager::YieldLuaEnvironment(lua_State * a_luaState)
{
	 return lua_yield(a_luaState, 0);
}

int ScriptManager::Quit(lua_State * a_luaState)
{
	InputManager::Get().Quit();
	return 1;
}

int ScriptManager::GetFrameDelta(lua_State * a_luaState)
{
	lua_pushnumber(a_luaState, ScriptManager::Get().m_lastFrameDelta);

	return 1;
}

int ScriptManager::RegisterGameObject(lua_State * a_luaState)
{  
	luaL_newlib(a_luaState, s_gameObjectFuncs);
	return 1;	
}

GameObject * ScriptManager::CheckGameObject(lua_State * a_luaState, unsigned int a_argumentId)
{
	// Retrieve the object from a userdata reference
	luaL_checktype(a_luaState, a_argumentId, LUA_TUSERDATA);
	unsigned int * objId = (unsigned int*)(lua_touserdata(a_luaState, a_argumentId));
	return WorldManager::Get().GetGameObject(*objId);	
}

int ScriptManager::CreateGameObject(lua_State * a_luaState)
{
	if (a_luaState == nullptr)
	{
		return -1;
	}

	int numArgs = lua_gettop(a_luaState);
    if (numArgs < 2) 
	{
        return luaL_error(a_luaState, "GameObject:Create error, expecting at least 2 arguments: class (before the scope operator) then template name."); 
	}

	// Qualify template path with template extension
	const char * templateName = luaL_checkstring(a_luaState, 2);
	char templatePath[StringUtils::s_maxCharsPerName];
	templatePath[0] = '\n';
	if (strstr(templateName, ".tmp") == nullptr)
	{
		sprintf(templatePath, "%s.tmp", templateName);
	}
	else // Just copy chars over from LUA string
	{
		strncpy(templatePath, templateName, StringUtils::s_maxCharsPerName);
	}

	// Get the scene to add to if specified
	WorldManager & worldMan = WorldManager::Get();
	Scene * sceneToAddTo = nullptr;
	if (numArgs == 3)
	{
		const char * sceneName = luaL_checkstring(a_luaState, 3);
		sceneToAddTo = worldMan.GetScene(sceneName);
	}

	// Create the new object
	if (GameObject * newGameObject = worldMan.CreateObject(templatePath, sceneToAddTo))
	{
		// Allocate memory for an push userdata onto the stack
		unsigned int * userData = (unsigned int*)lua_newuserdata(a_luaState, sizeof(unsigned int));
		*userData = newGameObject->GetId();

		// Push the metatable where gameobject functions are stored onto the stack
		luaL_getmetatable(a_luaState, "GameObject.Registry");

		// Pop the metatable from the stack and sets it as the metatable for the userdata (at index -2)
		lua_setmetatable(a_luaState, -2);

		// Push a copy of the userdata with the attached metatable onto the stack
		lua_pushvalue(a_luaState, -1);

		// Create a reference in the registry for the object at the top of the stack (our userdata with metatable)
		int ref = luaL_ref(a_luaState, LUA_REGISTRYINDEX);
		newGameObject->SetScriptReference(ref);
	}

	return 1; // Userdata with metatable at the top of the stack is returned
}

int ScriptManager::GetGameObject(lua_State * a_luaState)
{
	if (a_luaState == nullptr)
	{
		return -1;
	}

	// Make sure an id has been supplied
	int numArgs = lua_gettop(a_luaState);
    if (numArgs != 2) 
	{
        LogScriptError(a_luaState, "GetGameObject", "GameObject:Get error, expecting 2 argument: class (before the scope operator) then name or ID of the game object.");
		lua_pushnil(a_luaState);
		return 1;
	}  

	// Get the object by name or ID
	GameObject * gameObj = nullptr;
	size_t stringLen  = 0;
	const char * objName = luaL_checklstring(a_luaState, 2, &stringLen);
	if (objName != nullptr)
	{
		gameObj = WorldManager::Get().GetGameObject(objName);
	}
	else
	{
		gameObj = WorldManager::Get().GetGameObject((unsigned int)lua_tonumber(a_luaState, 1));
	}

	if (gameObj != nullptr)
	{
		// Allocate memory for an push userdata onto the stack
		unsigned int * userData = (unsigned int*)lua_newuserdata(a_luaState, sizeof(unsigned int));
		*userData = gameObj->GetId();

		// Push the metatable where gameobject functions are stored onto the stack
		luaL_getmetatable(a_luaState, "GameObject.Registry");

		// Pop the metatable from the stack and sets it as the metatable for the userdata (at index -2)
		lua_setmetatable(a_luaState, -2);

		// Push a copy of the userdata with the attached metatable onto the stack
		lua_pushvalue(a_luaState, -1);

		// Create a reference in the registry for the object at the top of the stack (our userdata with metatable) and pop it
		int ref = luaL_ref(a_luaState, LUA_REGISTRYINDEX);

		return 1; // Userdata with metatable at the top of the stack is returned
	}
	else
	{
		LogScriptError(a_luaState, "GetGameObject", "GameObject:Get error, could not find an object with specified name or ID.");
		lua_pushnil(a_luaState);
		return 1;
	}
}

int ScriptManager::IsVR(lua_State * a_luaState)
{
	bool keyIsDown = false;
	if (lua_gettop(a_luaState) == 0)
	{
		return VRManager::Get().IsInitialised();
	}
	else
	{
		LogScriptError(a_luaState, "IsVR", "expects no parameters.");
	}

	lua_pushboolean(a_luaState, keyIsDown);
	return 1; // One bool returned
}

int ScriptManager::GetVRLookDirection(lua_State * a_luaState)
{
	Vector lookDir(0.0f);
	if (lua_gettop(a_luaState) == 0)
	{
		VRManager& vrMan = VRManager::Get();
		if (vrMan.IsInitialised())
		{
			lookDir = vrMan.GetVRRender()->GetLookDir();
		}
	}
	else
	{
		LogScriptError(a_luaState, "GetVRLookDirection", "expects no parameters.");
	}

	lua_pushnumber(a_luaState, lookDir.GetX());
	lua_pushnumber(a_luaState, lookDir.GetY());
	lua_pushnumber(a_luaState, lookDir.GetZ());
	return 3;
}

int ScriptManager::GetVRLookPosition(lua_State * a_luaState)
{
	Vector lookDir(0.0f);
	if (lua_gettop(a_luaState) == 0)
	{
		VRManager& vrMan = VRManager::Get();
		if (vrMan.IsInitialised())
		{
			lookDir = vrMan.GetVRRender()->GetLookPos();
		}
	}
	else
	{
		LogScriptError(a_luaState, "GetVRLookPosition", "expects no parameters.");
	}

	lua_pushnumber(a_luaState, lookDir.GetX());
	lua_pushnumber(a_luaState, lookDir.GetY());
	lua_pushnumber(a_luaState, lookDir.GetZ());
	return 3;
}

int ScriptManager::IsKeyDown(lua_State * a_luaState)
{
	bool keyIsDown = false;
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		if (lua_gettop(a_luaState) == 1)
		{
			int keyCode = (int)lua_tonumber(a_luaState, 1);
			keyIsDown = InputManager::Get().IsKeyDepressed((SDL_Keycode)keyCode);
		}
		else
		{
			LogScriptError(a_luaState, "IsKeyDown", "expects 1 parameter of the key code to test.");
		}
	}
	lua_pushboolean(a_luaState, keyIsDown);
	return 1; // One bool returned
}

int ScriptManager::IsGamePadConnected(lua_State * a_luaState)
{
	bool padConnected = false;
	if (lua_gettop(a_luaState) == 1)
	{
		int padId = (int)lua_tonumber(a_luaState, 1);
		padConnected = InputManager::Get().IsGamePadConnected(padId);
	}
	else
	{
		LogScriptError(a_luaState, "IsGamePadConnected", "expects 1 parameter of the ID of the pad to query.");
	}
	lua_pushboolean(a_luaState, padConnected);
	return 1;
}

int ScriptManager::IsGamePadButtonDown(lua_State * a_luaState)
{
	bool buttonDown = false;
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		if (lua_gettop(a_luaState) == 2)
		{
			int padId = (int)lua_tonumber(a_luaState, 1);
			int buttonId = (int)lua_tonumber(a_luaState, 2);
			buttonDown = InputManager::Get().IsGamePadButtonDepressed(padId, buttonId);
		}
		else
		{
			LogScriptError(a_luaState, "IsGamePadButtonDown", "expects 2 parameters of the ID of the pad to query and the button to check.");
		}
	}
	lua_pushboolean(a_luaState, buttonDown);
	return 1;
}

int ScriptManager::GetGamePadLeftStick(lua_State * a_luaState)
{
	float xPos = 0.0f;
	float yPos = 0.0f;
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		if (lua_gettop(a_luaState) == 1)
		{
			int padId = (int)lua_tonumber(a_luaState, 1);
			xPos = InputManager::Get().GetGamePadAxis(padId, 0);
			yPos = InputManager::Get().GetGamePadAxis(padId, 1);
		}
		else
		{
			LogScriptError(a_luaState, "GetGamePadLeftStick", "expects 1 parameters of the ID of the pad to query.");
		}
	}
	lua_pushnumber(a_luaState, xPos);
	lua_pushnumber(a_luaState, yPos);
	return 2;
}

int ScriptManager::GetGamePadRightStick(lua_State * a_luaState)
{
	float xPos = 0.0f;
	float yPos = 0.0f;
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		if (lua_gettop(a_luaState) == 1)
		{
			int padId = (int)lua_tonumber(a_luaState, 1);
			xPos = InputManager::Get().GetGamePadAxis(padId, 4);
			yPos = InputManager::Get().GetGamePadAxis(padId, 3);
		}
		else
		{
			LogScriptError(a_luaState, "GetGamePadRightStick", "expects 1 parameters of the ID of the pad to query.");
		}
	}
	lua_pushnumber(a_luaState, xPos);
	lua_pushnumber(a_luaState, yPos);
	return 2;
}

int ScriptManager::GetGamePadLeftTrigger(lua_State * a_luaState)
{
	float pos = 0.0f;
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		if (lua_gettop(a_luaState) == 1)
		{
			int padId = (int)lua_tonumber(a_luaState, 1);
			if (InputManager::Get().IsGamePadConnected(padId))
			{
				float rawVal = InputManager::Get().GetGamePadAxis(padId, 4);
				if (rawVal > -1.0f)
				{
					pos = (rawVal + 1.0f) * 0.5f;
				}
			}
		}
		else
		{
			LogScriptError(a_luaState, "GetGamePadLeftTrigger", "expects 1 parameters of the ID of the pad to query.");
		}
	}
	lua_pushnumber(a_luaState, pos);
	return 1;
}

int ScriptManager::GetGamePadRightTrigger(lua_State * a_luaState)
{
	float pos = 0.0f;
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		if (lua_gettop(a_luaState) == 1)
		{
			int padId = (int)lua_tonumber(a_luaState, 1);
			if (InputManager::Get().IsGamePadConnected(padId))
			{
				float rawVal = InputManager::Get().GetGamePadAxis(padId, 5);
				if (rawVal > -1.0f)
				{
					pos = (rawVal + 1.0f) * 0.5f;
				}
			}
		}
		else
		{
			LogScriptError(a_luaState, "GetGamePadRightTrigger", "expects 1 parameters of the ID of the pad to query.");
		}
	}
	lua_pushnumber(a_luaState, pos);
	return 1;
}

int ScriptManager::IsMouseLeftButtonDown(lua_State * a_luaState)
{
	bool buttonDown = false;
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		if (lua_gettop(a_luaState) == 0)
		{
			buttonDown = InputManager::Get().IsMouseButtonDepressed(MouseButton::Left);
		}
		else
		{
			LogScriptError(a_luaState, "IsMouseLeftButtonDown", "expects no parameters.");
		}
	}
	lua_pushboolean(a_luaState, buttonDown);
	return 1;
}

int ScriptManager::IsMouseRightButtonDown(lua_State * a_luaState)
{
	bool buttonDown = false;
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		if (lua_gettop(a_luaState) == 0)
		{
			buttonDown = InputManager::Get().IsMouseButtonDepressed(MouseButton::Right);
		}
		else
		{
			LogScriptError(a_luaState, "IsMouseRightButtonDown", "expects no parameters.");
		}
	}
	lua_pushboolean(a_luaState, buttonDown);
	return 1;
}

int ScriptManager::IsMouseMiddleButtonDown(lua_State * a_luaState)
{
	bool buttonDown = false;
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		if (lua_gettop(a_luaState) == 0)
		{
			buttonDown = InputManager::Get().IsMouseButtonDepressed(MouseButton::Middle);
		}
		else
		{
			LogScriptError(a_luaState, "IsMouseMiddleButtonDown", "expects no parameters.");
		}
	}
	lua_pushboolean(a_luaState, buttonDown);
	return 1;
}

int ScriptManager::SetCameraPosition(lua_State * a_luaState)
{
	// Don't allow camera mutation in the debug menu
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		return 0;
	}

	if (lua_gettop(a_luaState) == 3)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		Vector newPos((float)lua_tonumber(a_luaState, 1), (float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3)); 
		CameraManager::Get().SetPosition(newPos);	
	}
	else // Wrong number of parms
	{
		LogScriptError(a_luaState, "SetCameraPosition", "expects 3 number parameters.");
	}
	return 0;
}

int ScriptManager::SetCameraRotation(lua_State * a_luaState)
{
	// Don't allow camera mutation in the debug menu
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		return 0;
	}

	if (lua_gettop(a_luaState) == 3)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		const Vector newRot((float)lua_tonumber(a_luaState, 1), (float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3)); 

		Quaternion rotation(newRot);
		Matrix rotMat = Matrix::Identity();
		rotation.ApplyToMatrix(rotMat);
		Vector newTarget = rotMat.TransformInverse(Vector(0.0f, Camera::sc_defaultCameraTargetDistance, 0.0f));
		CameraManager::Get().SetTarget(CameraManager::Get().GetWorldPos() + newTarget);
	}
	else // Wrong number of parms
	{
		LogScriptError(a_luaState, "SetCameraRotation", "expects 3 number parameters.");
	}
	return 0;
}

int ScriptManager::SetCameraFOV(lua_State * a_luaState)
{
	// Don't allow camera mutation in the debug menu
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		return 0;
	}

	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		CameraManager::Get().SetFOV((float)lua_tonumber(a_luaState, 1));	
	}
	else // Wrong number of parms
	{
		LogScriptError(a_luaState, "SetCameraFOV", "expects 1 number parameter.");
	}
	return 0;
}

int ScriptManager::SetCameraTarget(lua_State * a_luaState)
{
	// Don't allow camera mutation in the debug menu
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		return 0;
	}

	if (lua_gettop(a_luaState) == 3)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		const Vector newTarget((float)lua_tonumber(a_luaState, 1), (float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3)); 
		CameraManager::Get().SetTarget(newTarget);	
	}
	else // Wrong number of parms
	{
		LogScriptError(a_luaState, "SetCameraFOV", "expects 1 number parameter.");
	}
	return 0;
}


int ScriptManager::MoveCamera(lua_State * a_luaState)
{
	// Don't allow camera mutation in the debug menu
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		return 0;
	}

	if (lua_gettop(a_luaState) == 3)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		Vector moveVec((float)lua_tonumber(a_luaState, 1), (float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3));
		CameraManager & camMan = CameraManager::Get();
		Matrix camMat = camMan.GetCameraMatrix();
		Vector camNewPos = camMan.GetWorldPos() + camMat.Transform(moveVec);
		camMan.SetPosition(camNewPos);
	}
	else // Wrong number of parms
	{
		LogScriptError(a_luaState, "MoveCamera", "expects 3 number parameters.");
	}
	return 0;
}

int ScriptManager::RotateCamera(lua_State * a_luaState)
{
	// Don't allow camera mutation in the debug menu
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		return 0;
	}

	if (lua_gettop(a_luaState) == 3)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		Vector rotVec((float)lua_tonumber(a_luaState, 1), (float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3)); 
		CameraManager & camMan = CameraManager::Get();
		Matrix camMat = camMan.GetCameraMatrix();
		Quaternion rotQ(rotVec);
		rotQ.ApplyToMatrix(camMat);
	}
	else // Wrong number of parms
	{
		LogScriptError(a_luaState, "RotateCamera", "expects 3 number parameters.");
	}
	return 0;
}

int ScriptManager::RayCollisionTest(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 6)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		const Vector rayStart((float)lua_tonumber(a_luaState, 1), (float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3)); 

		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		luaL_checktype(a_luaState, 6, LUA_TNUMBER);
		const Vector rayEnd((float)lua_tonumber(a_luaState, 4), (float)lua_tonumber(a_luaState, 5), (float)lua_tonumber(a_luaState, 6));

		PhysicsManager & physMan = PhysicsManager::Get();
		Vector worldHit(0.0f);
		Vector worldNormal(0.0f);
		if (physMan.RayCast(rayStart, rayEnd, worldHit, worldNormal))
		{
			lua_pushnumber(a_luaState, worldHit.GetX());
			lua_pushnumber(a_luaState, worldHit.GetY());
			lua_pushnumber(a_luaState, worldHit.GetZ());
			lua_pushnumber(a_luaState, worldNormal.GetX());
			lua_pushnumber(a_luaState, worldNormal.GetX());
			lua_pushnumber(a_luaState, worldNormal.GetX());
			return 6;
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "GetRayCollision", "expects 7 parameters: 3 numbers for start XYZ, 3 numbers for end XYZ and a game object.");
	}
	return 0;
}

int ScriptManager::NewScene(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		const char * sceneName = lua_tostring(a_luaState, 1);
		if (sceneName != nullptr)
		{
			WorldManager::Get().SetNewScene(sceneName);
			return 0;
		}
	}
	else
	{
		LogScriptError(a_luaState, "NewScene", "expects 1 parameter: name of the scene to set.");
	}
	return 0;
}

int ScriptManager::PlaySoundFX(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		const char * soundName = lua_tostring(a_luaState, 1);
		if (soundName != nullptr)
		{
			SoundManager::Get().PlaySoundFX(soundName);
			return 0;
		}
	}
	else
	{
		LogScriptError(a_luaState, "PlaySound", "expects 1 parameter: filename of the sound to play.");
	}
	return 0;
}

int ScriptManager::PlaySoundFX3D(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 4)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		const char * soundName = lua_tostring(a_luaState, 1);
		if (soundName != nullptr)
		{
			float x = (float)lua_tonumber(a_luaState, 2);
			float y = (float)lua_tonumber(a_luaState, 3);
			float z = (float)lua_tonumber(a_luaState, 4);
			SoundManager::Get().PlaySoundFX3D(soundName, Vector(x, y, z));
			return 0;
		}
	}
	else
	{
		LogScriptError(a_luaState, "PlaySound3D", "expects 4 parameter: filename of the sound to play then 3 floats for the position.");
	}
	return 0;
}

int ScriptManager::PlayMusic(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		const char * musicName = lua_tostring(a_luaState, 1);
		if (musicName != nullptr)
		{
			SoundManager::Get().PlayMusic(musicName);
			return 0;
		}
	}
	else
	{
		LogScriptError(a_luaState, "PlayMusic", "expects 1 parameter: filename of the music to play.");
	}
	return 0;
}

int ScriptManager::StopMusic(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		const char * musicName = lua_tostring(a_luaState, 1);
		if (musicName != nullptr)
		{
			SoundManager::Get().StopMusic(musicName);
			return 0;
		}
	}
	else
	{
		LogScriptError(a_luaState, "StopMusic", "expects 1 parameter: filename of the music to stop.");
	}
	return 0;
}

int ScriptManager::SetMusicVolume(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		const char * musicName = lua_tostring(a_luaState, 1);
		float newVolume = (float)lua_tonumber(a_luaState, 2);
		if (newVolume >= 0.0f && newVolume <= 1.0f)
		{
			if (musicName != nullptr)
			{
				SoundManager::Get().SetMusicVolume(musicName, newVolume);
				return 0;
			}
		}
		else
		{
			LogScriptError(a_luaState, "SetMusicVolume", "cannot set volume to a value less than 0 or greater than 1.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetMusicVolume", "expects 2 parameter: filename of the music that's playing then the volume of the music between 0 and 1.");
	}
	return 0;
}

int ScriptManager::SetListenerPosition(lua_State * a_luaState)
{
	const int numArg = lua_gettop(a_luaState);
	if (lua_gettop(a_luaState) == 9)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		luaL_checktype(a_luaState, 6, LUA_TNUMBER);
		luaL_checktype(a_luaState, 7, LUA_TNUMBER);
		luaL_checktype(a_luaState, 8, LUA_TNUMBER);
		luaL_checktype(a_luaState, 9, LUA_TNUMBER);
		float posX = (float)lua_tonumber(a_luaState, 1);
		float posY = (float)lua_tonumber(a_luaState, 2);
		float posZ = (float)lua_tonumber(a_luaState, 3);
		float dirX = (float)lua_tonumber(a_luaState, 4);
		float dirY = (float)lua_tonumber(a_luaState, 5);
		float dirZ = (float)lua_tonumber(a_luaState, 6);
		float velX = (float)lua_tonumber(a_luaState, 7);
		float velY = (float)lua_tonumber(a_luaState, 8);
		float velZ = (float)lua_tonumber(a_luaState, 9);
		const Vector pos(posX, posY, posZ);
		const Vector dir(dirX, dirY, dirZ);
		const Vector vel(velX, velY, velZ);
		SoundManager::Get().SetListenerPosition(pos, dir, vel);
		return 0;
	}
	else
	{
		LogScriptError(a_luaState, "SetListenerPosition", "expects 9 parameter: xyz numbers for pos, dir and vel.");
	}
	return 0;
}

int ScriptManager::StopAllSoundsAndMusic(lua_State * a_luaState)
{
	SoundManager::Get().StopAllSoundsAndMusic();
	return 0;
}

int ScriptManager::SetScene(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		const char * sceneName = lua_tostring(a_luaState, 1);
		if (sceneName != nullptr)
		{
			WorldManager::Get().SetCurrentScene(sceneName);
			return 0;
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetScene", "expects 1 parameter: name of the scene to set.");
	}
	return 0;
}

int ScriptManager::SetLightAmbient(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 5)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		int lightId = (int)lua_tonumber(a_luaState, 1);
		float r = (float)lua_tonumber(a_luaState, 2);
		float g = (float)lua_tonumber(a_luaState, 3);
		float b = (float)lua_tonumber(a_luaState, 4);
		float a = (float)lua_tonumber(a_luaState, 5);
		if (Scene * curScene = WorldManager::Get().GetCurrentScene())
		{
			if (lightId-1 >= 0 && lightId-1 < curScene->GetNumLights())
			{
				Light & curLight = curScene->GetLight(lightId-1);
				curLight.m_ambient.SetR(r);
				curLight.m_ambient.SetG(g);
				curLight.m_ambient.SetB(b);
				curLight.m_ambient.SetA(a);
			}
			return 0;
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetLightAmbient", "expects 5 parameter: the light ID then r, g, b, a.");
	}
	return 0;
}

int ScriptManager::SetLightDiffuse(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 5)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		int lightId = (int)lua_tonumber(a_luaState, 1);
		float r = (float)lua_tonumber(a_luaState, 2);
		float g = (float)lua_tonumber(a_luaState, 3);
		float b = (float)lua_tonumber(a_luaState, 4);
		float a = (float)lua_tonumber(a_luaState, 5);
		if (Scene * curScene = WorldManager::Get().GetCurrentScene())
		{
			if (lightId - 1 >= 0 && lightId - 1 < curScene->GetNumLights())
			{
				Light & curLight = curScene->GetLight(lightId - 1);
				curLight.m_diffuse.SetR(r);
				curLight.m_diffuse.SetG(g);
				curLight.m_diffuse.SetB(b);
				curLight.m_diffuse.SetA(a);
			}
			return 0;
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetLightDiffuse", "expects 5 parameter: the light ID then r, g, b, a.");
	}
	return 0;
}

int ScriptManager::SetLightSpecular(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 5)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		int lightId = (int)lua_tonumber(a_luaState, 1);
		float r = (float)lua_tonumber(a_luaState, 2);
		float g = (float)lua_tonumber(a_luaState, 3);
		float b = (float)lua_tonumber(a_luaState, 4);
		float a = (float)lua_tonumber(a_luaState, 5);
		if (Scene * curScene = WorldManager::Get().GetCurrentScene())
		{
			if (lightId - 1 >= 0 && lightId - 1 < curScene->GetNumLights())
			{
				Light & curLight = curScene->GetLight(lightId - 1);
				curLight.m_specular.SetR(r);
				curLight.m_specular.SetG(g);
				curLight.m_specular.SetB(b);
				curLight.m_specular.SetA(a);
			}
			return 0;
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetLightSpecular", "expects 5 parameter: the light ID then r, g, b, a.");
	}
	return 0;
}

int ScriptManager::SetLightPosition(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 4)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		int lightId = (int)lua_tonumber(a_luaState, 1);
		float x = (float)lua_tonumber(a_luaState, 2);
		float y = (float)lua_tonumber(a_luaState, 3);
		float z = (float)lua_tonumber(a_luaState, 4);
		if (Scene * curScene = WorldManager::Get().GetCurrentScene())
		{
			if (lightId - 1 >= 0 && lightId - 1 < curScene->GetNumLights())
			{
				Light & curLight = curScene->GetLight(lightId - 1);
				curLight.m_pos.SetX(x);
				curLight.m_pos.SetY(y);
				curLight.m_pos.SetZ(z);
			}
			return 0;
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetLightPosition", "expects 4 parameter: the light ID then x, y, z.");
	}
	return 0;
}

int ScriptManager::GetNumCPUCores(lua_State * a_luaState)
{
	int numCores = 1;
#ifdef _WIN32
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	numCores = sysInfo.dwNumberOfProcessors;
#endif
	lua_pushnumber(a_luaState, numCores);
	return 1;
}

int ScriptManager::GetStorageDrives(lua_State * a_luaState)
{
	lua_newtable(a_luaState);
#ifdef _WIN32
	DWORD drives = GetLogicalDrives();
	int driveCount = 1;
	for (int i = 0; i < 26; i++)
	{
		if ((drives & (1 << i)))
		{
			char driveLetter = 'a' + i;
			char driveString[2] = { driveLetter, '\0' };
			lua_pushnumber(a_luaState, driveCount++);
			lua_pushstring(a_luaState, &driveString[0]);
			lua_settable(a_luaState, -3);
		}
	}
#endif
	return 1;
}

int ScriptManager::Draw3DText(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 10)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		luaL_checktype(a_luaState, 6, LUA_TNUMBER);
		luaL_checktype(a_luaState, 7, LUA_TNUMBER);
		luaL_checktype(a_luaState, 8, LUA_TNUMBER);
		luaL_checktype(a_luaState, 9, LUA_TNUMBER);
		const char * string = lua_tostring(a_luaState, 1);
		const char * fontName = lua_tostring(a_luaState, 2);
		const float size = (float)lua_tonumber(a_luaState, 3);
		const float x = (float)lua_tonumber(a_luaState, 4);
		const float y = (float)lua_tonumber(a_luaState, 5);
		const float z = (float)lua_tonumber(a_luaState, 6);
		const float r = (float)lua_tonumber(a_luaState, 7);
		const float g = (float)lua_tonumber(a_luaState, 8);
		const float b = (float)lua_tonumber(a_luaState, 9);
		const float a = (float)lua_tonumber(a_luaState, 10);
		StringHash fontNameHash(fontName);
		FontManager::Get().DrawString(string, fontNameHash.GetHash(), size, Vector(x, y, z), Colour(r, g, b, a));
	}
	else
	{
		LogScriptError(a_luaState, "Draw3DText", "expects 10 parameters: string, font, size, posX, posY, posZ, colourR, colourG, colourB, colourA.");
	}
	return 0;
}

int ScriptManager::GetDirectoryListing(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		lua_newtable(a_luaState);
		if (const char * filePath = lua_tostring(a_luaState, 1))
		{
			lua_createtable(a_luaState, 1, 0);
			int fileCount = 1;
			FileManager & fileMan = FileManager::Get();
			FileManager::FileList allFiles;
			if (!fileMan.FillFileList(filePath, allFiles))
			{
				LogScriptError(a_luaState, "GetDirectoryListing", "invalid path requested.");
				return 0;
			}
			FileManager::FileListNode * curNode = allFiles.GetHead();
			while (curNode != nullptr)
			{	
				FileManager::FileInfo * fileInfo = curNode->GetData();
				lua_pushnumber(a_luaState, fileCount++);

				lua_createtable(a_luaState, 0, 3);
					
				lua_pushstring(a_luaState, fileInfo->m_name);
				lua_setfield(a_luaState, -2, "name");

				lua_pushnumber(a_luaState, fileInfo->m_sizeBytes);
				lua_setfield(a_luaState, -2, "size");

				lua_pushboolean(a_luaState, fileInfo->m_isDir);
				lua_setfield(a_luaState, -2, "isDir");

				lua_settable(a_luaState, -3);
				curNode = curNode->GetNext();
			}
			fileMan.CleanupFileList(allFiles);
			return 1;
		}
	}
	else
	{
		LogScriptError(a_luaState, "GetDirectoryListing", "expects 1 parameter: path of the directory to get.");
	}
	return 0;
}

int ScriptManager::GetFileBytes(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 3)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		lua_newtable(a_luaState);
		if (const char * filePath = lua_tostring(a_luaState, 1))
		{
			const int byteOffset = (int)lua_tonumber(a_luaState, 2);
			const int byteCount = (int)lua_tonumber(a_luaState, 3);

			ifstream fileToRead(filePath, ifstream::in | ifstream::binary);
			if (fileToRead.is_open())
			{
				lua_createtable(a_luaState, 1, 0);
				fileToRead.seekg(byteOffset);
				char readChar[2] = { ' ', '\0' };
				for (int i = 0; i < byteCount; ++i)
				{
					fileToRead.read(&readChar[0], 1);
					lua_pushnumber(a_luaState, i + 1);
					lua_pushstring(a_luaState, &readChar[0]);
					lua_settable(a_luaState, -3);

					if (!fileToRead.good())
					{
						break;
					}
				}
				fileToRead.close();
				return 1;
			}
			else
			{
				LogScriptError(a_luaState, "GetFileBytes", "can't open file.");
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GetFileBytes", "expects 3 parameters: path of the directory to get, offset into the file to read from and the number of bytes to read.");
	}
	return 0;
}

int ScriptManager::CreateParticleEmitter(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 7)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		luaL_checktype(a_luaState, 6, LUA_TNUMBER);
		luaL_checktype(a_luaState, 7, LUA_TTABLE);
		const int numParticles = (int)lua_tonumber(a_luaState, 1);
		const float emissionRate = (float)lua_tonumber(a_luaState, 2);
		const float lifeTime = (float)lua_tonumber(a_luaState, 3);
		const float posX = (float)lua_tonumber(a_luaState, 4);
		const float posY = (float)lua_tonumber(a_luaState, 5);
		const float posZ = (float)lua_tonumber(a_luaState, 6);

		// Parse the particle definition table for any and all supplied parameters
		ParticleDefinition pd;
		const int maxTableLen = ParticleDefinition::s_maxProperties;
		const int startIndex = 7;
		int index = startIndex;
		lua_pushnil(a_luaState);
		while (lua_next(a_luaState, startIndex) != 0 && index < maxTableLen + startIndex)
		{ 
			// Declare types for each possible property
			const char * propertyName = lua_tostring(a_luaState, -2);
			size_t numValues = lua_rawlen(a_luaState, -1);
			Range<float> numberVal = 0.0f;
			Range<Vector> vectorVal(0.0f);
			Range<Colour> colourVal(0.0f);

			// Figure out which property is being defined
			int propIndex = -1;
			for (int i = 0; i < ParticleDefinition::s_maxProperties; ++i)
			{
				if (strcmp(propertyName, ParticleDefinition::s_propertyNames[i]) == 0)
				{
					propIndex = i;
					break;
				}
			}
			bool numberType = propIndex == 0 || propIndex == 2 || propIndex == 3;
			bool vectorType = propIndex == 1 || propIndex == 6 || propIndex == 7;
			bool colourType = propIndex == 4 || propIndex == 5;
			
			// Push each value from each table onto the top of the stack and pop it off when done
			if (numberType)
			{
				lua_rawgeti(a_luaState, -1, 1);
				numberVal.m_low = (float)lua_tonumber(a_luaState, -1);
				lua_pop(a_luaState, 1);

				lua_rawgeti(a_luaState, -1, 2);
				numberVal.m_hi = (float)lua_tonumber(a_luaState, -1);
				lua_pop(a_luaState, 1);
			}
			else if (vectorType)
			{
				lua_rawgeti(a_luaState, -1, 1);
				{
					lua_rawgeti(a_luaState, -1, 1);
					vectorVal.m_low.SetX((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);

					lua_rawgeti(a_luaState, -1, 2);
					vectorVal.m_low.SetY((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);

					lua_rawgeti(a_luaState, -1, 3);
					vectorVal.m_low.SetZ((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);
				}
				lua_pop(a_luaState, 1);

				lua_rawgeti(a_luaState, -1, 2);
				{
					lua_rawgeti(a_luaState, -1, 1);
					vectorVal.m_hi.SetX((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);

					lua_rawgeti(a_luaState, -1, 2);
					vectorVal.m_hi.SetY((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);

					lua_rawgeti(a_luaState, -1, 3);
					vectorVal.m_hi.SetZ((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);
				}
				lua_pop(a_luaState, 1);
			}
			else if (colourType)
			{
				lua_rawgeti(a_luaState, -1, 1);
				{
					lua_rawgeti(a_luaState, -1, 1);
					colourVal.m_low.SetR((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);

					lua_rawgeti(a_luaState, -1, 2);
					colourVal.m_low.SetG((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);

					lua_rawgeti(a_luaState, -1, 3);
					colourVal.m_low.SetB((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);

					lua_rawgeti(a_luaState, -1, 4);
					colourVal.m_low.SetA((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);
				}
				lua_pop(a_luaState, 1);

				lua_rawgeti(a_luaState, -1, 2);
				{
					lua_rawgeti(a_luaState, -1, 1);
					colourVal.m_hi.SetR((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);

					lua_rawgeti(a_luaState, -1, 2);
					colourVal.m_hi.SetG((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);

					lua_rawgeti(a_luaState, -1, 3);
					colourVal.m_hi.SetB((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);

					lua_rawgeti(a_luaState, -1, 4);
					colourVal.m_hi.SetA((float)lua_tonumber(a_luaState, -1));
					lua_pop(a_luaState, 1);
				}
				lua_pop(a_luaState, 1);
			}
			
			lua_pop(a_luaState, 1);
			index++;

			// Handle all the different particle properties
			switch (propIndex)
			{
				case 0: pd.m_lifeTime = numberVal; break;
				case 1: pd.m_startPos = vectorVal; break;
				case 2: pd.m_startSize = numberVal; break;
				case 3: pd.m_endSize = numberVal; break;
				case 4: pd.m_startColour = colourVal; break;
				case 5: pd.m_endColour = colourVal; break;
				case 6: pd.m_startVel = vectorVal; break;
				case 7:	pd.m_endVel = vectorVal; break;
				default: break;
			}
		}

		// Add the emitter
		const int emitterId = RenderManager::Get().AddParticleEmitter(numParticles, emissionRate, lifeTime, Vector(posX, posY, posZ), pd);
		lua_pushnumber(a_luaState, emitterId);
		return 1;
	}
	else
	{
		LogScriptError(a_luaState, "CreateParticleEmitter", "expects 7 parameters: numParticles, emissionRate, lifeTime, posX, posY, posZ, particleDefTable.");
	}
	return 0;
}

int ScriptManager::MoveParticleEmitter(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 4)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		const int emitterId = (int)lua_tonumber(a_luaState, 1);
		const float posX = (float)lua_tonumber(a_luaState, 2);
		const float posY = (float)lua_tonumber(a_luaState, 3);
		const float posZ = (float)lua_tonumber(a_luaState, 4);
		RenderManager::Get().SetParticleEmitterPosition(emitterId, Vector(posX, posY, posZ));
		return 0;
	}
	else
	{
		LogScriptError(a_luaState, "MoveParticleEmitter", "expects 4 parameters: ID of emitter then posX, posY, posZ.");
	}
	return 0;
}

int ScriptManager::DestroyParticleEmitter(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		const int emitterId = (int)lua_tonumber(a_luaState, 1);
		RenderManager::Get().DestroyParticleEmitter(emitterId);
		return 0;
	}
	else
	{
		LogScriptError(a_luaState, "DestroyParticleEmitter", "expects 1 parameters: ID of emitter.");
	}
	return 0;
}

int ScriptManager::GUIGetValue(lua_State * a_luaState)
{
	char strValue[StringUtils::s_maxCharsPerName];
	strValue[0] = '\0';
	if (lua_gettop(a_luaState) == 2)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		if (const char * guiName = lua_tostring(a_luaState, 2))
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				strncpy(strValue, foundElem->GetText(), strlen(foundElem->GetText()) + 1);
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:GetValue", "expects 1 parameter: name of the element to get.");
	}
	
	lua_pushstring(a_luaState, strValue);
	return 1;
}

int ScriptManager::GUISetValue(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 3)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		luaL_checktype(a_luaState, 3, LUA_TSTRING);
		const char * guiName = lua_tostring(a_luaState, 2);
		const char * newText = lua_tostring(a_luaState, 3);
		if (guiName != nullptr && newText != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				foundElem->SetText(newText);
				return 0;
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:SetValue", "expects 2 parameters: name of the element to set and the string to set.");
	}

	LogScriptError(a_luaState, "GUI:SetValue", "could not find the GUI element to set a value on.");
	return 0;
}

int ScriptManager::GUISetFont(lua_State* a_luaState)
{
	if (lua_gettop(a_luaState) == 3)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		luaL_checktype(a_luaState, 3, LUA_TSTRING);
		const char * guiName = lua_tostring(a_luaState, 2);
		const char * newFontName = lua_tostring(a_luaState, 3);
		if (guiName != nullptr && newFontName != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				foundElem->SetFontName(StringHash(newFontName).GetHash());
				return 0;
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:SetFont", "expects 2 parameters: name of the element to set and the name of the font to set.");
	}

	LogScriptError(a_luaState, "GUI:SetFont", "could not find the GUI element to set a value on.");
	return 0;
}

int ScriptManager::GUISetFontSize(lua_State* a_luaState)
{
	if (lua_gettop(a_luaState) == 3)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		const char * guiName = lua_tostring(a_luaState, 2);
		const float newFontSize = (float)lua_tonumber(a_luaState, 3);
		if (guiName != nullptr && newFontSize > 0.0f)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				foundElem->SetFontSize(newFontSize);
				return 0;
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:SetFontSize", "expects 2 parameters: name of the element to set and a number for how big.");
	}

	LogScriptError(a_luaState, "GUI:SetFontSize", "could not find the GUI element to set a value on.");
	return 0;
}

int ScriptManager::GUISetColour(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 6)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		luaL_checktype(a_luaState, 6, LUA_TNUMBER);
		const char * guiName = lua_tostring(a_luaState, 2);
		Colour newColour((float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4), (float)lua_tonumber(a_luaState, 5), (float)lua_tonumber(a_luaState, 6));
		if (guiName != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				foundElem->SetColour(newColour);
				return 0;
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:SetColour", "expects 5 parameters: name of the element then four numbers between 0 and 1 for each colour channel.");
	}

	LogScriptError(a_luaState, "GUI:SetColour", "could not find the GUI element to set colour on.");
	return 0;
}

int ScriptManager::GUISetSize(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 4)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		const char * guiName = lua_tostring(a_luaState, 2);
		Vector2 newSize((float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4));
		if (guiName != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				foundElem->SetSize(newSize);
				return 0;
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:SetSize", "expects 3 parameters: name of the element then 2 dimensions for X and Y.");
	}

	LogScriptError(a_luaState, "GUI:SetSize", "could not find the GUI element to set size on.");
	return 0;
}

int ScriptManager::GUISetScissor(lua_State * a_luaState)
{

	if (lua_gettop(a_luaState) == 4)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		const char * guiName = lua_tostring(a_luaState, 2);
		if (guiName != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				foundElem->SetScissorRect(Vector2((float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4)));
				return 0;
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:SetScissor", "expects 3 parameters: name of the element then two numbers between 0 and 1 for each scissor dimension.");
	}

	LogScriptError(a_luaState, "GUI:SetScissor", "could not find the GUI element to set scissor on.");
	return 0;
}

int ScriptManager::GUISetTexture(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 3)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		luaL_checktype(a_luaState, 3, LUA_TSTRING);		
		const char * guiName = lua_tostring(a_luaState, 2);
		const char * textureName = lua_tostring(a_luaState, 3);
		if (guiName != nullptr && textureName != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				if (Texture * newGuiTex = TextureManager::Get().GetTexture(textureName, TextureCategory::Gui))
				{
					foundElem->SetTexture(newGuiTex);
					return 0;
				}
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:GUISetTexture", "expects 2 parameters: name of the element then the name of the texture file.");
	}

	LogScriptError(a_luaState, "GUI:GUISetTexture", "could not find the GUI element to set texture on.");
	return 0;
}

int ScriptManager::GUIShowWidget(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		const char * guiName = lua_tostring(a_luaState, 2);
		if (guiName != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				foundElem->SetActive(true);
				return 0;
			}
		}
	}
	else
	{

		LogScriptError(a_luaState, "GUI:Show", "expects 1 parameter: name of the element.");
	}

	LogScriptError(a_luaState, "GUI:Show", "could not find the GUI element to set a value on.");
	return 0;
}

int ScriptManager::GUIHideWidget(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		const char * guiName = lua_tostring(a_luaState, 2);
		if (guiName != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				foundElem->SetActive(false);
				return 0;
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:Hide", "expects 1 parameter: name of the element.");
	}

	LogScriptError(a_luaState, "GUI:Hide", "could not find the GUI element to set a value on.");
	return 0;
}

int ScriptManager::GUIActivateWidget(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		const char * guiName = lua_tostring(a_luaState, 2);
		if (guiName != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				foundElem->DoActivation();
				return 0;
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:ActivateWidet", "expects 1 parameter: name of the element.");
	}

	LogScriptError(a_luaState, "GUI:ActivateWidet", "could not find the GUI element to activate.");
	return 0;
}

int ScriptManager::GUIGetSelectedWidget(lua_State * a_luaState)
{	
	if (Widget * selectedElem = Gui::Get().FindSelectedWidget())
	{
		lua_pushstring(a_luaState, selectedElem->GetName());
		return 1;
	}

	return 0;
}

int ScriptManager::GUISetSelectedWidget(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		const char * guiName = lua_tostring(a_luaState, 2);
		if (guiName != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				InputManager::Get().SetMousePosRelative(foundElem->GetDrawPos());
				return 0;
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:SetSelectedWidget", "expects 1 parameter: name of the element.");
	}

	LogScriptError(a_luaState, "GUI:GUISetSelectedWidget", "could not find the GUI element to set selected.");
	return 0;
}

int ScriptManager::GUISetActiveMenu(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		luaL_checktype(a_luaState, 2, LUA_TSTRING);
		const char * guiName = lua_tostring(a_luaState, 2);
		if (guiName != nullptr)
		{
			if (Widget * foundElem = Gui::Get().FindWidget(guiName))
			{
				Gui::Get().SetActiveMenu(foundElem);
				return 0;
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "GUI:GUISetActiveMenu", "expects 1 parameter: name of the menu.");
	}

	LogScriptError(a_luaState, "GUI:GUISetActiveMenu", "could not find the menu to set active.");
	return 0;
}

int ScriptManager::GUIEnableMouse(lua_State * a_luaState)
{
	Gui::Get().EnableMouseCursor();
	InputManager::Get().EnableMouseInput();
	return 0;
}

int ScriptManager::GUIDisableMouse(lua_State * a_luaState)
{
	Gui::Get().DisableMouseCursor();
	InputManager::Get().DisableMouseInput();
	return 0;
}

int ScriptManager::GUIGetMouseClipPosition(lua_State * a_luaState)
{
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		lua_pushnumber(a_luaState, 0.0f);
		lua_pushnumber(a_luaState, 0.0f);
		return 2;
	}

	InputManager & inMan = InputManager::Get();
	const Vector2 mousePos = inMan.GetMousePosRelative();
	lua_pushnumber(a_luaState, mousePos.GetX());
	lua_pushnumber(a_luaState, mousePos.GetY());
	return 2;
}

int ScriptManager::GUIGetMouseScreenPosition(lua_State * a_luaState)
{
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		lua_pushnumber(a_luaState, 0.0f);
		lua_pushnumber(a_luaState, 0.0f);
		return 2;
	}

	InputManager & inMan = InputManager::Get();
	const Vector2 mousePos = inMan.GetMousePosAbsolute();
	lua_pushnumber(a_luaState, mousePos.GetX());
	lua_pushnumber(a_luaState, mousePos.GetY());
	return 2;
}

int ScriptManager::GUIGetMouseDirection(lua_State * a_luaState)
{
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		lua_pushnumber(a_luaState, 0.0f);
		lua_pushnumber(a_luaState, 0.0f);
		return 2;
	}
	
	InputManager & inMan = InputManager::Get();
	const Vector2 mouseDir = inMan.GetMouseDirection();
	lua_pushnumber(a_luaState, mouseDir.GetX());
	lua_pushnumber(a_luaState, mouseDir.GetY());
	return 2;
}

int ScriptManager::GUISetMousePosition(lua_State * a_luaState)
{
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		// Stubbed out
		// SDL_WarpMouseGlobal(0, 0);
	}
	return 0;
}

int ScriptManager::DataPackRequire(lua_State * a_luaState)
{
	// This is only a valid call when running from datapack
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		const char * scriptPath = lua_tostring(a_luaState, 1);
		if (scriptPath != nullptr && scriptPath[0] != '\0')
		{
			char scriptName[StringUtils::s_maxCharsPerLine];
			ScriptManager & scriptMan = ScriptManager::Get();
			sprintf(scriptName, "%s%s.lua", scriptMan.m_scriptPath, scriptPath);
			if (DataPackEntry * luaResource = DataPack::Get().GetEntry(scriptName))
			{
				luaL_loadbuffer(scriptMan.m_gameLua, luaResource->m_data, luaResource->m_size, scriptName);
				lua_pcall(scriptMan.m_gameLua, 0, 0, 0);
			}
			else
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Game, "DataPackRequire cannot find a lua file called %s in the pack.", scriptPath);
			}
		}
	}
	else
	{
		LogScriptError(a_luaState, "DataPackRequire", "at least one parameter of the lua file that is required.");
	}
	return 1;
}

int ScriptManager::DebugPrint(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		const char * debugText = lua_tostring(a_luaState, 1);
		if (debugText != nullptr && debugText != nullptr)
		{
			DebugMenu::Get().ShowScriptDebugText(debugText);
		}
	}
	else
	{
		LogScriptError(a_luaState, "DebugPrint", "at least one parameter to show on the screen.");
	}

	return 0;
}

int ScriptManager::DebugLog(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		const char * debugText = lua_tostring(a_luaState, 1);
		if (debugText != nullptr && debugText != nullptr)
		{
			Log::Get().Write(LogLevel::Info, LogCategory::Game, debugText);
		}
	}
	else
	{
		LogScriptError(a_luaState, "DebugLog", "expects just one parameter to show on the screen.");
	}

	return 0;
}

int ScriptManager::DebugLine(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 6)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		luaL_checktype(a_luaState, 6, LUA_TNUMBER);
		Vector p1((float)lua_tonumber(a_luaState, 1), (float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3)); 
		Vector p2((float)lua_tonumber(a_luaState, 4), (float)lua_tonumber(a_luaState, 5), (float)lua_tonumber(a_luaState, 6));
		RenderManager::Get().AddDebugLine(p1, p2);
	}
	return 0;
}

int ScriptManager::DebugPoint(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 6)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		luaL_checktype(a_luaState, 6, LUA_TNUMBER);
		Vector pos((float)lua_tonumber(a_luaState, 1), (float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3)); 
		Colour col((float)lua_tonumber(a_luaState, 4), (float)lua_tonumber(a_luaState, 5), (float)lua_tonumber(a_luaState, 6), 1.0f);
		RenderManager::Get().AddDebugSphere(pos, 1.0f, col);
	}
	return 0;
}

int ScriptManager::IsDebugMenuActive(lua_State * a_luaState)
{
	bool menuActive = false;
#ifndef _RELEASE
	menuActive = DebugMenu::Get().IsDebugMenuActive();
#endif
	lua_pushboolean(a_luaState, menuActive);
	return 1;
}

int ScriptManager::GetGameObjectId(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			lua_pushnumber(a_luaState, gameObj->GetId());
			return 1;
		}
		else
		{
			LogScriptError(a_luaState, "GetId", "Cannot find the game object referred to."); 
		}
	}
	else
	{
		LogScriptError(a_luaState, "GetId", "Function expects no parameters.");
	}
	
	return 0;
}

int ScriptManager::GetGameObjectName(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			lua_pushstring(a_luaState, gameObj->GetName());
			return 1;
		}
		else
		{
			LogScriptError(a_luaState, "GetName", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "GetName", "expects no parameters.");
	}
	
	return 0;
}

int ScriptManager::SetGameObjectName(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TSTRING);
			const char * newName = lua_tostring(a_luaState, 2);
			gameObj->SetName(newName);
			return 0;
		}
		else
		{
			LogScriptError(a_luaState, "SetName", "cannot find the game object referred to.");
		}
	}
	 
	LogScriptError(a_luaState, "SetName", "expects 1 string parameter.");

	return 0;
}

int ScriptManager::SetGameObjectModel(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TSTRING);
			const char * modelName = lua_tostring(a_luaState, 2);
			if (Model * model = ModelManager::Get().GetModel(modelName))
			{
				gameObj->SetModel(model);
			}
			else
			{
				LogScriptError(a_luaState, "SetModel", "cannot find the mode referred to.");
			}
		}
		else
		{
			LogScriptError(a_luaState, "SetModel", "cannot find the game object referred to.");
		}
	}
	return 0;
}

int ScriptManager::SetGameObjectShaderData(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 4)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TNUMBER);
			luaL_checktype(a_luaState, 3, LUA_TNUMBER);
			luaL_checktype(a_luaState, 4, LUA_TNUMBER);
			gameObj->SetShaderData(Vector((float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4)));
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetShaderData", "cannot find the game object referred to.");
	}
	return 0;
}

int ScriptManager::GetGameObjectPosition(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			Vector pos = gameObj->GetPos();
			lua_pushnumber(a_luaState, pos.GetX());
			lua_pushnumber(a_luaState, pos.GetY());
			lua_pushnumber(a_luaState, pos.GetZ());
			return 3;
		}
		else
		{
			LogScriptError(a_luaState, "GetPosition", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "GetPosition", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::SetGameObjectPosition(lua_State * a_luaState)
{ 
	if (lua_gettop(a_luaState) == 4)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TNUMBER);
			luaL_checktype(a_luaState, 3, LUA_TNUMBER);
			luaL_checktype(a_luaState, 4, LUA_TNUMBER);
			Vector newPos((float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4)); 
			gameObj->SetPos(newPos);
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "SetPosition", "could not find game object referred to.");
		}
	}
	else // Wrong number of parms
	{
		LogScriptError(a_luaState, "SetPosition", "expects 3 number parameters.");
	}
	return 0;
}

int ScriptManager::GetGameObjectRotation(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			Quaternion rot = gameObj->GetRot();
			Vector eulerRot = rot.GetXYZ();
			lua_pushnumber(a_luaState, eulerRot.GetX());
			lua_pushnumber(a_luaState, eulerRot.GetY());
			lua_pushnumber(a_luaState, eulerRot.GetZ());
			return 3;
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "GetRotation", "cannot find the game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "GetRotation", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::SetGameObjectRotation(lua_State * a_luaState)
{ 
	if (lua_gettop(a_luaState) == 4)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TNUMBER);
			luaL_checktype(a_luaState, 3, LUA_TNUMBER);
			luaL_checktype(a_luaState, 4, LUA_TNUMBER);
			Vector newRot((float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4)); 
			gameObj->SetRot(newRot);
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "SetRotation", "could not find game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "SetRotation", "expects 3 number parameters.");
	}
	return 0;
}

int ScriptManager::SetGameObjectLookAt(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 4)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TNUMBER);
			luaL_checktype(a_luaState, 3, LUA_TNUMBER);
			luaL_checktype(a_luaState, 4, LUA_TNUMBER);
			const Vector lookAt((float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4));
			const Vector pos = gameObj->GetPos();
			gameObj->SetWorldMat(Matrix::LookAtRH(gameObj->GetPos(), lookAt, pos));
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "SetLookAt", "could not find game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "SetLookAt", "expects 4 number parameters of the world position to look at.");
	}
	return 0;
}

int ScriptManager::GetGameObjectScale(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			Vector scale = gameObj->GetScale();
			lua_pushnumber(a_luaState, scale.GetX());
			lua_pushnumber(a_luaState, scale.GetY());
			lua_pushnumber(a_luaState, scale.GetZ());
			return 3;
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "GetScale", "cannot find the game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "GetScale", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::SetGameObjectScale(lua_State * a_luaState)
{ 
	if (lua_gettop(a_luaState) == 4)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TNUMBER);
			luaL_checktype(a_luaState, 3, LUA_TNUMBER);
			luaL_checktype(a_luaState, 4, LUA_TNUMBER);
			Vector newScale((float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4)); 
			gameObj->SetScale(newScale);
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "SetScale", "could not find game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "SetScale", "expects 3 number parameters.");
	}
	return 0;
}

int ScriptManager::MultiplyGameObjectScale(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 4)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TNUMBER);
			luaL_checktype(a_luaState, 3, LUA_TNUMBER);
			luaL_checktype(a_luaState, 4, LUA_TNUMBER);
			Vector newScale((float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4));
			gameObj->MulScale(newScale);
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "MultiplyScale", "could not find game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "MultiplyScale", "expects 3 number parameters.");
	}
	return 0;
}

int ScriptManager::ResetGameObjectScale(lua_State * a_luaState)
{ 
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			gameObj->RemoveScale();
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "ResetScale", "could not find game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "ResetScale", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::SetAttachedTo(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 8)
	{
		GameObject * attachmentGameObj = CheckGameObject(a_luaState);
		GameObject * parentGameObj = CheckGameObject(a_luaState, 2);
		if (attachmentGameObj != nullptr && parentGameObj != nullptr)
		{
			luaL_checktype(a_luaState, 3, LUA_TNUMBER);
			luaL_checktype(a_luaState, 4, LUA_TNUMBER);
			luaL_checktype(a_luaState, 5, LUA_TNUMBER);
			luaL_checktype(a_luaState, 6, LUA_TNUMBER);
			luaL_checktype(a_luaState, 7, LUA_TNUMBER);
			luaL_checktype(a_luaState, 8, LUA_TNUMBER);
			Vector offsetPos((float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4), (float)lua_tonumber(a_luaState, 5));
			const Vector offsetRot((float)lua_tonumber(a_luaState, 6), (float)lua_tonumber(a_luaState, 7), (float)lua_tonumber(a_luaState, 8));
			Matrix attachmentMat = parentGameObj->GetWorldMat();
			offsetPos = attachmentMat.Transform(offsetPos);
			Quaternion qRot = Quaternion(offsetRot);
			qRot.ApplyToMatrix(attachmentMat);
			attachmentMat.SetPos(attachmentMat.GetPos() + offsetPos);
			attachmentGameObj->SetWorldMat(attachmentMat);
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "SetAttachedTo", "could not find game object referred to as parent or attachment.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "SetAttachedTo", "expects 7 parameters - the object to attach to then offsetPosX, offsetPosY, offsetPosZ, offsetRotX, offsetRotY, offsetRotZ.");
	}
	return 0;
}

int ScriptManager::SetAttachedToCamera(lua_State * a_luaState)
{
	if (!DebugMenu::Get().IsDebugMenuEnabled())
	{
		if (lua_gettop(a_luaState) == 7)
		{
			if (GameObject * gameObj = CheckGameObject(a_luaState))
			{
				luaL_checktype(a_luaState, 2, LUA_TNUMBER);
				luaL_checktype(a_luaState, 3, LUA_TNUMBER);
				luaL_checktype(a_luaState, 4, LUA_TNUMBER);
				luaL_checktype(a_luaState, 5, LUA_TNUMBER);
				luaL_checktype(a_luaState, 6, LUA_TNUMBER);
				luaL_checktype(a_luaState, 7, LUA_TNUMBER);
				Vector offsetPos((float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4));
				Vector offsetRot((float)lua_tonumber(a_luaState, 5), (float)lua_tonumber(a_luaState, 6), (float)lua_tonumber(a_luaState, 7));
				Quaternion qOffsetRot = Quaternion(offsetRot);

				// Construct a new object matrix as the camera looks down the up vector
				const Matrix & cameraMat = CameraManager::Get().GetCameraMatrix();
				Matrix newGameObjMat = Matrix::LookAtRH(Vector::Zero(), -cameraMat.GetUp(), cameraMat.GetPos());
				offsetPos = newGameObjMat.Transform(offsetPos);
				newGameObjMat.SetPos(newGameObjMat.GetPos() + offsetPos);
				gameObj->SetWorldMat(newGameObjMat);
			}
			else // Object not found, destroyed?
			{
				LogScriptError(a_luaState, "SetAttachedToCamera", "could not find game object referred to.");
			}
		}
		else // Wrong number of args
		{
			LogScriptError(a_luaState, "SetAttachedToCamera", "expects 6 parameters - offsetX, offsetY, offsetZ, rotationX, rotationY, rotationZ.");
		}
	}
	return 0;
}

int ScriptManager::GetGameObjectLifeTime(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			lua_pushnumber(a_luaState, gameObj->GetLifeTime());
			return 1;
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "GetLifeTime", "cannot find the game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "GetLifeTime", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::SetGameObjectLifeTime(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TNUMBER);
			gameObj->SetLifeTime((float)lua_tonumber(a_luaState, 2));
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "SetLifeTime", "could not find game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "SetLifeTime", "expects 1 number parameter.");
	}
	return 0;
}

int ScriptManager::SetGameObjectSleeping(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			gameObj->SetSleeping();
		}
		else
		{
			LogScriptError(a_luaState, "SetSleeping", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetSleeping", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::SetGameObjectActive(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			gameObj->SetActive();
		}
		else
		{
			LogScriptError(a_luaState, "SetActive", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetActive", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::GetGameObjectSleeping(lua_State * a_luaState)
{
	bool isSleeping = false;

	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			isSleeping = gameObj->IsSleeping();
		}
		else
		{
			LogScriptError(a_luaState, "IsSleeping", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "IsSleeping", "expects no parameters.");
	}

	lua_pushboolean(a_luaState, isSleeping);
	return 1;
}

int ScriptManager::GetGameObjectActive(lua_State * a_luaState)
{
	bool isActive = false;

	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			isActive = gameObj->IsActive();
		}
		else
		{
			LogScriptError(a_luaState, "IsActive", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "IsActive", "expects no parameters.");
	}

	lua_pushboolean(a_luaState, isActive);
	return 1;
}

int ScriptManager::SetGameObjectDiffuseTexture(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			if (Model * model = gameObj->GetModel())
			{
				luaL_checktype(a_luaState, 2, LUA_TSTRING);
				if (Texture * diffuseTex = TextureManager::Get().GetTexture(lua_tostring(a_luaState, 2), TextureCategory::Model))
				{
					const int objectCount = model->GetNumObjects();
					for (int i = 0; i < objectCount; ++i)
					{
						if (Object * object = model->GetObjectAtIndex(i))
						{
							if (Material * mat = object->GetMaterial())
							{
								mat->SetDiffuseTexture(diffuseTex);
							}
						}
					}
				}
				else // No texture
				{
					LogScriptError(a_luaState, "SetDiffuseTexture", "could not find the specified texture.");
				}
			}
			else // No model
			{
				LogScriptError(a_luaState, "SetDiffuseTexture", "game object does not have a model.");
			}
			
			gameObj->SetLifeTime((float)lua_tonumber(a_luaState, 2));
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "SetDiffuseTexture", "could not find game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "SetDiffuseTexture", "expects 1 string parameter of the texture to set.");
	}
	return 0;
}

int ScriptManager::SetGameObjectNormalTexture(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			if (Model * model = gameObj->GetModel())
			{
				luaL_checktype(a_luaState, 2, LUA_TSTRING);
				if (Texture * normalTex = TextureManager::Get().GetTexture(lua_tostring(a_luaState, 2), TextureCategory::Model))
				{
					const int objectCount = model->GetNumObjects();
					for (int i = 0; i < objectCount; ++i)
					{
						if (Object * object = model->GetObjectAtIndex(i))
						{
							if (Material * mat = object->GetMaterial())
							{
								mat->SetNormalTexture(normalTex);
							}
						}
					}
				}
				else // No texture
				{
					LogScriptError(a_luaState, "SetNormalTexture", "could not find the specified texture.");
				}
			}
			else // No model
			{
				LogScriptError(a_luaState, "SetNormalTexture", "game object does not have a model.");
			}

			gameObj->SetLifeTime((float)lua_tonumber(a_luaState, 2));
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "SetNormalTexture", "could not find game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "SetNormalTexture", "expects 1 string parameter of the texture to set.");
	}
	return 0;
}

int ScriptManager::SetGameObjectSpecularTexture(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			if (Model * model = gameObj->GetModel())
			{
				luaL_checktype(a_luaState, 2, LUA_TSTRING);
				if (Texture * specTex = TextureManager::Get().GetTexture(lua_tostring(a_luaState, 2), TextureCategory::Model))
				{
					const int objectCount = model->GetNumObjects();
					for (int i = 0; i < objectCount; ++i)
					{
						if (Object * object = model->GetObjectAtIndex(i))
						{
							if (Material * mat = object->GetMaterial())
							{
								mat->SetSpecularTexture(specTex);
							}
						}
					}
				}
				else // No texture
				{
					LogScriptError(a_luaState, "SetSpecularTexture", "could not find the specified texture.");
				}
			}
			else // No model
			{
				LogScriptError(a_luaState, "SetSpecularTexture", "game object does not have a model.");
			}

			gameObj->SetLifeTime((float)lua_tonumber(a_luaState, 2));
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "SetSpecularTexture", "could not find game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "SetSpecularTexture", "expects 1 string parameter of the texture to set.");
	}
	return 0;
}


int ScriptManager::SetGameObjectVisibility(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 2)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			gameObj->SetVisible(lua_toboolean(a_luaState, 2) != 0);
		}
		else
		{
			LogScriptError(a_luaState, "SetGameObjectVisibility", "could not find game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetGameObjectVisibility", "expects 1 bool parameter of the visible state to set.");
	}
	return 0;
}

int ScriptManager::EnableGameObjectCollision(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			gameObj->SetClipping(true);
		}
		else
		{
			LogScriptError(a_luaState, "EnableCollision", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "EnableCollision", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::DisableGameObjectCollision(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			gameObj->SetClipping(false);
		}
		else
		{
			LogScriptError(a_luaState, "DisableCollision", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "DisableCollision", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::AddGameObjectToCollisionWorld(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			// Add object to collision world
			if (gameObj->GetClipType() == ClipType::Mesh || gameObj->GetClipSize().LengthSquared() > 0.0f)
			{
				PhysicsManager::Get().AddCollisionObject(gameObj);
			}
		}
		else
		{
			LogScriptError(a_luaState, "AddToCollisionWorld", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "AddToCollisionWorld", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::SetGameObjectClipSize(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 4)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TNUMBER);
			luaL_checktype(a_luaState, 3, LUA_TNUMBER);
			luaL_checktype(a_luaState, 4, LUA_TNUMBER);
			Vector clipSize((float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4));
			gameObj->SetClipSize(clipSize);
		}
		else
		{
			LogScriptError(a_luaState, "SetClipSize", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "SetClipSize", "expects 3 numbers of size X, Y and Z.");
	}
	return 0;
}

int ScriptManager::AddGameObjectToPhysicsWorld(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			PhysicsManager::Get().AddPhysicsObject(gameObj);
		}
		else
		{
			LogScriptError(a_luaState, "AddToPhysicsWorld", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "AddToPhysicsWorld", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::ApplyGameObjectPhysicsForce(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 4)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TNUMBER);
			luaL_checktype(a_luaState, 3, LUA_TNUMBER);
			luaL_checktype(a_luaState, 4, LUA_TNUMBER);
			Vector force((float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4));
			PhysicsManager::Get().ApplyForce(gameObj, force);
		}
		else // Object not found, destroyed?
		{
			LogScriptError(a_luaState, "ApplyPhysicsForce", "could not find game object referred to.");
		}
	}
	else // Wrong number of args
	{
		LogScriptError(a_luaState, "ApplyPhysicsForce", "expects 3 number parameters.");
	}
	return 0;
}

int ScriptManager::GetGameObjectVelocity(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			Vector vel = PhysicsManager::Get().GetVelocity(gameObj);
			lua_pushnumber(a_luaState, vel.GetX());
			lua_pushnumber(a_luaState, vel.GetY());
			lua_pushnumber(a_luaState, vel.GetZ());
			return 3;
		}
		else
		{
			LogScriptError(a_luaState, "GetGameObjectVelocity", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "GetGameObjectVelocity", "expects the game object as the self.");
	}
	return 0;
}

int ScriptManager::TestGameObjectCollisions(lua_State * a_luaState)
{
	// Check the list of collisions
	bool foundCollisions = false;
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			GameObject::CollisionList * colList = gameObj->GetCollisions();
			foundCollisions = !colList->IsEmpty();
		}
		else
		{
			LogScriptError(a_luaState, "TestCollisions", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "TestCollisions", "expects no parameters.");
	}

	// Push if we found any collisions
	lua_pushboolean(a_luaState, foundCollisions);
	return 1;
}


int ScriptManager::GetGameObjectCollisions(lua_State * a_luaState)
{
	// Populate table with any collisions this frame
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			// Push a new table
			lua_newtable(a_luaState);

			int numCollisions = 0;
			GameObject::CollisionList * colList = gameObj->GetCollisions();
			GameObject::Collider * curCol = colList->GetHead();
			while (curCol != nullptr)
			{
				// Push the key for a table entry
				lua_pushnumber(a_luaState, ++numCollisions);
				
				// Push the value - a unsigned int wrapped as user data (our LUA game object)
				unsigned int * userData = (unsigned int*)lua_newuserdata(a_luaState, sizeof(unsigned int));
				*userData = curCol->GetData()->GetId();

				// Associate the metatable with the userdata
				luaL_getmetatable(a_luaState, "GameObject.Registry");
				lua_setmetatable(a_luaState, -2);

				lua_settable(a_luaState, -3);

				curCol = curCol->GetNext();
			}
		}
		else
		{
			LogScriptError(a_luaState, "GetCollisions", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "GetCollisions", "expects no parameters.");
	}

	return 1;
}

int ScriptManager::PlayGameObjectAnimation(lua_State * a_luaState)
{
	bool playedAnim = false;
	if (lua_gettop(a_luaState) == 2)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TSTRING);
			const char * animName = lua_tostring(a_luaState, 2);
			AnimationManager::Get().PlayAnimation(gameObj, StringHash(animName));
		}
		else
		{
			LogScriptError(a_luaState, "PlayAnimation", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "PlayAnimation", "expects one parameter - the name of the animation to play.");
	}

	lua_pushboolean(a_luaState, playedAnim);
	return 1;
}

int ScriptManager::GetGameObjectTransformedPos(lua_State * a_luaState)
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	if (lua_gettop(a_luaState) == 4)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			luaL_checktype(a_luaState, 2, LUA_TNUMBER);
			luaL_checktype(a_luaState, 3, LUA_TNUMBER);
			luaL_checktype(a_luaState, 4, LUA_TNUMBER);
			Vector inVec((float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3), (float)lua_tonumber(a_luaState, 4));
			Matrix & gameObjMat = gameObj->GetWorldMat();
			Vector outMat = gameObjMat.Transform(inVec);
			x = outMat.GetX();
			y = outMat.GetY();
			z = outMat.GetZ();
		}
		else
		{
			LogScriptError(a_luaState, "GetGameObjectTransformPos", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "GetGameObjectTransformPos", "expects 3 parameters: X, Y and Z components of the vector to transform.");
	}
	lua_pushnumber(a_luaState, x);
	lua_pushnumber(a_luaState, y);
	lua_pushnumber(a_luaState, z);
	return 3;
}

int ScriptManager::DestroyGameObject(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		if (GameObject * gameObj = CheckGameObject(a_luaState))
		{
			WorldManager::Get().DestroyObject(gameObj->GetId());
		}
		else
		{
			LogScriptError(a_luaState, "Destroy", "cannot find the game object referred to.");
		}
	}
	else
	{
		LogScriptError(a_luaState, "Destroy", "expects no parameters.");
	}
	return 0;
}

int ScriptManager::RenderSetShader(lua_State * a_luaState)
{
	return 0;
}

int ScriptManager::RenderSetShaderData(lua_State * a_luaState)
{
	return 0;
}

int ScriptManager::RenderQuad(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 6)
	{
		luaL_checktype(a_luaState, 1, LUA_TNUMBER);
		luaL_checktype(a_luaState, 2, LUA_TNUMBER);
		luaL_checktype(a_luaState, 3, LUA_TNUMBER);
		luaL_checktype(a_luaState, 4, LUA_TNUMBER);
		luaL_checktype(a_luaState, 5, LUA_TNUMBER);
		luaL_checktype(a_luaState, 6, LUA_TNUMBER);
		Vector p1((float)lua_tonumber(a_luaState, 1), (float)lua_tonumber(a_luaState, 2), (float)lua_tonumber(a_luaState, 3));
		Vector p2((float)lua_tonumber(a_luaState, 4), (float)lua_tonumber(a_luaState, 5), (float)lua_tonumber(a_luaState, 6));
		RenderManager::Get().AddDebugLine(p1, p2);
	}
	return 0;
}

int ScriptManager::RenderTri(lua_State * a_luaState)
{
	return 0;
}

void ScriptManager::StackDump(lua_State *a_luaState)
{
	Log & log = Log::Get();
	log.Write(LogLevel::Info, LogCategory::Engine, "LUA Stack follows:");
	int top = lua_gettop(a_luaState);
	for (int i = 1; i <= top; i++)
	{
		int t = lua_type(a_luaState, i);
		switch (t)
		{
			case LUA_TSTRING:
			{
				log.Write(LogLevel::Info, LogCategory::Engine, "-%d: `%s'", i, lua_tostring(a_luaState, i));
				break;
			}
			case LUA_TBOOLEAN:
			{
				log.Write(LogLevel::Info, LogCategory::Engine, "-%d: %s", i, lua_toboolean(a_luaState, i) ? "true" : "false");
				break;
			}
			case LUA_TNUMBER:
			{
				log.Write(LogLevel::Info, LogCategory::Engine, "-%d: %g", i, lua_tonumber(a_luaState, i));
				break;
			}
			default:
			{
				log.Write(LogLevel::Info, LogCategory::Engine, "-%d: %s", i, lua_typename(a_luaState, t));
				break;
			}
		}
	}
}
