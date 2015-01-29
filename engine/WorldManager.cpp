#include <assert.h>

#include "CollisionUtils.h"
#include "DebugMenu.h"
#include "ModelManager.h"
#include "RenderManager.h"

#include "WorldManager.h"

template<> WorldManager * Singleton<WorldManager>::s_instance = NULL;

const float Scene::s_updateFreq = 1.0f;								///< How often the scene should check it's config on disk for updates

Scene::Scene() 
: m_numObjects(0)
, m_state(SceneState::Unloaded) 
, m_beginLoaded(false)
, m_shader(NULL)
, m_numLights(0)
, m_updateTimer(0.0f)
, m_updateFreq(s_updateFreq)
{ 
	m_name[0] = '\0';
	m_filePath[0] = '\0';

	// Allocate memory for the scene's object pool
	m_objects.Init(s_numObjects, sizeof(GameObject));
	sprintf(m_name, "defaultScene");
}

Scene::~Scene()
{
	// Shutdown all game objects in the scene
	for (unsigned int i = 0; i < m_numObjects; ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			gameObj->Shutdown();
		}
	}
	
	// Clean up shader if set
	if (m_shader != NULL)
	{
		RenderManager::Get().UnManageShader(this);
	}
}

bool Scene::InitFromConfig()
{
	// Create a new widget and copy properties from file
	if (GameFile::Object * sceneObject = m_sourceFile.FindObject("scene"))
	{
		// Set various properties of a scene
		bool propsOk = true;
		if (GameFile::Object * sceneObj = m_sourceFile.FindObject("scene"))
		{
			GameFile::Property * nameProp = sceneObj->FindProperty("name");
			GameFile::Property * beginLoadedProp = sceneObj->FindProperty("beginLoaded");
			if (nameProp && beginLoadedProp)
			{
				SetName(nameProp->GetString());
				SetBeginLoaded(beginLoadedProp->GetBool());

				// Set whole scene shader if specified
				if (GameFile::Property * shaderProp = sceneObj->FindProperty("shader"))
				{
					RenderManager::Get().ManageShader(this, shaderProp->GetString());
				}
			}
			// Support for up to four lights per scene
			GameFile::Object * lightingObj = sceneObj->FindObject("lighting");
			if (lightingObj)
			{
				int numLights = 0;
				LinkedListNode<GameFile::Object> * nextLight = lightingObj->GetChildren();
				while (nextLight != NULL)
				{
					// Check there aren't more lights than we support
					if (numLights >= Shader::s_maxLights)
					{
						Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Scene %s declares more than the maximum of %d lights.", GetName(), Shader::s_maxLights);
						propsOk = false;
						break;
					}
					GameFile::Object * light = nextLight->GetData();
					GameFile::Property * lName = light->FindProperty("name");
					GameFile::Property * lPos = light->FindProperty("pos");
					GameFile::Property * lDir = light->FindProperty("dir");
					GameFile::Property * lAmbient = light->FindProperty("ambient");
					GameFile::Property * lDiffuse = light->FindProperty("diffuse");
					GameFile::Property * lSpecular = light->FindProperty("specular");
					if (lName && lPos && lDir && lAmbient && lDiffuse && lSpecular)
					{
						AddLight(lName->GetString(), 
							lPos->GetVector(),
							lDir->GetQuaternion(),
							lAmbient->GetColour(),
							lDiffuse->GetColour(),
							lSpecular->GetColour());
						numLights++;
					}
					else
					{
						Log::Get().Write(LogLevel::Error, LogCategory::Game, "Scene %s declares a light without the required required name, pos, dir, ambient, diffuse and specular properties.", GetName());
						propsOk = false;
					}
					nextLight = nextLight->GetNext();
				}
			}
		}
		else
		{
			propsOk = false;
		}

		// Load child game objects of the scene
		LinkedListNode<GameFile::Object> * childGameObject = sceneObject->GetChildren();
		while (childGameObject != NULL)
		{
			// Children of the scene file can be lighting or game objects, make sure we only create game objects
			if (childGameObject->GetData()->m_name.GetHash() != StringHash::GenerateCRC("gameObject"))
			{
				childGameObject = childGameObject->GetNext();
				continue;
			}

			const char * templateName = NULL;
			GameFile::Object * childObj = childGameObject->GetData();
			if (GameFile::Property * prop = childObj->FindProperty("template"))
			{
				templateName = prop->GetString();
			}
				
			// Create object with optional template values and add to the scene
			GameObject * newObject = WorldManager::Get().CreateObject(templateName, this);

			// Override any templated values
			if (templateName != NULL)
			{
				newObject->SetTemplate(templateName);
			}

			if (childObj->FindProperty("name"))
			{
				newObject->SetName(childObj->FindProperty("name")->GetString());
			}
			if (childObj->FindProperty("pos"))
			{
				newObject->SetPos(childObj->FindProperty("pos")->GetVector());
			}
			if (childObj->FindProperty("rot"))
			{
				newObject->SetRot(childObj->FindProperty("rot")->GetQuaternion());
			}
			if (GameFile::Property * clipType = childObj->FindProperty("clipType"))
			{
				if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[ClipType::Sphere]) != NULL)
				{
					newObject->SetClipType(ClipType::Sphere);
				}
				else if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[ClipType::Box]) != NULL)
				{
					newObject->SetClipType(ClipType::Box);
				}
				else if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[ClipType::Mesh]) != NULL)
				{
					newObject->SetClipType(ClipType::Mesh);
					newObject->SetPhysicsMesh(clipType->GetString());
				}
				else
				{
					Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Invalid clip type of %s specified for object %s in scene %s, defaulting to box.", clipType->GetString(), newObject->GetName(), GetName());
				}
			}
			if (GameFile::Property * clipSize = childObj->FindProperty("clipSize"))
			{
				newObject->SetClipSize(clipSize->GetVector());
			}
			if (GameFile::Property * clipOffset = childObj->FindProperty("clipOffset"))
			{
				newObject->SetClipOffset(clipOffset->GetVector());
			}
			if (childObj->FindProperty("model"))
			{
				newObject->SetModel(ModelManager::Get().GetModel(childObj->FindProperty("model")->GetString()));
			}
			if (childObj->FindProperty("shader"))
			{
				RenderManager::Get().ManageShader(newObject, childObj->FindProperty("shader")->GetString());
			}
			else if (HasLights())
			{
				// Set default shader
				newObject->SetShader(RenderManager::Get().GetLightingShader());		
			}
				
			childGameObject = childGameObject->GetNext();
		}

		// No properties present
		if (!propsOk) 
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Error loading scene %s, scene does not have required properties.", GetName());
			return false;
		}
	}
	else // Unexpected file format, no root element
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Error loading scene %s, no valid scene parent element.", GetName());
		return false;
	}
	
	return true;
}

GameObject * Scene::AddObject(unsigned int a_objectId)
{
	// Scene objects are stored contiguously in object ID order
	GameObject * newGameObject = m_objects.Add(m_numObjects);
	m_numObjects++;
	assert(newGameObject != NULL);
	return newGameObject;
}

bool Scene::RemoveObject(unsigned int a_objectId)
{
	GameObject * gameObj = m_objects.Get(a_objectId);
	assert(gameObj != NULL);
	gameObj->Shutdown();
	
	// TODO! return m_objects.Remove(a_objectId);
	return false;
}

bool Scene::AddLight(const char * a_name, const Vector & a_pos, const Quaternion & a_dir, const Colour & a_ambient, const Colour & a_diffuse, const Colour & a_specular)
{
	if (m_numLights < Shader::s_maxLights)
	{
		sprintf(m_lights[m_numLights].m_name, "%s", a_name);
		m_lights[m_numLights].m_enabled = true;
		m_lights[m_numLights].m_pos = a_pos;
		m_lights[m_numLights].m_dir = a_dir;
		m_lights[m_numLights].m_ambient = a_ambient;
		m_lights[m_numLights].m_diffuse = a_diffuse;
		m_lights[m_numLights].m_specular = a_specular;
		m_numLights++;
		return true;
	}
	else
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not add light, the max lights is %d!", Shader::s_maxLights);
	}
	return false;
}

bool Scene::RemoveLight(const char * a_name)
{
	for (unsigned int i = 0; i < Shader::s_maxLights; ++i)
	{
		
		if (strcmp(m_lights[i].m_name, a_name) == 0)
		{
			// If the light we are looking for is on the end of the light array
			if (m_numLights == i)
			{
				memset(&m_lights[i], 0, sizeof(Light));
			}
			else // Swap the end light with this one
			{
				memcpy(&m_lights[i], &m_lights[m_numLights-1], sizeof(Light));
			}
			--m_numLights;
			return true;
		}
	}
	return false;
}

GameObject * Scene::GetSceneObject(unsigned int a_storageId)
{
	if (a_storageId < m_numObjects)
	{
		return m_objects.Get(a_storageId);
	}
	return NULL;
}

GameObject * Scene::GetSceneObject(const char * a_objName)
{
	// Iterate through all objects in the scene
	for (unsigned int i = 0; i < m_numObjects; ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			if (strstr(gameObj->GetName(),a_objName) != 0)
			{
				return gameObj;
			}
		}
	}

	return NULL;
}

GameObject * Scene::GetSceneObject(Vector a_worldPos)
{
	// Iterate through all objects in the scene
	for (unsigned int i = 0; i < m_numObjects; ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			// To the first object that intersects with a point
			if (gameObj->CollidesWith(a_worldPos))
			{
				return gameObj;
			}
		}
	}
	return NULL;
}

GameObject * Scene::GetSceneObject(Vector a_lineStart, Vector a_lineEnd)
{
	// Iterate through all objects in the scene
	for (unsigned int i = 0; i < m_numObjects; ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			// To the first object that intersects with a point
			if (gameObj->CollidesWith(a_lineStart, a_lineEnd))
			{
				return gameObj;
			}
		}
	}
	return NULL;
}

Light * Scene::GetLightAtPos(Vector a_worldPos)
{
	// Iterate through all lights in the scene
	for (int i = 0; i < m_numLights; ++i)
	{
		Light * curLight = &m_lights[i];
		if (fabs((a_worldPos - curLight->m_pos).Length()) < Light::s_lightDrawSize*10.0f)
		{
			return curLight;
		}
	}
	return NULL;
}

Light * Scene::GetLight(Vector a_lineStart, Vector a_lineEnd)
{
	// Iterate through all lights in the scene
	for (int i = 0; i < m_numLights; ++i)
	{
		Light * curLight = &m_lights[i];
		if (CollisionUtils::IntersectLineSphere(a_lineStart, a_lineEnd, curLight->m_pos, Light::s_lightDrawSize*10.0f))
		{
			return curLight;
		}
	}
	return NULL;
}

void Scene::RemoveAllObjects(bool a_destroyScriptOwned)
{
	for (unsigned int i = 0; i < m_numObjects; ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			// Test if script owned before destruction
			bool destroyObject = !a_destroyScriptOwned && gameObj->IsScriptOwned() ? false : true;
			if (destroyObject)
			{
				gameObj->Shutdown();
			}
		}
	}

	m_numLights = 0;
	m_numObjects = 0;
	m_objects.Reset();
}

void Scene::RemoveAllScriptOwnedObjects(bool a_destroyScriptBindings)
{
	// Remove all objects including scene and script owned objects
	for (unsigned int i = 0; i < m_numObjects; ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			gameObj->Shutdown();
		}
	}

	m_numLights = 0;
	m_numObjects = 0;
	m_objects.Reset();

	// Load scene front scratch so we are back with just scene objects and no script objects
	InitFromConfig();
}

bool Scene::Update(float a_dt)
{
	// Iterate through all objects in the scene and update state
	bool updateSuccess = true;
	for (unsigned int i = 0; i < m_numObjects; ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			updateSuccess &= gameObj->Update(a_dt);
		}
	}

	// Now state and position have been updated, submit resources to be rendered
	bool drawSuccess = Draw();

#ifndef _RELEASE
	// Check if the scene needs to be reloaded
	if (m_updateTimer < m_updateFreq)
	{
		m_updateTimer += a_dt;
	}
	else
	{
		m_updateTimer = 0.0f;
		FileManager::Timestamp curTimeStamp;
		FileManager::Get().GetFileTimeStamp(m_filePath, curTimeStamp);
		if (curTimeStamp > m_timeStamp)
		{
			ScriptManager::Get().ReloadScripts();
			ResetFileDateStamp();
		}
	}
#endif

	return updateSuccess && drawSuccess;
}

void Scene::Serialise()
{
	// Construct the path from the scene directory and name of the scene
	char scenePath[StringUtils::s_maxCharsPerLine];
	sprintf(scenePath, "%s%s.scn", WorldManager::Get().GetScenePath(), m_name);

	// Create an output stream
	GameFile * sceneFile = new GameFile();

	GameFile::Object * sceneObject = sceneFile->AddObject("scene");
	sceneFile->AddProperty(sceneObject, "name", m_name);
	sceneFile->AddProperty(sceneObject, "beginLoaded", StringUtils::BoolToString(m_beginLoaded));

	// Optional properties of the scene
	if (m_shader != NULL) 
	{
		sceneFile->AddProperty(sceneObject, "shader", m_shader->GetName());
	}

	// Add lighting section
	if (HasLights())
	{
		GameFile::Object * lightsObject = sceneFile->AddObject("lighting", sceneObject);
		for (int i = 0; i < m_numLights; ++i)
		{
			char vecBuf[StringUtils::s_maxCharsPerName];
			GameFile::Object * curLightObj = sceneFile->AddObject("light", lightsObject);
			sceneFile->AddProperty(curLightObj, "name", m_lights[i].m_name);
			m_lights[i].m_pos.GetString(vecBuf);		sceneFile->AddProperty(curLightObj, "pos", vecBuf);
			m_lights[i].m_dir.GetString(vecBuf);		sceneFile->AddProperty(curLightObj, "dir", vecBuf);
			m_lights[i].m_ambient.GetString(vecBuf);	sceneFile->AddProperty(curLightObj, "ambient", vecBuf);
			m_lights[i].m_diffuse.GetString(vecBuf);	sceneFile->AddProperty(curLightObj, "diffuse", vecBuf);
			m_lights[i].m_specular.GetString(vecBuf);	sceneFile->AddProperty(curLightObj, "specular", vecBuf);
		}
	}
	
	// Add each object in the scene
	for (unsigned int i = 0; i < m_numObjects; ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			// Do not save out objects created by script
			if (!gameObj->IsScriptOwned())
			{
				// Add the object to the game file
				gameObj->Serialise(sceneFile, sceneObject);
			}
		}
	}
	
	// Write all the game file data to a file
	sceneFile->Write(scenePath);
	delete sceneFile;
}

bool Scene::Draw()
{
	// Iterate through all objects in the scene and update state
	bool drawSuccess = true;
	for (unsigned int i = 0; i < m_numObjects; ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			drawSuccess &= gameObj->Draw();
		}
	}

	// Draw all the lights in the scene when debug menu is up
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		for (int i = 0; i < m_numLights; ++i)
		{
			const Colour drawColour = m_lights[i].m_enabled ? sc_colourYellow : sc_colourGrey;
			RenderManager::Get().AddDebugSphere(m_lights[i].m_pos, Light::s_lightDrawSize, drawColour);
			RenderManager::Get().AddDebugLine(m_lights[i].m_pos, m_lights[i].m_pos + m_lights[i].m_dir.GetXYZ(), drawColour);
			FontManager::Get().DrawDebugString3D(m_lights[i].m_name, m_lights[i].m_pos, drawColour);
		}
	}

	return drawSuccess;
}

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
	
	if (a_dataPack != NULL && a_dataPack->IsLoaded())
	{
		// Populate a list of scene files
		DataPack::EntryList sceneEntries;
		a_dataPack->GetAllEntries(".scn", sceneEntries);
		DataPack::EntryNode * curNode = sceneEntries.GetHead();

		// Load each scene in the data pack
		bool loadSuccess = true;
		while (curNode != NULL)
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
							if (m_currentScene == NULL)
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
		while(curNode != NULL)
		{
			char fullPath[StringUtils::s_maxCharsPerLine];
			sprintf(fullPath, "%s%s", a_scenePath, curNode->GetData()->m_name);
		
			// Add to the loaded scenes if begin loaded
			if (Scene * newScene = new Scene())
			{
				if (newScene->Load(fullPath))
				{
					// Set file up so it can reload itself
					newScene->SetFilePath(a_scenePath);
					newScene->ResetFileDateStamp();

					// Insert into list
					SceneNode * newSceneNode = new SceneNode();
					newSceneNode->SetData(newScene);
					m_scenes.Insert(newSceneNode);

					if (newScene->IsBeginLoaded())
					{
						// Set current scene to the first loaded scene
						if (m_currentScene == NULL)
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
	while(next != NULL)
	{
		// Cache off next pointer
		SceneNode * cur = next;
		next = cur->GetNext();

		m_scenes.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	// Clear the current scene as it's data has been cleared
	m_currentScene = NULL;

	return true;
}

bool WorldManager::Update(float a_dt)
{
	// Only update the active scene
	if (m_currentScene != NULL)
	{
		return m_currentScene->Update(a_dt);
	}
	return false;
}

GameObject * WorldManager::CreateObject(const char * a_templatePath, Scene * a_scene)
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

	// Add a new game object to the scene
	GameObject * newGameObject = sceneToAddObjectTo->AddObject(m_totalGameObjects);
	ObjectLookup * lookup = m_objectLookup.Add(m_totalGameObjects);
	lookup->m_scene = sceneToAddObjectTo;
	lookup->m_storageId = sceneToAddObjectTo->GetNumObjects() - 1;
	newGameObject->SetId(m_totalGameObjects++);

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
					if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[ClipType::Sphere]) != NULL)
					{
						hasCollision = true;
						newGameObject->SetClipType(ClipType::Sphere);
					}
					else if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[ClipType::Box]) != NULL)
					{
						hasCollision = true;
						newGameObject->SetClipType(ClipType::Box);
					}
					else if (strstr(clipType->GetString(), GameObject::s_clipTypeStrings[ClipType::Mesh]) != NULL)
					{
						hasCollision = true;
						newGameObject->SetClipType(ClipType::Mesh);
						newGameObject->SetPhysicsMesh(clipType->GetString());
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

					// Add object to collision world
					if (newGameObject->GetClipType() == ClipType::Mesh || newGameObject->GetClipSize().LengthSquared() > 0.0f)
					{
						pMan.AddCollisionObject(newGameObject);
					}
				}

				// Optionally add to physics world
				if (GameFile::Property * physics = object->FindProperty("physics"))
				{
					if (physics->GetBool())
					{
						pMan.AddPhysicsObject(newGameObject);
					}
				}

				// All loading operations have completed
				newGameObject->SetState(GameObjectState::Active);
			}
			else // Can't find the first object
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to find a root gameObject node for template file %s", a_templatePath);
				return NULL;
			}
		}
		else // Load failed
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Unable to load template file %s", a_templatePath);
			return NULL;
		}
	}
	else // Set properties for default object
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

		// Remove from scene
		obj->Shutdown();
		return m_currentScene->RemoveObject(a_objectId);
	}
	
	return false; 
}

void WorldManager::DestroyAllObjects(bool a_destroyScriptOwned)
{
	// Iterate through all scenes and destroy objects
	m_totalGameObjects = 0;
	SceneNode * next = m_scenes.GetHead();
	while(next != NULL)
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
	while(next != NULL)
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
	return NULL;
}

Scene * WorldManager::GetScene(const char * a_sceneName)
{
	SceneNode * next = m_scenes.GetHead();
	while(next != NULL)
	{
		Scene * curScene = next->GetData();
		if (strcmp(a_sceneName, curScene->GetName()) == 0)
		{
			return curScene;
		}
		next = next->GetNext();
	}

	return NULL;
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
	Scene * newScene = new Scene();
	assert(newScene != NULL);
	newScene->SetName(a_sceneName);

	// Insert into list of scenes
	SceneNode * newSceneNode = new SceneNode();
	newSceneNode->SetData(newScene);
	m_scenes.Insert(newSceneNode);

	// Set current scene to the first loaded scene
	SetCurrentScene(newScene);
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
		while(next != NULL)
		{
			if (GameObject * foundObject = next->GetData()->GetSceneObject(a_objName))
			{
				return foundObject;
			}
			next = next->GetNext();
		}
	}

	// Failure case
	return NULL;
}
