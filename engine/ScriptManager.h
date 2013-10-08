#ifndef _ENGINE_SCRIPT_MANAGER_
#define _ENGINE_SCRIPT_MANAGER_
#pragma once

#include "Singleton.h"

class GameObject;
struct lua_State;
struct luaL_Reg;

//\brief ScriptManager provides access from LUA to C++ functions
class ScriptManager : public Singleton<ScriptManager>
{
public:

	//\ No work done in the constructor, only Init
	ScriptManager() 
		: m_globalLua(NULL)
		, m_gameLua(NULL)
		, m_guiLua(NULL) 
		, m_lastFrameDelta(0.0f)
		{ m_scriptPath[0] = '\0'; }
	~ScriptManager() { Shutdown(); }

	//\brief Set clear colour buffer and depth buffer setup 
    bool Startup(const char * a_scriptPath);
	bool Shutdown();

	//\brief Load a script into the GUI specific lua context and optionally call it
	bool LoadGUIScript(const char * a_scriptPath, bool a_execute = true);

	//\brief Update managed texture
	bool Update(float a_dt);

private:

	static const char * s_mainScriptName;						///< Constant name of the main game script file
	static const luaL_Reg s_gameObjectFuncs[];					///< Constant array of functions registered for game objects

	//\brief Called by LUA during each update to allow the game to run
	static int YieldLuaEnvironment(lua_State * a_luaState);

	//\brief Verify a user data pointer passed by script is a valid GameObject pointer
	//\param a_luaState pointer to the LUA state to interrogate
	//\param a_index the id of the user data pointer in the LUA registry
	//\return a pointer to a GameObject or null if unvalid
	static GameObject * ScriptManager::CheckGameObject(lua_State * a_luaState, int a_index);

	//\brief Register a new metatable with LUA for gameobject lookups
	//\param a_luaState pointer to the LUA state to operate on
	static int RegisterGameObject(lua_State * a_luaState);

	//\brief LUA versions of game functions made avaiable from C++
	static int GetFrameDelta(lua_State * a_luaState);
	static int CreateGameObject(lua_State * a_luaState);
	static int GetGameObject(lua_State * a_luaState);
	static int DestroyGameObject(lua_State * a_luaState);

	//brief LUA versions of game object functions
	static int GetGameObjectPosition(lua_State * a_luaState);
	static int SetGameObjectPosition(lua_State * a_luaState);
	static int GetGameObjectName(lua_State * a_luaState);
	static int SetGameObjectName(lua_State * a_luaState);

	lua_State * m_globalLua;									///< Globally scoped LUA environment for game and GUI
	lua_State * m_gameLua;										///< Seperate LUA execution process for the game thread to run on
	lua_State * m_guiLua;										///< Separate LUA execution process for the GUI thread to run on
	char m_scriptPath[StringUtils::s_maxCharsPerLine];			///< Cache off path to scripts 
	float m_lastFrameDelta;										///< Cache of the delta for the last update received by the script manager
};

#endif // _ENGINE_SCRIPT_MANAGER
