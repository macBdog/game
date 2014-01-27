#include "AnimationManager.h"
#include "DebugMenu.h"
#include "InputManager.h"
#include "LuaScript.h"
#include "WorldManager.h"

#include "ScriptManager.h"

template<> ScriptManager * Singleton<ScriptManager>::s_instance = NULL;

const float ScriptManager::s_updateFreq = 1.0f;								///< How often the script manager should check for updates to scripts
const char * ScriptManager::s_mainScriptName = "game.lua";					///< Constant name of the main game script file

// Registration of gui functions
const luaL_Reg ScriptManager::s_guiFuncs[] = {
	{"GetValue", GUIGetValue},
	{"SetValue", GUISetValue},
	{NULL, NULL}
};

// Registration of game object functions
const luaL_Reg ScriptManager::s_gameObjectFuncs[] = {
	{"Create", CreateGameObject},
	{NULL, NULL}
};

// Registration of game object class member functions
const luaL_Reg ScriptManager::s_gameObjectMethods[] = {
	{"GetId", GetGameObjectId},
	{"GetName", GetGameObjectName},
	{"SetName", SetGameObjectName},
	{"GetPosition", GetGameObjectPosition},
	{"SetPosition", SetGameObjectPosition},
	{"GetRotation", GetGameObjectRotation},
	{"SetRotation", SetGameObjectRotation},
	{"GetScale", GetGameObjectScale},
	{"SetScale", SetGameObjectScale},
	{"GetLifeTime", GetGameObjectLifeTime},
	{"SetLifeTime", SetGameObjectLifeTime},
	{"EnableCollision", EnableGameObjectCollision},
	{"DisableCollision", DisableGameObjectCollision},
	{"HasCollisions", TestGameObjectCollisions},
	{"GetCollisions", GetGameObjectCollisions},
	{"PlayAnimation", PlayGameObjectAnimation},
	{"Destroy", DestroyGameObject},
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
		lua_register(m_globalLua, "DestroyGameObject", DestroyGameObject);
		lua_register(m_globalLua, "IsKeyDown", IsKeyDown);
		lua_register(m_globalLua, "Yield", YieldLuaEnvironment);
		lua_register(m_globalLua, "DebugPrint", DebugPrint);
		lua_register(m_globalLua, "DebugLog", DebugLog);

		// Register C++ functions available on the global GUI
		lua_newtable(m_globalLua);
		luaL_setfuncs(m_globalLua, s_guiFuncs, 0);
		lua_setglobal(m_globalLua, "GUI");

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
			Log::Get().Write(LogLevel::Error, LogCategory::Game, "Fatal script error: %s\n", lua_tostring(m_gameLua, -1));
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
		fileMan.CleanupFileList(scriptFiles);
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
				Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in script %s, reloading.", curScript->m_path);

				// Clean up any script-owned objects
				WorldManager::Get().DestoryAllScriptOwnedObjects();

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

GameObject * ScriptManager::CheckGameObject(lua_State * a_luaState, unsigned int a_argumentId)
{
	// Retrieve the object from a userdata reference
	luaL_checktype(a_luaState, a_argumentId, LUA_TUSERDATA);
	unsigned int * objId = (unsigned int*)(lua_touserdata(a_luaState, a_argumentId));
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
        return luaL_error(a_luaState, "GameObject:Create error, expecting 2 argument: class (before the scope operator) then template name."); 
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
		Log::Get().WriteGameErrorNoParams("GUIGetValue expects 1 parameter: name of the element to get.");
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
		if (guiName != NULL && newText != NULL)
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
		Log::Get().WriteGameErrorNoParams("GUISetValue expects 2 parameters: name of the element to set and the string to set.");
	}

	Log::Get().WriteGameErrorNoParams("GUISetValue could not find the GUI element to set a value on.");
	return 0;
}

int ScriptManager::DebugPrint(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		const char * debugText = lua_tostring(a_luaState, 1);
		if (debugText != NULL && debugText != NULL)
		{
			DebugMenu::Get().ShowScriptDebugText(debugText);
		}
	}
	else
	{
		Log::Get().WriteGameErrorNoParams("DebugPrint at least one parameter to show on the screen.");
	}

	return 0;
}

int ScriptManager::DebugLog(lua_State * a_luaState)
{
	if (lua_gettop(a_luaState) == 1)
	{
		luaL_checktype(a_luaState, 1, LUA_TSTRING);
		const char * debugText = lua_tostring(a_luaState, 1);
		if (debugText != NULL && debugText != NULL)
		{
			Log::Get().Write(LogLevel::Info, LogCategory::Game, debugText);
		}
	}
	else
	{
		Log::Get().WriteGameErrorNoParams("DebugLog expects just one parameter to show on the screen.");
	}

	return 0;
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
	}
	 
	LogScriptError(a_luaState, "SetName", "expects 1 string parameter.");

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
			Vector rot = gameObj->GetRot();
			lua_pushnumber(a_luaState, rot.GetX());
			lua_pushnumber(a_luaState, rot.GetY());
			lua_pushnumber(a_luaState, rot.GetZ());
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
		LogScriptError(a_luaState, "SetScale", "expects no parameters.");
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
			while (curCol != NULL)
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
