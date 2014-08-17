#ifndef _ENGINE_WORLD_MANAGER_H_
#define _ENGINE_WORLD_MANAGER_H_

#include <iostream>
#include <fstream>

#include "../core/LinkedList.h"
#include "../core/PageAllocator.h"

#include "GameObject.h"
#include "Log.h"
#include "ModelManager.h"
#include "PhysicsManager.h"
#include "ScriptManager.h"
#include "Singleton.h"
#include "StringUtils.h"

class Shader;

//\brief SceneState keeps track of which scenes are loaded
namespace SceneState
{
	enum Enum
	{
		Unloaded = 0,	///< Not rendering or updating
		Loading,		///< Loading settings and game objects
		Active,			///< Updating and rendering
		Count,
	};
}

//\brief A scene is a subset of a world, containing objects that are fixed and created by the engine and script objects
class Scene
{
	// WorldManager has access to the scene's objects
	friend class WorldManager;	

public:
	
	//\brief Set scene count to 0 on construction
	Scene();
		
	// Cleanup the of objects in the scene on destruction
	~Scene();
	
	//\brief Read scene details from a file into a scene object
	//\param a_scenePath is a string containing the filename of a scene file
	//\param a_sceneToLoad_OUT is a pointer to a scene object to modify with data from the scene path
	bool Load(const char * a_scenePath);

	//\brief Adding and removing objects from the scene
	GameObject * AddObject(unsigned int a_objectId);
	bool RemoveObject(unsigned int a_objectId);
	void RemoveAllObjects(bool a_destroyScriptOwned);
	void RemoveAllScriptOwnedObjects(bool a_destroyScriptBindings);

	//\brief Adding lights to the scene
	//\return false if there are already the maximum number of lights in the scene
	bool AddLight(const char * a_name, const Vector & a_pos, const Vector & a_dir, float a_ambient, float a_diffuse, float a_specular);
	inline const Shader::Light & GetLight(int a_lightId) const { return m_lights[a_lightId]; }
	inline int GetNumLights() const { return m_numLights; }
	inline bool HasLights() const { return m_numLights > 0; }
	inline const char * GetName() const { return m_name; }

	//\brief Get an object by it's ID
	//\param a_objectId is the ID of the object to get
	//\return a pointer a game object if one exists with that name, otherwise NULL
	GameObject * GetSceneObject(unsigned int a_objectId);

	//\brief Get an object by name
	//\param a_objName is the string name of object to get
	//\return a pointer a game object if one exists with that name, otherwise NULL
	GameObject * GetSceneObject(const char * a_objName);
	
	//\brief Get the first object in the scene that intersects with a point in worldspace
	//\param a vector of the point to check agains
	//\return a pointer to a game object or NULL if no hits
	GameObject * GetSceneObject(Vector a_worldPos);

	//\brief Get the first object in the scene that intersects with a line segment
	//\param a_lineStart the start of the line segment to check against
	//\return a pointer to a game object or NULL if no hits
	GameObject * GetSceneObject(Vector a_lineStart, Vector a_lineEnd);

	//\brief Get the first light in the scene that envelops a world position
	//\param a world pos vector near the light
	//\return a pointer to the light or NULL if no light
	const Shader::Light * GetLight(Vector a_worldPos) const;

	//\brief Get the first light in the scene that intersects with a line segment
	//\param a world pos vector near the light
	//\return a pointer to the light or NULL if no light
	const Shader::Light * GetLight(Vector a_lineStart, Vector a_lineEnd) const;

	//\brief Update all the objects in the scene
	bool Update(float a_dt);

	//\brief Called when the game writes to disk and triggers a reload, no reason to reload again
	void ResetFileDateStamp();

	//\brief Get the number of objects in the scene
	//\return uint of the number of objects
	inline unsigned int GetNumObjects() const { return m_numObjects; }
	
	//\brief Resource mutators and accessors
	inline void SetName(const char * a_name) { sprintf(m_name, "%s", a_name); }
	inline void SetFilePath(const char * a_path) { sprintf(m_filePath, "%s", a_path); }
	inline void SetBeginLoaded(bool a_begin) { m_beginLoaded = a_begin; }
	inline bool IsBeginLoaded() { return m_beginLoaded; }
	inline Shader * GetShader() { return m_shader; }
	inline void SetShader(Shader * a_shader) { m_shader = a_shader; }
	
	//\brief Write all objects in the scene out to a scene file
	void Serialise();
	 
private:

	//\brief Draw will cause active objects in the scene to submit resources to the render manager
	//\return true if resources were submitted without issue
	bool Draw();

	static const int s_numObjects = 16000;							///< Each GameObject is about 500 bytes, should be less than 16M
	static const float s_updateFreq;								///< How often the scene should check it's config on disk for updates

	PageAllocator<GameObject> m_objects;							///< Pointer to memory allocated for contiguous game objects
	unsigned int m_numObjects;										///< How many objects are in the scene
	char m_name[StringUtils::s_maxCharsPerName];					///< Scene name for serialization
	char m_filePath[StringUtils::s_maxCharsPerLine];				///< Path of the scene for reloading
	FileManager::Timestamp m_timeStamp;								///< When the scene file was last edited
	float m_updateTimer;											///< The amount of time elapsed since the last file datestamp check
	float m_updateFreq;												///< How often the file resource is checked for changes
	Shader::Light m_lights[Shader::s_maxLights];					///< The lights in the scene
	SceneState::Enum m_state;										///< What state the scene is in
	bool m_beginLoaded;												///< If the scene should be loaded and rendering on startup
	int m_numLights;												///< The number of lights in the scene
	Shader * m_shader;												///< Shader for the whole scene
};

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
	bool Startup(const char * a_templatePath, const char * a_scenePath);
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

#endif /* _ENGINE_WORLD_MANAGER_H_ */
