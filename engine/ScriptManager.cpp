#include "LuaScript.h"
#include "WorldManager.h"

#include "ScriptManager.h"

template<> ScriptManager * Singleton<ScriptManager>::s_instance = NULL;

bool ScriptManager::Startup()
{
	// Open Lua and load the standard libraries
	if (m_lua = luaL_newstate())
	{
		luaL_requiref(m_lua, "io", luaopen_io, 1);
		luaL_requiref(m_lua, "base", luaopen_base, 1);
		luaL_requiref(m_lua, "table", luaopen_table, 1);
		luaL_requiref(m_lua, "string", luaopen_string, 1);
		luaL_requiref(m_lua, "math", luaopen_math, 1);

		// Register C++ functions made accessible to LUA
		lua_register(m_lua, "GetGameObject", GetGameObject);
	}

	return m_lua != NULL;
}

bool ScriptManager::Shutdown()
{
	// Clean up global lua object
	if (m_lua != NULL)
	{
		lua_close(m_lua);
	}
	return true;
}

bool ScriptManager::Load(const char * a_scriptPath, bool a_execute)
{
	if (m_lua != NULL)
	{
		if (luaL_loadfile(m_lua, a_scriptPath) == 0)
		{
			// Call code located in script file...
			if (a_execute)
			{
				if (lua_pcall(m_lua, 0, LUA_MULTRET, 0) != 0)
				{
					// Report ant remove error message from stack
					Log::Get().Write(Log::LL_ERROR, Log::LC_GAME, lua_tostring(m_lua, -1));
					lua_pop(m_lua, 1);
				}
				else
				{
					return true;
				}
			}
		}
		else
		{
			Log::Get().Write(Log::LL_ERROR, Log::LC_GAME, "Cannot find script file %s.", a_scriptPath);
		}
	}

	return false;
}

bool ScriptManager::LoadGameObjectScript(const char * a_scriptName, GameObject * a_gameObject)
{
	// Check if a reference has already been generated
	int ref = a_gameObject->GetScript();
	if (ref != LUA_NOREF && ref != LUA_REFNIL) 
	{
		lua_pushinteger(m_lua, ref);
		lua_gettable(m_lua, LUA_REGISTRYINDEX);
	} 
	else 
	{
		unsigned int * userData = (unsigned int *)lua_newuserdata(m_lua, sizeof(GameObject));
		*userData = a_gameObject->GetId();
		luaL_getmetatable(m_lua, "GameObject");
		lua_setmetatable(m_lua, -2);

		// Generate the registry reference
		lua_pushinteger(m_lua, -1);
		ref = luaL_ref(m_lua, LUA_REGISTRYINDEX);
		a_gameObject->SetScript(ref);
	}

	return ref != LUA_NOREF && ref != LUA_REFNIL;
}

bool ScriptManager::GameObjectUpdate(GameObject * a_gameObject, float a_dt)
{
	// Early out for bad script reference
	if(a_gameObject == NULL || a_gameObject->GetScript() < 0)
	{
		return false;
	}
	
	// Call update
	lua_pushinteger(m_lua, a_gameObject->GetId());
	lua_gettable(m_lua, LUA_REGISTRYINDEX);

	// Push delta time argument and call function
	lua_pushnumber(m_lua, a_dt);
	if (lua_pcall(m_lua, 1, 1, 0) != 0)
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_GAME, "Missing script Update function for game object called %s", a_gameObject->GetName());
		return false;
	}
	
    // Return result from script
	bool luaRetVal = lua_toboolean(m_lua, -1) == 0;
    lua_pop(m_lua, 1);
     
	return luaRetVal;
}

bool ScriptManager::FreeGameObjectScript(GameObject * a_gameObject)
{
	if (a_gameObject != NULL)
	{
		// Clear the registry reference out if set
		if (a_gameObject->GetScript() != LUA_NOREF && a_gameObject->GetScript() != LUA_REFNIL) 
		{
			lua_pushinteger(m_lua, a_gameObject->GetScript());
			lua_pushnil(m_lua);
			lua_settable(m_lua, LUA_REGISTRYINDEX);
			a_gameObject->SetScript(-1);
		}
	}

	return false;
}


bool ScriptManager::Update(float a_dt)
{
	return false;
}

int ScriptManager::GetGameObject(lua_State * m_luaState)
{
	if (m_luaState == NULL)
	{
		return -1;
	}

	int numArgs = lua_gettop(m_luaState);
	lua_pushnumber(m_luaState, 123); // return value
	return 1;
}