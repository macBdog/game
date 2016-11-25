#pragma once

#include <iostream>
#include <fstream>

#include "../core/PageAllocator.h"

#include "GameFile.h"
#include "GameObject.h"
#include "FileManager.h"
#include "Shader.h"
#include "StringUtils.h"

struct Light;
class DataPack;

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
	//\param a_scenePath is a pointer to a scene file to load
	//\param a_sceneToLoad_OUT is a pointer to a scene object to modify with data from the scene path
	inline bool Load(const char * a_sceneConfigFilePath) 
	{ 
		if (m_sourceFile.Load(a_sceneConfigFilePath))
		{
			return InitFromConfig();
		}
		return false;
	}
	bool Load(DataPackEntry * a_sceneConfigData)
	{
		if (a_sceneConfigData != NULL && a_sceneConfigData->m_size > 0 && m_sourceFile.Load(a_sceneConfigData))
		{
			return InitFromConfig();
		}
		return false;
	}

	//\brief Adding and removing objects from the scene
	GameObject * AddObject(unsigned int a_objectId);
	void RemoveAllObjects(bool a_destroyScriptOwned);
	void RemoveAllScriptOwnedObjects(bool a_destroyScriptBindings);

	//\brief When an object is removed from the page storage, it is swapped with the last element, so the IDs will need to be remapped
	//\return -1 if the swap last failed, otherwise return the ID of the last object
	bool RemoveObject(unsigned int a_objectId);

	//\brief Adding lights to the scene
	//\return false if there are already the maximum number of lights in the scene
	bool AddLight(const char * a_name, const Vector & a_pos, const Quaternion & a_dir, const Colour & a_ambient, const Colour & a_diffuse, const Colour & a_specular);
	bool RemoveLight(const char * a_name);
	inline Light & GetLight(int a_lightId) { return m_lights[a_lightId]; }
	inline int GetNumLights() const { return m_numLights; }
	inline bool HasLights() const { return m_numLights > 0; }

	//\brief Get the name of the scene
	//\return pointer to C string containing name of scene as referenced in file on disk
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
	Light * GetLightAtPos(Vector a_worldPos);

	//\brief Get the first light in the scene that intersects with a line segment
	//\param a world pos vector near the light
	//\return a pointer to the light or NULL if no light
	Light * GetLight(Vector a_lineStart, Vector a_lineEnd);

	//\brief Update all the objects in the scene
	bool Update(float a_dt);

	//\brief Called when the game writes to disk and triggers a reload, no reason to reload again
	inline void ResetFileDateStamp() { FileManager::Get().GetFileTimeStamp(m_filePath, m_timeStamp); }

	//\brief Get the number of objects in the scene
	//\return uint of the number of objects
	inline unsigned int GetNumObjects() const { return m_objects.GetCount(); }
	
	//\brief Resource mutators and accessors
	inline void SetName(const char * a_name) { strncpy(m_name, a_name, StringUtils::s_maxCharsPerName); }
	inline void SetFilePath(const char * a_path) { strncpy(m_filePath, a_path, StringUtils::s_maxCharsPerLine); }
	inline void SetBeginLoaded(bool a_begin) { m_beginLoaded = a_begin; }
	inline bool IsBeginLoaded() { return m_beginLoaded; }
	inline Shader * GetShader() { return m_shader; }
	inline void SetShader(Shader * a_shader) { m_shader = a_shader; }
	
	//\brief Write all objects in the scene out to a scene file
	void Serialise();
	 
private:

	//\brief Load the scene's internal state from the config file
	bool InitFromConfig();

	//\brief Draw will cause active objects in the scene to submit resources to the render manager
	//\return true if resources were submitted without issue
	bool Draw();

	static const int s_numObjects = 16000;							///< Each GameObject is about 500 bytes, should be less than 16M
	static const float s_updateFreq;								///< How often the scene should check it's config on disk for updates

	GameFile m_sourceFile;											///< Configuration of the scene
	PageAllocator<GameObject> m_objects;							///< Pointer to memory allocated for contiguous game objects
	char m_name[StringUtils::s_maxCharsPerName];					///< Scene name for serialization
	char m_filePath[StringUtils::s_maxCharsPerLine];				///< Path of the scene for reloading
	FileManager::Timestamp m_timeStamp;								///< When the scene file was last edited
	float m_updateTimer;											///< The amount of time elapsed since the last file datestamp check
	float m_updateFreq;												///< How often the file resource is checked for changes
	Light m_lights[Shader::s_maxLights];							///< The lights in the scene
	SceneState::Enum m_state;										///< What state the scene is in
	bool m_beginLoaded;												///< If the scene should be loaded and rendering on startup
	int m_numLights;												///< The number of lights in the scene
	Shader * m_shader;												///< Shader for the whole scene
};

