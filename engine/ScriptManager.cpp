#include "LuaScript.h"
#include "WorldManager.h"

#include "ScriptManager.h"

template<> ScriptManager * Singleton<ScriptManager>::s_instance = NULL;

const char * ScriptManager::s_mainScriptName = "game.lua";					///< Constant name of the main game script file

// Registration of game object class members
const luaL_Reg ScriptManager::s_gameObjectFuncs[] = {
	{"Create", CreateGameObject},
	{"GetPosition", GetGameObjectPosition},
	{"SetPosition", SetGameObjectPosition},
	{"GetName", GetGameObjectName},
	{"SetName", SetGameObjectName},
	{NULL, NULL}
};

bool ScriptManager::Startup(const char * a_scriptPath)
{
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
		lua_register(m_globalLua, "Yield", YieldLuaEnvironment);

		// Register C++ functions for GameObjects referenced with : in LUA
		luaL_requiref(m_globalLua, "GameObject", RegisterGameObject, 1);
		
		// Register metatable for user data in registry
		/*luaL_newmetatable(a_luaState, "GameObject.Registry");
		lua_pushvalue(a_luaState, -1);
		lua_setfield(a_luaState, -2, "__index"); */

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
	}

	return m_globalLua != NULL;
}

bool ScriptManager::Shutdown()
{
	// Clean up game thread
	if (m_gameLua != NULL)
	{
		lua_close(m_gameLua);
	}

	// Clean up global lua object
	if (m_globalLua != NULL)
	{
		lua_close(m_globalLua);
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

	// Call back to LUA main thread
	if (m_gameLua != NULL)
	{
		lua_resume(m_gameLua, NULL, 0);
		return true;
	}
	return false;
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

GameObject * ScriptManager::CheckGameObject(lua_State * a_luaState, int a_index)
{
	// Check the userdata passed from LUA matches the registered format
	void * userData = 0;
	luaL_checktype(a_luaState, a_index, LUA_TTABLE); 
	lua_getfield(a_luaState, a_index, "__self");
	userData = luaL_checkudata(a_luaState, a_index, "GameObject.Registry");
	luaL_argcheck(a_luaState, userData != 0, 0, "GameObject.Registry expected");  
  
	return *((GameObject**)userData);      
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

	// First argument is now a table that represent the class to instantiate
	luaL_checktype(a_luaState, 1, LUA_TTABLE);   
	
	// Create table to represent instance
    lua_newtable(a_luaState);    

    // Set first argument of new to metatable of instance
    lua_pushvalue(a_luaState, 1);       
    lua_setmetatable(a_luaState, -2);

    // Do function lookups in metatable
    lua_pushvalue(a_luaState, 1);
    lua_setfield(a_luaState, 1, "__index");  

    // Allocate memory for a pointer to to object
	GameObject **gameObjectUserData = (GameObject **)lua_newuserdata(a_luaState, sizeof(GameObject *));
	
	// Create the new object
	if (GameObject * newGameObject = WorldManager::Get().CreateObject<GameObject>(templatePath))
	{
		// Make sure the object is not managed by the scene
		newGameObject->SetScriptOwned();
		*gameObjectUserData = newGameObject;

		// Get metatable 'GameObject.Registry' store in the registry
		luaL_getmetatable(a_luaState, "GameObject.Registry");

		// Set user data for Sprite to use this metatable
		lua_setmetatable(a_luaState, -2);       
    
		// Set field '__self' of instance table to the sprite user data
		lua_setfield(a_luaState, -2, "__self");  

		// TODO - Potential memory leak here from the new user data never being released, see game object destruction
	}

	return 1;
}

int ScriptManager::GetGameObject(lua_State * a_luaState)
{
	return 1;
}

int ScriptManager::DestroyGameObject(lua_State * a_luaState)
{
	return 1;
}

int ScriptManager::GetGameObjectPosition(lua_State * a_luaState)
{
	return 1;
}

int ScriptManager::SetGameObjectPosition(lua_State * a_luaState)
{
	return 1;
}

int ScriptManager::GetGameObjectName(lua_State * a_luaState)
{
	return 1;
}

int ScriptManager::SetGameObjectName(lua_State * a_luaState)
{
	int n = lua_gettop(a_luaState);  // Number of arguments
  
	if (n == 2) 
	{
		// Retrieve the game object from the LUA metatable using the index
		lua_Number gameObjectIndex = luaL_checknumber(a_luaState, 1);
		if (GameObject * gameObj = CheckGameObject(a_luaState, (int)gameObjectIndex))
		{
			gameObj->SetName(luaL_checkstring(a_luaState, 2));
		}
	}
	else
	{
		luaL_error(a_luaState, "SetGameObjectName Got %d arguments, execting 2 (self, name)", n); 
	}
  
  return 1;
}
