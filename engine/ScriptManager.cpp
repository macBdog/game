#include "DebugMenu.h"
#include "InputManager.h"
#include "LuaScript.h"
#include "WorldManager.h"

#include "ScriptManager.h"

template<> ScriptManager * Singleton<ScriptManager>::s_instance = NULL;

const float ScriptManager::s_updateFreq = 1.0f;								///< How often the script manager should check for updates to scripts
const char * ScriptManager::s_mainScriptName = "game.lua";					///< Constant name of the main game script file

// Registration of game object functions
const luaL_Reg ScriptManager::s_gameObjectFuncs[] = {
	{"Create", CreateGameObject},
	{NULL, NULL}
};

// Registration of game object class member functions
const luaL_Reg ScriptManager::s_gameObjectMethods[] = {
	{"GetPosition", GetGameObjectPosition},
	{"SetPosition", SetGameObjectPosition},
	{"GetName", GetGameObjectName},
	{"SetName", SetGameObjectName},
	{"DestroyGameObject", DestroyGameObject},
	{"__gc", DestroyGameObject},
	{NULL, NULL}
};

bool ScriptManager::Startup(const char * a_scriptPath)
{
	// As this method is called when scripts reload, make sure the global state is dead
	if (m_globalLua != NULL || m_managedScripts.GetLength() > 0)
	{
		Shutdown();
	}
	
	// Open Lua and load the standard libraries
	if (m_globalLua = luaL_newstate())
	{
		luaL_requiref(m_globalLua, "io", luaopen_io, 1);
		luaL_requiref(m_globalLua, "base", luaopen_base, 1);
		luaL_requiref(m_globalLua, "table", luaopen_table, 1);
		luaL_requiref(m_globalLua, "string", luaopen_string, 1);
		luaL_requiref(m_globalLua, "math", luaopen_math, 1);

		// Register C++ functions made accessible to LUA
		lua_register(m_globalLua, "GetFrameDelta", GetFrameDelta);
		lua_register(m_globalLua, "CreateGameObject", CreateGameObject);
		lua_register(m_globalLua, "GetGameObject", GetGameObject);
		lua_register(m_globalLua, "DestroyGameObject", DestroyGameObject);
		lua_register(m_globalLua, "IsKeyDown", IsKeyDown);
		lua_register(m_globalLua, "Yield", YieldLuaEnvironment);

		// Register C++ functions available on the global GameObject
		lua_newtable(m_globalLua);
		luaL_setfuncs(m_globalLua, s_gameObjectFuncs, 0);
		lua_setglobal(m_globalLua, "GameObject");

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
		
		// And kick it off and wait for a yield
		m_gameLua = lua_newthread(m_globalLua);
		luaL_loadfile(m_gameLua, gameScriptPath);
		if (lua_resume(m_gameLua, NULL, 0) != LUA_YIELD)
		{
			Log::Get().WriteEngineErrorNoParams("Fatal script error! Game script must yield to engine, or there is an error in game.lua");
		}

		// Scan all the scripts in the dir for changes to trigger a reload
		FileManager & fileMan = FileManager::Get();
		FileManager::FileList scriptFiles;
		fileMan.FillFileList(m_scriptPath, scriptFiles, ".lua");

		// Add each script in the directory
		FileManager::FileListNode * curNode = scriptFiles.GetHead();
		while(curNode != NULL)
		{
			// Get a fresh timestamp on the new script
			char fullPath[StringUtils::s_maxCharsPerLine];
			sprintf(fullPath, "%s%s", m_scriptPath, curNode->GetData()->m_name);
			FileManager::Timestamp curTimeStamp;
			FileManager::Get().GetFileTimeStamp(fullPath, curTimeStamp);

			// Add managed script to the list
			ManagedScript * manScript = new ManagedScript(fullPath, curTimeStamp);
			ManagedScriptNode * manScriptNode = new ManagedScriptNode();
			if (manScript != NULL && manScriptNode != NULL)
			{
				manScriptNode->SetData(manScript);
				m_managedScripts.Insert(manScriptNode);
			}
			curNode = curNode->GetNext();
		}

		// Clean up the list of fonts
		fileMan.EmptyFileList(scriptFiles);
	}

	return m_globalLua != NULL;
}

bool ScriptManager::Shutdown()
{
	// Clean up global lua object which will clean up the game thread
	if (m_globalLua != NULL)
	{
		lua_close(m_globalLua);
		m_globalLua = NULL;
		m_gameLua = NULL;
	}

	// And any managed scripts
	ManagedScriptNode * next = m_managedScripts.GetHead();
	while (next != NULL)
	{
		// Cache off next pointer
		ManagedScriptNode * cur = next;
		next = cur->GetNext();

		m_managedScripts.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	return true;
}

bool ScriptManager::LoadGUIScript(const char * a_scriptPath, bool a_execute)
{
	if (m_globalLua == NULL)
	{
		Log::Get().WriteEngineErrorNoParams("GUI Script executed before global LUA setup!");
		return false;
	}

	// Create the GUI thread 
	if (m_guiLua == NULL)
	{
		m_guiLua = lua_newthread(m_globalLua);
	}

	// Failed to create thread 
	if (m_guiLua == NULL)
	{
		Log::Get().WriteEngineErrorNoParams("Failed to create thread for GUI script!");
		return false;
	}

	// Call code located in script file...
	if (luaL_loadfile(m_guiLua, a_scriptPath) == 0)
	{
		if (a_execute)
		{
			// Note: for later execution, call lua_pcall(m_globalLua, 0, LUA_MULTRET, 0)
		}
		else
		{
			Log::Get().Write(Log::LL_ERROR, Log::LC_GAME, "Cannot find script file %s.", a_scriptPath);
		}
	}
	else
	{
		// Report and remove error message from stack
		Log::Get().Write(Log::LL_ERROR, Log::LC_GAME, lua_tostring(m_guiLua, -1));
		lua_pop(m_guiLua, 1);	
	}

	return true;
}

bool ScriptManager::Update(float a_dt)
{
	// Cache off last delta
	m_lastFrameDelta = a_dt;

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
		ManagedScriptNode * next = m_managedScripts.GetHead();
		while (next != NULL)
		{
			// Get a fresh timestampt and test it against the stored timestamp
			FileManager::Timestamp curTimeStamp;
			ManagedScript * curScript = next->GetData();
			FileManager::Get().GetFileTimeStamp(curScript->m_path, curTimeStamp);
			if (curTimeStamp > curScript->m_timeStamp)
			{
				scriptsReloaded = true;
				curScript->m_timeStamp = curTimeStamp;
				Log::Get().Write(Log::LL_INFO, Log::LC_ENGINE, "Change detected in script %s, reloading.", curScript->m_path);

				// Clean up any script-owned objects
				WorldManager::Get().DestoryAllScriptsOwnedObjects();

				// Kick the script VM in the guts
				Shutdown();
				Startup(m_scriptPath);
				return true;
			}

			next = next->GetNext();
		}
	}

	// Don't update scripts while debugging
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		return true;
	}

	// Call back to LUA main thread
	if (m_gameLua != NULL)
	{
		lua_resume(m_gameLua, NULL, 0);
		return true;
	}
	return false;
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

int ScriptManager::YieldLuaEnvironment(lua_State * a_luaState)
{
	 return lua_yield(a_luaState, 0);
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

GameObject * ScriptManager::CheckGameObject(lua_State * a_luaState)
{
	// Retrieve the object from a userdata reference
	luaL_checktype(a_luaState, 1, LUA_TUSERDATA);
	unsigned int * objId = (unsigned int*)(lua_touserdata(a_luaState, 1));
	return WorldManager::Get().GetGameObject(*objId);	
}

int ScriptManager::CreateGameObject(lua_State * a_luaState)
{
	if (a_luaState == NULL)
	{
		return -1;
	}

	int numArgs = lua_gettop(a_luaState);
    if (numArgs != 2) 
	{
        return luaL_error(a_luaState, "CreateGameObject error, expecting 2 arguments: class and template name."); 
	}  

	// Qualify template path with template extension
	const char * templateName = luaL_checkstring(a_luaState, 2);
	char templatePath[StringUtils::s_maxCharsPerName];
	templatePath[0] = '\n';
	if (strstr(templateName, ".tmp") == NULL)
	{
		sprintf(templatePath, "%s.tmp", templateName);
	}
	else // Just copy chars over from LUA string
	{
		strcpy(templatePath, templateName);
	}

	// Create the new object
	if (GameObject * newGameObject = WorldManager::Get().CreateObject<GameObject>(templatePath))
	{
		unsigned int * userData = (unsigned int*)lua_newuserdata(a_luaState, sizeof(unsigned int));
		*userData = newGameObject->GetId();

		luaL_getmetatable(a_luaState, "GameObject.Registry");
		lua_setmetatable(a_luaState, -2);

		// Generate the registry reference
		lua_pushvalue(a_luaState, -1);
		int ref = luaL_ref(a_luaState, LUA_REGISTRYINDEX);
		newGameObject->SetScriptReference(ref);
	}

	return 1; // Userdata is returned
}

int ScriptManager::GetGameObject(lua_State * a_luaState)
{
	return 1;
}

int ScriptManager::DestroyGameObject(lua_State * a_luaState)
{
	return 0;
}

int ScriptManager::IsKeyDown(lua_State * a_luaState)
{
	bool keyIsDown = false;
	if (lua_gettop(a_luaState) == 1)
	{
		int keyCode = (int)lua_tonumber(a_luaState, 1);
		keyIsDown = InputManager::Get().IsKeyDepressed((SDLKey)keyCode);
	}
	else
	{
		Log::Get().WriteGameErrorNoParams("IsKeyDown expects 1 parameter of the key code to test.");
	}

	lua_pushboolean(a_luaState, keyIsDown);
	return 1; // One bool returned
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
			Log::Get().WriteGameErrorNoParams("GetGameObjectPosition cannot find the game object referred to.");
		}
	}
	else
	{
		Log::Get().WriteGameErrorNoParams("GetGameObjectPosition expects no parameters.");
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
			Log::Get().WriteGameErrorNoParams("SetGameObjectPosition could not find game object referred to.");
		}
	}
	else // Wrong number of parms
	{
		Log::Get().WriteGameErrorNoParams("SetGameObjectPosition expects 3 number parameters.");
	}
	return 0;
}

int ScriptManager::GetGameObjectName(lua_State * a_luaState)
{
	return 1;
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
	}
	 
	// Log error to user
	Log::Get().WriteGameErrorNoParams("SetGameObjectName expects 1 string parameter.");
	return 0;
}
