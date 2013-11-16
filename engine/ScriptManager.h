#ifndef _ENGINE_SCRIPT_MANAGER_
#define _ENGINE_SCRIPT_MANAGER_
#pragma once

#include "../core/LinkedList.h"

#include "FileManager.h"
#include "Singleton.h"

class GameObject;
struct lua_State;
struct luaL_Reg;

//\brief ScriptManager provides access from LUA to C++ functions
class ScriptManager : public Singleton<ScriptManager>
{
public:

	//\ No work done in the constructor, only Init
	ScriptManager(float a_updateFreq = s_updateFreq) 
		: m_globalLua(NULL)
		, m_gameLua(NULL)
		, m_lastFrameDelta(0.0f)
		, m_updateFreq(a_updateFreq)
		, m_updateTimer(0.0f)
		{ m_scriptPath[0] = '\0'; }
	~ScriptManager() { Shutdown(); }

	//\brief Set clear colour buffer and depth buffer setup 
    bool Startup(const char * a_scriptPath);
	bool Shutdown();

	//\brief Update managed texture
	bool Update(float a_dt);

	//\brief Remove the script side version of an object so the script isn't left hanging
	//\param a_gameObject pointer to object to unbind and set null on script side
	void DestroyObjectScriptBindings(GameObject * a_gameObj);

private:

	static const float s_updateFreq;							///< How often the script manager should check for script updates
	static const char * s_mainScriptName;						///< Constant name of the main game script file
	static const luaL_Reg s_guiFuncs[];							///< Constant array of functions registered for for the GUI global
	static const luaL_Reg s_gameObjectFuncs[];					///< Constant array of functions registered for for the GameObject global
	static const luaL_Reg s_gameObjectMethods[];				///< Constant array of functions registered for game object members

	//\brief Called by LUA during each update to allow the game to run
	static int YieldLuaEnvironment(lua_State * a_luaState);

	//\brief Verify a user data pointer passed by script is a valid GameObject pointer
	//\param a_luaState pointer to the LUA state to interrogate
	//\param a_argumentId the id of the user data pointer in the LUA registry
	//\return a pointer to a GameObject or null if unvalid
	static GameObject * CheckGameObject(lua_State * a_luaState, unsigned int a_argumentId = 1U);

	//\brief Register a new metatable with LUA for gameobject lookups
	//\param a_luaState pointer to the LUA state to operate on
	static int RegisterGameObject(lua_State * a_luaState);

	//\brief LUA versions of game functions made avaiable from C++
	static int GetFrameDelta(lua_State * a_luaState);
	static int CreateGameObject(lua_State * a_luaState);
	static int GetGameObject(lua_State * a_luaState);
	static int DestroyGameObject(lua_State * a_luaState);
	static int IsKeyDown(lua_State * a_luaState);
	static int GUIGetValue(lua_State * a_luaState);
	static int GUISetValue(lua_State * a_luaState);
	static int DebugPrint(lua_State * a_luaState);

	//brief LUA versions of game object functions
	static int GetGameObjectId(lua_State * a_luaState);
	static int GetGameObjectName(lua_State * a_luaState);
	static int SetGameObjectName(lua_State * a_luaState);
	static int GetGameObjectPosition(lua_State * a_luaState);
	static int SetGameObjectPosition(lua_State * a_luaState);
	static int GetGameObjectRotation(lua_State * a_luaState);
	static int SetGameObjectRotation(lua_State * a_luaState);
	static int EnableGameObjectCollision(lua_State * a_luaState);
	static int DisableGameObjectCollision(lua_State * a_luaState);
	static int TestGameObjectCollisions(lua_State * a_luaState);
	static int GetGameObjectCollisions(lua_State * a_luaState);

	//\brief A managed script stores info critical to the hot reloading of scripts
	struct ManagedScript
	{
		ManagedScript(const char * a_scriptPath, const FileManager::Timestamp & a_timeStamp)
			: m_timeStamp(a_timeStamp)	{ strcpy(&m_path[0], a_scriptPath);	}
		FileManager::Timestamp m_timeStamp;						///< When the script file was last edited
		char m_path[StringUtils::s_maxCharsPerLine];			///< Where the script resides for reloading
	};

	typedef LinkedListNode<ManagedScript> ManagedScriptNode;	///< Alias for a linked list node that points to a managed script
	typedef LinkedList<ManagedScript> ManagedScriptList;		///< Alias for a linked list of managed scripts

	ManagedScriptList m_managedScripts;							///< List of all the scripts found on disk at startup
	lua_State * m_globalLua;									///< Globally scoped LUA environment for game and GUI
	lua_State * m_gameLua;										///< Seperate LUA execution process for the game thread to run on
	char m_scriptPath[StringUtils::s_maxCharsPerLine];			///< Cache off path to scripts 
	float m_lastFrameDelta;										///< Cache of the delta for the last update received by the script manager
	float m_updateFreq;											///< How often the script manager should check for changes to shaders
	float m_updateTimer;										///< If we are due for a scan and update of scripts
};

#endif // _ENGINE_SCRIPT_MANAGER
