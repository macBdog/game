#ifndef _ENGINE_WORLD_MANAGER_H_
#define _ENGINE_WORLD_MANAGER_H_

#include <iostream>
#include <fstream>

#include "../core/LinkedList.h"

#include "GameObject.h"
#include "Singleton.h"
#include "StringUtils.h"

//\brief A scene is a subset of a world, containing objects that are fixed and floating
class Scene
{

public:
	
	//\brief SceneState keeps track of which scenes are loaded
	enum SceneState
	{
		eSceneState_Unloaded = 0,	///< Not rendering or updating
		eSceneState_Loading,		///< Loading settings and game objects
		eSceneState_Active,			///< Updating and rendering
		
		eSceneState_Count,
	};

	//\brief Set scene count to 0 on construction
	Scene() 
		: m_numObjects(0)
		, m_state(eSceneState_Unloaded) { sprintf(m_name, "Scene01"); }

	//\brief Adding and removing objects from the scene
	void AddObject(GameObject * a_newObject);
	GameObject * GetSceneObject(unsigned int a_objectId);

	//\brief Update all the objects in the scene
	bool Update(float a_dt);

	//\brief Get the number of objects in the scene
	//\return uint of the number of objects
	inline unsigned int GetNumObjects() { return m_numObjects; }
	
	//\brief Resource mutators and accessors
	inline void SetName(const char * a_name) { sprintf(m_name, "%s", a_name); }

	//\brief Write all objects in the scene out to a scene file
	void Serialise();

private:

	//\brief Alias to refer to a group of objects
	typedef LinkedListNode<GameObject> SceneObject;
	typedef LinkedList<GameObject> SceneObjects;

	//\brief Draw will cause active objects in the scene to submit resources to the render manager
	//\return true if resources were submitted without issue
	bool Draw();

	SceneObjects m_objects;							///< All the objects in the current scene
	char m_name[StringUtils::s_maxCharsPerName];	///< Scene name for serialization
	unsigned int m_numObjects;						///< How many objects are in the default scene
	SceneState m_state;								///< What state the scene is in
};

//\brief WorldManager handles object and scene management.
//		 The current thought is that the world is made of scenes.
//		 And scenes are filled with objects.
class WorldManager : public Singleton<WorldManager>
{
public:

	//\brief Ctor calls through to startup
	WorldManager() : m_totalSceneNumObjects(0) { }
	~WorldManager() { Shutdown(); }

	//\brief Initialise memory pools on startup, cleanup worlds objects on shutdown
	//\param a_templatePath is the path to templates for game object creation
	//\param a_scenePath is the path to scene files for partitioning groups of game objects
	//\return bool true if the world and scenes were started without error
	bool Startup(const char * a_templatePath, const char * a_scenePath);
	bool Shutdown();

	//\brief Update will propogate through all objects in the active scene
	//\return true if a world was update without issue
	bool Update(float a_dt);

	//\brief Create and object from an optional game file template
	//\param a_templatePath Pointer to a cstring with an optional game file to create from
	//\return A pointer to the newly created game object of NULL for failure
	GameObject * CreateObject(const char * a_templatePath = NULL);

	//\brief Remove a created object from the world, will not be destroyed right away
	bool RemoveObject(unsigned int a_objectId) { return false; }

	//\brief Get a pointer to an existing object in the world.
	//\param a_objectId the unique game id for this object
	//\return Pointer to a game object in the world
	GameObject * GetGameObject(unsigned int a_objectId);

	//\brief Get the scene that the world is currently showing
	//\return A pointer to a scene
	Scene * GetCurrentScene() { return &m_defaultScene; }

	//\brief Accessor for the relative paths
	inline const char * GetTemplatePath() { return m_templatePath; }
	inline const char * GetScenePath() { return m_scenePath; }

private:

	Scene	m_defaultScene;									///< Eventually scenes can be added and removed by the user - multiple at once?
	unsigned int m_totalSceneNumObjects;					///< Total object count across all scenes, drives ID creation
	char	m_templatePath[StringUtils::s_maxCharsPerLine]; ///< Path for templates
	char	m_scenePath[StringUtils::s_maxCharsPerLine];	///< Path for scene files
};

#endif /* _ENGINE_WORLD_MANAGER_H_ */
