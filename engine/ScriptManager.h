#ifndef _ENGINE_SCRIPT_MANAGER_
#define _ENGINE_SCRIPT_MANAGER_
#pragma once

#include "Singleton.h"

class GameObject;
struct lua_State;

//\brief ScriptManager provides access from LUA to C++ functions
class ScriptManager : public Singleton<ScriptManager>
{
public:

	//\ No work done in the constructor, only Init
	ScriptManager() : m_lua(NULL) { }
	~ScriptManager() { Shutdown(); }

	//\brief Set clear colour buffer and depth buffer setup 
    bool Startup();
	bool Shutdown();

	//\brief Load a script into the global lua context and optionally call it
	bool Load(const char * a_scriptPath, bool a_execute = true);

	//\brief Load a game object script into it's own registry table
	bool LoadGameObjectScript(const char * a_scriptName, GameObject * a_gameObject);
	bool GameObjectUpdate(GameObject * a_gameObject, float a_dt);
	bool FreeGameObjectScript(GameObject * a_gameObject);

	//\brief Update managed texture
	bool Update(float a_dt);

private:

	// LUA versions of functions made avaiable from C++
	static int GetGameObject(lua_State * m_luaState);

	lua_State * m_lua;	// Globally scoped LUA environment for everything, may need revision
};

#endif // _ENGINE_SCRIPT_MANAGER
