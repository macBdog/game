#ifndef _ENGINE_WORLD_MANAGER_H_
#define _ENGINE_WORLD_MANAGER_H_

#include <iostream>
#include <fstream>

#include "../core/LinkedList.h"

#include "GameObject.h"
#include "Log.h"
#include "ModelManager.h"
#include "ScriptManager.h"
#include "Singleton.h"
#include "StringUtils.h"

class Shader;

//\brief A scene is a subset of a world, containing objects that are fixed and created by the engine and script objects
class Scene
{
	// WorldManager has access to the scene's objects
	friend class WorldManager;	

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
		, m_state(eSceneState_Unloaded) 
		, m_beginLoaded(false)
		, m_shader(NULL)
		{ sprintf(m_name, "scene01"); }

	// Cleanup the of objects in the scene on destruction
	~Scene();

	//\brief Adding and removing objects from the scene
	void AddObject(GameObject * a_newObject);
	bool RemoveObject(unsigned int a_objectId);

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

	//\brief TODO Stubbed out for implementation
	GameObject * GetSceneObjects(Vector a_worldPos);
	GameObject * GetSceneObjects(Vector a_lineStart, Vector a_lineEnd);

	//\brief Update all the objects in the scene
	bool Update(float a_dt);

	//\brief Get the number of objects in the scene
	//\return uint of the number of objects
	inline unsigned int GetNumObjects() { return m_numObjects; }
	
	//\brief Resource mutators and accessors
	inline void SetName(const char * a_name) { sprintf(m_name, "%s", a_name); }
	inline void SetBeginLoaded(bool a_begin) { m_beginLoaded = a_begin; }
	inline bool IsBeginLoaded() { return m_beginLoaded; }
	inline Shader * GetShader() { return m_shader; }
	inline void SetShader(Shader * a_shader) { m_shader = a_shader; }
	
	//\brief Write all objects in the scene out to a scene file
	void Serialise();

protected:

	//\brief Accessor for the world to remove all objects
	inline LinkedListNode<GameObject> * GetHeadObject() { return m_objects.GetHead(); }

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
	bool m_beginLoaded;								///< If the scene should be loaded and rendering on startup
	Shader * m_shader;								///< Shader for the whole scene
};

//\brief WorldManager handles object and scene management.
//		 The current thought is that the world is made of scenes.
//		 And scenes are filled with objects.
class WorldManager : public Singleton<WorldManager>
{
public:

	//\brief Ctor calls through to startup
	WorldManager() 
		: m_totalSceneNumObjects(0)
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
	template <typename T>
	T * CreateObject(const char * a_templatePath = NULL, Scene * a_scene = NULL)
	{
		// Check there is a valid scene to add the object to
		Scene * sceneToAddObjectTo = NULL;
		if (a_scene == NULL)
		{
			sceneToAddObjectTo = m_currentScene;
		}
		else
		{
			sceneToAddObjectTo = a_scene;
		}

		// Early out if no scene
		if (sceneToAddObjectTo == NULL)
		{
			Log::Get().WriteEngineErrorNoParams("Cannot create an object, there is no scene to add it to!");
			return NULL;
		}

		// TODO Please allocate a heap for game objects

		// Template paths are either fully qualified or relative to the config template dir
		ModelManager & modelMan = ModelManager::Get();
		if (a_templatePath)
		{
			char fileNameBuf[StringUtils::s_maxCharsPerLine];
			if (!strstr(a_templatePath, ":\\"))
			{
				sprintf(fileNameBuf, "%s%s", m_templatePath, a_templatePath);

				// Add on file extension if not present
				if (!strstr(fileNameBuf, ".tmp"))
				{
					const char * fileExt = ".tmp\0";
					unsigned char lastChar = strlen(fileNameBuf);
					strncpy(&fileNameBuf[lastChar], fileExt, sizeof(char) * strlen(fileExt)+1);
				}
			} 
			else // Already fully qualified
			{
				sprintf(fileNameBuf, "%s", a_templatePath);
			}

			// Open the template file
			GameFile templateFile(fileNameBuf);
			if (templateFile.IsLoaded())
			{
				// Create from template properties
				// TODO memory management kill std::new
				if (T * newGameObject = new T())
				{
					newGameObject->SetId(m_totalSceneNumObjects++);
					newGameObject->SetState(GameObject::eGameObjectState_Loading);
					newGameObject->SetTemplate(a_templatePath);
					if (GameFile::Object * object = templateFile.FindObject("gameObject"))
					{	
						bool validObject = true;
						
						// Name
						if (GameFile::Property * name = object->FindProperty("name"))
						{
							newGameObject->SetName(name->GetString());
						}
						// Model file
						if (GameFile::Property * model = object->FindProperty("model"))
						{
							if (Model * newModel = modelMan.GetModel(model->GetString()))
							{
								newGameObject->SetModel(newModel);
							}
							else // Failure of model load will report errors
							{
								validObject = false;
							}
						}
						// Clipping type
						if (GameFile::Property * clipType = object->FindProperty("clipType"))
						{
							if (strstr(clipType->GetString(), "sphere") != NULL)
							{
								newGameObject->SetClipType(GameObject::eClipTypeSphere);
							}
							else if (strstr(clipType->GetString(), "axisbox") != NULL)
							{
								newGameObject->SetClipType(GameObject::eClipTypeAxisBox);
							}
						}
						// Clipping size
						if (GameFile::Property * clipSize = object->FindProperty("clipSize"))
						{
							newGameObject->SetClipSize(clipSize->GetVector());
						}
						// Shader 
						if (GameFile::Property * shader = object->FindProperty("shader"))
						{
							// First try to find if the shader is already loaded
							Shader * existingShader = RenderManager::Get().GetManagedShader(shader->GetString());
							if (existingShader != NULL)
							{
								newGameObject->SetShader(existingShader);
								RenderManager::Get().ManageShader(newGameObject);
							}
							// If not, create the shader
							else if (Shader * pNewShader = new Shader(shader->GetString()))
							{
								if (RenderManager::InitShaderFromFile(*pNewShader))
								{
									newGameObject->SetShader(pNewShader);
									RenderManager::Get().ManageShader(newGameObject);
								}
								else // Compile error will be reported in the log
								{
									delete pNewShader;
									newGameObject->SetShader(NULL);
								}
							}
						}

						// Add to currently active scene
						if (validObject)
						{
							sceneToAddObjectTo->AddObject(newGameObject);
							return newGameObject;
						}
						else
						{
							return NULL;
						}
					}
					else // Can't find the first object
					{
						Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Unable to find a root gameObject node for template file %s", a_templatePath);
						return NULL;
					}
				}
				else // Can't create the game object
				{
					Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Unable to allocate memory to create game object from template file %s", a_templatePath);
					return NULL;
				}
			}
			else // Load failed
			{
				Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Unable to load template file %s", a_templatePath);
				return NULL;
			}
		}
		else // Create default object
		{
			if (T * newGameObject = new T())
			{
				newGameObject->SetId(m_totalSceneNumObjects++);
				newGameObject->SetState(GameObject::eGameObjectState_Loading);
				newGameObject->SetName("NEW_GAME_OBJECT");
				newGameObject->SetPos(Vector(0.0f, 0.0f, -20.0f));
				
				// Add to currently active scene
				sceneToAddObjectTo->AddObject(newGameObject);

				return newGameObject;
			}
		}

		return NULL;
	}
	
	//\brief Remove a created object from the world
	//\param a_destroyScriptBindings true if the script management bindings should be killed
	//\return true if an object is destroyed
	bool DestroyObject(unsigned int a_objectId, bool a_destroyScriptBindings = true);

	//\brief Destroy all objects in the current scene
	//\param a_destroyScriptOwned bool to specify destruction of scripts owned objects
	//\return true if any objects were destroyed
	bool DestoryAllObjects(bool a_destroyScriptOwned = false);

	//\brief Destroy all objects owned by script in the current scene
	//\param a_destroyScriptBindings bool to specify destruction of bindings to scripts, usually unwanted as script is going bye bye
	//\return true if any objects were destroyed
	bool DestoryAllScriptsOwnedObjects(bool a_destroyScriptBindings = false);

	//\brief Get a pointer to an existing object in the world.
	//\param a_objectId the unique game id for this object
	//\return Pointer to a game object in the world
	GameObject * GetGameObject(unsigned int a_objectId);
	GameObject * GetGameObject(const char * a_objName);

	//\brief Get the scene that the world is currently showing
	//\return A pointer to a scene
	Scene * GetCurrentScene() { return m_currentScene; }

	//\brief Accessor for the relative paths
	inline const char * GetTemplatePath() { return m_templatePath; }
	inline const char * GetScenePath() { return m_scenePath; }

private:

	//\brief Read scene details from a file into a scene object
	//\param a_scenePath is a string containing the filename of a scene file
	//\param a_sceneToLoad_OUT is a pointer to a scene object to modify with data from the scene path
	bool LoadScene(const char * a_scenePath, Scene * a_sceneToLoad_OUT);

	//\brief Alias to refer to a group of objects
	typedef LinkedListNode<Scene> SceneNode;
	
	LinkedList<Scene> m_scenes;								///< All the currently loaded scenes are added to this list
	Scene * m_currentScene;									///< The currently active scene
	unsigned int m_totalSceneNumObjects;					///< Total object count across all scenes, drives ID creation
	char m_templatePath[StringUtils::s_maxCharsPerLine];	///< Path for templates
	char m_scenePath[StringUtils::s_maxCharsPerLine];		///< Path for scene files
};

#endif /* _ENGINE_WORLD_MANAGER_H_ */
