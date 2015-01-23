#ifndef _ENGINE_SCRIPT_MANAGER_
#define _ENGINE_SCRIPT_MANAGER_
#pragma once

#include "../core/LinkedList.h"

#include "FileManager.h"
#include "Singleton.h"

class GameObject;
class Widget;
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
		, m_forceReloadScripts(false)
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

	//\brief Trigger the script files to be torn down reloaded as would happen when the on disk file changes
	inline void ReloadScripts() { m_updateTimer = m_updateFreq + 1.0f; m_forceReloadScripts = true; }

	//\brief Callback handler for when a widget with a script binding is clicked
	//\param A pointer to the widget that was interacted with
	bool OnWidgetAction(Widget * a_widget);

private:

	static const float s_updateFreq;							///< How often the script manager should check for script updates
	static const char * s_mainScriptName;						///< Constant name of the main game script file
	static const luaL_Reg s_guiFuncs[];							///< Constant array of functions registered for for the GUI global
	static const luaL_Reg s_gameObjectFuncs[];					///< Constant array of functions registered for for the GameObject global
	static const luaL_Reg s_gameObjectMethods[];				///< Constant array of functions registered for game object members

	//\brief Print out an error for the script user with line number and file name
	//\param a_luaState the LUA state that created the error
	//\param a_callingFunctionName name of the function that was called
	//\param a_message the reason the script created an error
	static void LogScriptError(lua_State * a_luaState, const char * a_callingFunctionName, const char * a_message);

	//\brief Called by LUA during each update to allow the game to run
	static int YieldLuaEnvironment(lua_State * a_luaState);

	//\brief Verify a user data pointer passed by script is a valid GameObject pointer
	//\param a_luaState pointer to the LUA state to interrogate
	//\param a_argumentId the id of the user data pointer in the LUA stack
	//\return a pointer to a GameObject or null if unvalid
	static GameObject * CheckGameObject(lua_State * a_luaState, unsigned int a_argumentId = 1U);

	//\brief Register a new metatable with LUA for gameobject lookups
	//\param a_luaState pointer to the LUA state to operate on
	static int RegisterGameObject(lua_State * a_luaState);

	//\brief LUA versions of game functions made avaiable from C++
	static int GetFrameDelta(lua_State * a_luaState);
	static int CreateGameObject(lua_State * a_luaState);
	static int GetGameObject(lua_State * a_luaState);
	static int IsKeyDown(lua_State * a_luaState);
	static int IsGamePadConnected(lua_State * a_luaState);
	static int IsGamePadButtonDown(lua_State * a_luaState);
	static int GetGamePadLeftStick(lua_State * a_luaState);
	static int GetGamePadRightStick(lua_State * a_luaState);
	static int GetGamePadLeftTrigger(lua_State * a_luaState);
	static int GetGamePadRightTrigger(lua_State * a_luaState);
	static int IsMouseLeftButtonDown(lua_State * a_luaState);
	static int IsMouseRightButtonDown(lua_State* a_luaState);
	static int IsMouseMiddleButtonDown(lua_State* a_luaState);
	static int SetCameraPosition(lua_State * a_luaState);
	static int SetCameraRotation(lua_State * a_luaState);
	static int SetCameraFOV(lua_State * a_luaState);
	static int SetCameraTarget(lua_State * a_luaState);
	static int MoveCamera(lua_State * a_luaState);
	static int RotateCamera(lua_State * a_luaState);
	static int RayCollisionTest(lua_State * a_luaState);
	static int NewScene(lua_State * a_luaState);
	static int SetScene(lua_State * a_luaState);

	static int PlaySound(lua_State * a_luaState);
	static int PlayMusic(lua_State * a_luaState);

	static int GUIGetValue(lua_State * a_luaState);
	static int GUISetValue(lua_State * a_luaState);
	static int GUISetColour(lua_State * a_luaState);
	static int GUISetScissor(lua_State * a_luaState);
	static int GUIShowWidget(lua_State * a_luaState);
	static int GUIHideWidget(lua_State * a_luaState);
	static int GUIActivateWidget(lua_State * a_luaState);
	static int GUIGetSelectedWidget(lua_State * a_luaState);
	static int GUISetSelectedWidget(lua_State * a_luaState);
	static int GUISetActiveMenu(lua_State * a_luaState);
	static int GUIEnableMouse(lua_State * a_luaState);
	static int GUIDisableMouse(lua_State * a_luaState);
	static int GUIGetMouseClipPosition(lua_State * a_luaState);
	static int GUIGetMouseScreenPosition(lua_State * a_luaState);
	
	static int DataPackRequire(lua_State * a_luaState);

	static int DebugPrint(lua_State * a_luaState);
	static int DebugLog(lua_State * a_luaState);
	static int DebugLine(lua_State * a_luaState);
	static int DebugPoint(lua_State * a_luaState);

	//brief LUA versions of game object functions
	static int GetGameObjectId(lua_State * a_luaState);
	static int GetGameObjectName(lua_State * a_luaState);
	static int SetGameObjectName(lua_State * a_luaState);
	static int SetGameObjectModel(lua_State * a_luaState);
	static int SetGameObjectShaderData(lua_State * a_luaState);
	static int GetGameObjectPosition(lua_State * a_luaState);
	static int SetGameObjectPosition(lua_State * a_luaState);
	static int GetGameObjectRotation(lua_State * a_luaState);
	static int SetGameObjectRotation(lua_State * a_luaState);
	static int GetGameObjectScale(lua_State * a_luaState);
	static int SetGameObjectScale(lua_State * a_luaState);
	static int ResetGameObjectScale(lua_State * a_luaState);
	static int GetGameObjectLifeTime(lua_State * a_luaState);
	static int SetGameObjectLifeTime(lua_State * a_luaState);
	static int SetGameObjectSleeping(lua_State * a_luaState);
	static int SetGameObjectActive(lua_State * a_luaState);
	static int GetGameObjectSleeping(lua_State * a_luaState);
	static int GetGameObjectActive(lua_State * a_luaState);
	static int EnableGameObjectCollision(lua_State * a_luaState);
	static int DisableGameObjectCollision(lua_State * a_luaState);
	static int TestGameObjectCollisions(lua_State * a_luaState);
	static int GetGameObjectCollisions(lua_State * a_luaState);
	static int PlayGameObjectAnimation(lua_State * a_luaState);
	static int DestroyGameObject(lua_State * a_luaState);

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
	bool m_forceReloadScripts;									///< If some other game system wants to force a reload, it's done here
};

#endif // _ENGINE_SCRIPT_MANAGER
