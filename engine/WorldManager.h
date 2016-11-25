#pragma once

#include <iostream>
#include <fstream>

#include "../core/LinkedList.h"
#include "../core/PageAllocator.h"

#include "GameObject.h"
#include "Scene.h"
#include "Singleton.h"
#include "StringUtils.h"

struct Light;
class DataPack;

//\brief WorldManager handles object and scene management.
//		 The current thought is that the world is made of scenes.
//		 And scenes are filled with objects.
class WorldManager : public Singleton<WorldManager>
{
public:

	//\brief Ctor calls through to startup
	WorldManager() 
		: m_totalGameObjects(0)
		, m_currentScene(NULL) { m_templatePath[0] = '\0'; m_scenePath[0] = '\0'; }
	~WorldManager() { Shutdown(); }

	//\brief Initialise memory pools on startup, cleanup worlds objects on shutdown
	//\param a_templatePath is the path to templates for game object creation
	//\param a_scenePath is the path to scene files for partitioning groups of game objects
	//\return bool true if the world and scenes were started without error
	bool Startup(const char * a_templatePath, const char * a_scenePath, const DataPack * a_dataPack);
	bool Shutdown();

	//\brief Update will propogate through all objects in the active scene
	//\return true if a world was update without issue
	bool Update(float a_dt);

	//\brief Create and object from an optional game file template
	//\param a_templatePath Pointer to a cstring with an optional game file to create from
	//\param a_scene a pointer to the scene to add the object to, will try the current if NULL
	//\return A pointer to the newly created game object of NULL for failure
	GameObject * CreateObject(const char * a_templatePath = NULL, Scene * a_scene = NULL);
	
	//\brief Remove a created object from the world
	//\param a_destroyScriptBindings true if the script management bindings should be killed
	//\return true if an object is destroyed
	bool DestroyObject(unsigned int a_objectId, bool a_destroyScriptBindings = false);

	//\brief Destroy all objects in the current scene
	//\param a_destroyScriptOwned bool to specify destruction of scripts owned objects
	//\return true if any objects were destroyed
	void DestroyAllObjects(bool a_destroyScriptOwned = false);

	//\brief Destroy all objects owned by script in the current scene
	//\param a_destroyScriptBindings bool to specify destruction of bindings to scripts, usually wanted as script is going bye bye
	//\return true if any objects were destroyed
	void DestroyAllScriptOwnedObjects(bool a_destroyScriptBindings = true);

	//\brief Get a pointer to an existing object in the world.
	//\param a_objectId the unique game id for this object
	//\return Pointer to a game object in the world
	GameObject * GetGameObject(unsigned int a_objectId);
	GameObject * GetGameObject(const char * a_objName);

	//\brief Get the scene that the world is currently showing
	//\return A pointer to a scene
	inline Scene * GetCurrentScene() { return m_currentScene; }
	inline void SetCurrentScene(Scene * a_scene) { m_currentScene = a_scene; }
	Scene * GetScene(const char * a_sceneName);
	void SetCurrentScene(const char * a_sceneName);
	void SetNewScene(const char * a_sceneName);

	//\brief Accessor for the relative paths
	inline const char * GetTemplatePath() { return m_templatePath; }
	inline const char * GetScenePath() { return m_scenePath; }

private:

	//\brief An object lookup maps a globally unique ID to a scene and object storage ID for that scene
	struct ObjectLookup
	{
		ObjectLookup() : m_scene(NULL), m_storageId(0) {}
		Scene * m_scene;			///< The scene that stores the object
		unsigned int m_storageId;	///< The position in the scene's storage array of the object
	};

	static const int s_numLookup = 655360;					///< Each ObjectLookup is about 48 bits, 20M of total lookup storage

	//\brief Alias to refer to a group of objects
	typedef LinkedListNode<Scene> SceneNode;
	PageAllocator<ObjectLookup> m_objectLookup;				///< Collection of lookups for finding game objects in O(1) time
	LinkedList<Scene> m_scenes;								///< All the currently loaded scenes are added to this list
	Scene * m_currentScene;									///< The currently active scene
	unsigned int m_totalGameObjects;						///< Total object count across all scenes, drives ID creation
	char m_templatePath[StringUtils::s_maxCharsPerLine];	///< Path for templates
	char m_scenePath[StringUtils::s_maxCharsPerLine];		///< Path for scene files
};
