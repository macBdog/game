#include "CollisionUtils.h"
#include "DebugMenu.h"
#include "FontManager.h"
#include "ModelManager.h"
#include "PhysicsManager.h"
#include "RenderManager.h"
#include "ScriptManager.h"

#include "WorldManager.h"

template<> WorldManager * Singleton<WorldManager>::s_instance = nullptr;

bool WorldManager::Startup(const char * a_templatePath, const char * a_scenePath, const DataPack * a_dataPack)
{
	// Cache off the template path for non qualified loading of game object
	memset(&m_templatePath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_templatePath, a_templatePath, sizeof(char) * strlen(a_templatePath) + 1);

	// Allocate memory for the global world lookup directory
	m_objectLookup.Init(s_numLookup, sizeof(ObjectLookup));

	// Generate list to iterate through all scenes in the scenepath and load them
	memset(&m_scenePath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_scenePath, a_scenePath, sizeof(char) * strlen(a_scenePath) + 1);
	
	if (a_dataPack != nullptr && a_dataPack->IsLoaded())
	{
		// Populate a list of scene files
		DataPack::EntryList sceneEntries;
		a_dataPack->GetAllEntries(".scn", sceneEntries);
		DataPack::EntryNode * curNode = sceneEntries.GetHead();

		// Load each scene in the data pack
		bool loadSuccess = true;
		while (curNode != nullptr)
		{
			GameFile sceneFile;
			if (sceneFile.Load(curNode->GetData()))
			{
				if (Scene * newScene = new Scene())
				{
					if (newScene->Load(curNode->GetData()))
					{
						// Insert into list
						SceneNode * newSceneNode = new SceneNode();
						newSceneNode->SetData(newScene);
						m_scenes.Insert(newSceneNode);

						if (newScene->IsBeginLoaded())
						{
							// Set current scene to the first loaded scene
							if (m_currentScene == nullptr)
							{
								m_currentScene = newScene;
							}
						}
					}
					else
					{
						// Load failed, abort this one
						delete newScene;
					}
				}
			}
			curNode = curNode->GetNext();
		}

		// Clean up the list of scenes
		a_dataPack->CleanupEntryList(sceneEntries);
	}
	else
	{
		// Populate a list of scenes on the disk
		FileManager::FileList sceneFiles;
		FileManager::Get().FillFileList(m_scenePath, sceneFiles, ".scn");

		// Load each scene in the directory
		FileManager::FileListNode * curNode = sceneFiles.GetHead();
		while(curNode != nullptr)
		{
			char fullPath[StringUtils::s_maxCharsPerLine];
			sprintf(fullPath, "%s%s", a_scenePath, curNode->GetData()->m_name);
		
			// Add to the loaded scenes if begin loaded
			if (Scene * newScene = new Scene())
			{
				if (newScene->Load(fullPath))
				{
					// Set file up so it can reload itself
					newScene->SetFilePath(fullPath);
					newScene->ResetFileDateStamp();

					// Insert into list
					SceneNode * newSceneNode = new SceneNode();
					newSceneNode->SetData(newScene);
					m_scenes.Insert(newSceneNode);

					if (newScene->IsBeginLoaded())
					{
						// Set current scene to the first loaded scene
						if (m_currentScene == nullptr)
						{
							m_currentScene = newScene;
						}
					}
				}
				else
				{
					// Clean up the scene, the load will report the error
					delete newScene;
				}
			}
			curNode = curNode->GetNext();
		}

		// Clean up the list of scenes
		FileManager::Get().CleanupFileList(sceneFiles);
	}

	// If no scenes, setup the default scene
	if (m_scenes.GetLength() == 0)
	{
		Log::Get().WriteEngineErrorNoParams("No scene files or no scenes set to start on load, creating a default scene.");
		Scene * newScene = new Scene();
		newScene->SetBeginLoaded(true);
		newScene->SetName("defaultScene");
		SceneNode * newSceneNode = new SceneNode();
		newSceneNode ->SetData(newScene);
		m_scenes.Insert(newSceneNode);
		m_currentScene = newScene;
		newScene->Serialise();
	}

	return true;
}

bool WorldManager::Shutdown()
{
	// Cleanup memory allocated for scene objects
	SceneNode * next = m_scenes.GetHead();
	while(next != nullptr)
	{
		// Cache off next pointer
		SceneNode * cur = next;
		next = cur->GetNext();

		m_scenes.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	// Clear the current scene as it's data has been cleared
	m_currentScene = nullptr;

	return true;
}

bool WorldManager::Update(float a_dt)
{
	// Only update the active scene
	if (m_currentScene != nullptr)
	{
		return m_currentScene->Update(a_dt);
	}
	return false;
}

GameObject * WorldManager::CreateObject(const char * a_templatePath, Scene * a_scene)
{
	// Check there is a valid scene to add the object to
	Scene * sceneToAddObjectTo = nullptr;
	if (a_scene == nullptr)
	{
		sceneToAddObjectTo = m_currentScene;
	}
	else
	{
		sceneToAddObjectTo = a_scene;
	}

	// Early out if no scene
	if (sceneToAddObjectTo == nullptr)
	{
		Log::Get().WriteEngineErrorNoParams("Cannot create an object, there is no scene to add it to!");
		return nullptr;
	}

	// Add a new game object to the scene
	GameObject * newGameObject = sceneToAddObjectTo->AddObject();
	ObjectLookup * lookup = m_objectLookup.Add(m_totalGameObjects);
	lookup->m_scene = sceneToAddObjectTo;
	lookup->m_storageId = sceneToAddObjectTo->GetNumObjects() - 1;
	newGameObject->SetId(m_totalGameObjects++);

	// Template paths are either fully qualified or relative to the config template dir
	ModelManager & modelMan = ModelManager::Get();
	if (a_templatePath)
	{
		GameFile templateFile;
		DataPack & dataPack = DataPack::Get();
		if (dataPack.IsLoaded())
		{
			char templatePath[StringUtils::s_maxCharsPerLine];
			sprintf(templatePath, "%s%s", m_templatePath, a_templatePath);
			if (DataPackEntry * templateEntry = dataPack.GetEntry(templatePath))
			{
				if (!templateFile.Load(templateEntry))
				{
					Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load template file %s from datapack", a_templatePath);
					return nullptr;
				}
			}
		}
		else
		{
			char fileNameBuf[StringUtils::s_maxCharsPerLine];
			if (!strstr(a_templatePath, ":\\"))
			{
				sprintf(fileNameBuf, "%s%s", m_templatePath, a_templatePath);

				// Add on file extension if not present
				if (!strstr(fileNameBuf, ".tmp"))
				{
					const char * fileExt = ".tmp\0";
					int lastChar = (int)strlen(fileNameBuf);
					strncpy(&fileNameBuf[lastChar], fileExt, sizeof(char) * strlen(fileExt) + 1);
				}
			}
			else // Already fully qualified
			{
				sprintf(fileNameBuf, "%s", a_templatePath);
			}

			// Open the template file
			if (!templateFile.Load(fileNameBuf))
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load template file %s", a_templatePath);
				return nullptr;
			}
		}

		// Process the properties of the template file
		if (templateFile.IsLoaded())
		{
			// Create from template properties
			newGameObject->SetState(GameObjectState::Loading);
			newGameObject->SetTemplate(a_templatePath);
			if (GameFile::Object * object = templateFile.FindObject("gameObject"))
			{	
				bool validObject = true;
						
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
				bool hasCollision = false;
				if (GameFile::Property * clipType = object->FindProperty("clipType"))
				{
					if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[static_cast<int>(ClipType::Sphere)]) != nullptr)
					{
						hasCollision = true;
						newGameObject->SetClipType(ClipType::Sphere);
					}
					else if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[static_cast<int>(ClipType::Box)]) != nullptr)
					{
						hasCollision = true;
						newGameObject->SetClipType(ClipType::Box);
					}
					else if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[static_cast<int>(ClipType::Mesh)]) != nullptr)
					{
						hasCollision = true;
						newGameObject->SetClipType(ClipType::Mesh);
					}
					else
					{
						Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Invalid clip type of %s specified for template %s, defaulting to box.", clipType->GetString(), a_templatePath);
					}
				}
				// Clipping size
				if (GameFile::Property * clipSize = object->FindProperty("clipSize"))
				{
					newGameObject->SetClipSize(clipSize->GetVector());
				}
				// Clipping offset
				if (GameFile::Property * clipOffset = object->FindProperty("clipOffset"))
				{
					newGameObject->SetClipOffset(clipOffset->GetVector());
				}
				// Shader 
				RenderManager & rMan = RenderManager::Get();
				if (GameFile::Property * shader = object->FindProperty("shader"))
				{
					// First try to find if the shader is already loaded
					rMan.ManageShader(newGameObject, shader->GetString());
				}
				else if (sceneToAddObjectTo->HasLights())
				{
					// Otherwise use lighting if the scene has been specified with lights
					newGameObject->SetShader(rMan.GetLightingShader());
				}

				// Add collision
				PhysicsManager & pMan = PhysicsManager::Get();
				if (hasCollision && newGameObject->GetClipType() > ClipType::None)
				{
					// Set the collision group up first
					if (GameFile::Property * clipGroup = object->FindProperty("clipGroup"))
					{
						if (pMan.GetCollisionGroupId(clipGroup->GetString()) >= 0)
						{
							newGameObject->SetClipGroup(clipGroup->GetString());
						}
						else
						{
							Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Unrecognised clip group of %s specified for template %s, defaulting to ALL.", clipGroup->GetString(), a_templatePath);
						}
					}
				}

				// Support physics properties
				if (GameFile::Property * massProp = object->FindProperty("physicsMass"))
				{
					newGameObject->SetPhysicsMass(massProp->GetFloat());
				}
				if (GameFile::Property * elasticityProp = object->FindProperty("physicsElasticity"))
				{
					newGameObject->SetPhysicsElasticity(elasticityProp->GetFloat());
				}
				if (GameFile::Property * linearDragProp = object->FindProperty("physicsLinearDrag"))
				{
					newGameObject->SetPhysicsLinearDrag(linearDragProp->GetFloat());
				}
				if (GameFile::Property * angularDragProp = object->FindProperty("physicsAngularDrag"))
				{
					newGameObject->SetPhysicsAngularDrag(angularDragProp->GetFloat());
				}

				// All loading operations have completed
				newGameObject->SetState(GameObjectState::Active);
			}
			else // Can't find the first object
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to find a root gameObject node for template file %s", a_templatePath);
				return nullptr;
			}
		}
	}
	else // Set properties for a default object
	{
		newGameObject->SetState(GameObjectState::Active);
		newGameObject->SetName("NEW_GAME_OBJECT");
		newGameObject->SetPos(Vector(0.0f, 0.0f, 0.0f));
	}

	return newGameObject;
}

bool WorldManager::DestroyObject(unsigned int a_objectId, bool a_destroyScriptBindings) 
{ 
	if (GameObject * obj = GetGameObject(a_objectId))
	{
		// Make sure the script reference is cleaned up as well, if flag is set
		if (obj->IsScriptOwned() && a_destroyScriptBindings)
		{
			ScriptManager::Get().DestroyObjectScriptBindings(obj);
		}

		// Remove itself from any scenes
		return obj->Shutdown();
	}
	
	return false; 
}

void WorldManager::DestroyAllObjects(bool a_destroyScriptOwned)
{
	// Iterate through all scenes and destroy objects
	m_totalGameObjects = 0;
	SceneNode * next = m_scenes.GetHead();
	while(next != nullptr)
	{
		next->GetData()->RemoveAllObjects(a_destroyScriptOwned);
		next = next->GetNext();
	}
}

void WorldManager::DestroyAllScriptOwnedObjects(bool a_destroyScriptBindings)
{
	// Iterate through all scenes and destroy objects
	m_totalGameObjects = 0;
	SceneNode * next = m_scenes.GetHead();
	while(next != nullptr)
	{
		next->GetData()->RemoveAllScriptOwnedObjects(a_destroyScriptBindings);
		next = next->GetNext();
	}
}

GameObject * WorldManager::GetGameObject(unsigned int a_objectId)
{
	// Use the lookup to reference the scene and object directly
	if (ObjectLookup * lookup = m_objectLookup.Get(a_objectId))
	{
		return lookup->m_scene->GetSceneObject(lookup->m_storageId);
	}

	// Failure case
	return nullptr;
}

Scene * WorldManager::GetScene(const char * a_sceneName)
{
	SceneNode * next = m_scenes.GetHead();
	while(next != nullptr)
	{
		Scene * curScene = next->GetData();
		if (strcmp(a_sceneName, curScene->GetName()) == 0)
		{
			return curScene;
		}
		next = next->GetNext();
	}

	return nullptr;
}

void WorldManager::SetCurrentScene(const char * a_sceneName)
{
	if (Scene * existingScene = GetScene(a_sceneName))
	{
		SetCurrentScene(existingScene);
	}
}

void WorldManager::SetNewScene(const char * a_sceneName)
{
	// Create new scene and set name
	if (Scene * newScene = new Scene())
	{
		newScene->SetName(a_sceneName);

		// Insert into list of scenes
		SceneNode * newSceneNode = new SceneNode();
		newSceneNode->SetData(newScene);
		m_scenes.Insert(newSceneNode);

		// Set current scene to the first loaded scene
		SetCurrentScene(newScene);
	}
}

GameObject * WorldManager::GetGameObject(const char * a_objName)
{
	// First try to find the object in the current scene
	if (GameObject * foundObject = m_currentScene->GetSceneObject(a_objName))
	{
		return foundObject;
	}
	else // Look through each scene for the target object
	{
		SceneNode * next = m_scenes.GetHead();
		while(next != nullptr)
		{
			if (GameObject * foundObject = next->GetData()->GetSceneObject(a_objName))
			{
				return foundObject;
			}
			next = next->GetNext();
		}
	}

	// Failure case
	return nullptr;
}
