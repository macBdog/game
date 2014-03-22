#include "DebugMenu.h"
#include "GameFile.h"
#include "ModelManager.h"
#include "RenderManager.h"

#include "WorldManager.h"

template<> WorldManager * Singleton<WorldManager>::s_instance = NULL;

Scene::~Scene()
{
	// Clean up memory allocated for scene list
	SceneObject * next = m_objects.GetHead();
	while(next != NULL)
	{
		// Cache off next pointer
		SceneObject * cur = next;
		next = cur->GetNext();

		m_objects.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	// Clean up shader if set
	if (m_shader != NULL)
	{
		delete m_shader;
	}
}

void Scene::AddObject(GameObject * a_newObject)
{
	// TODO Scene objects should also have a heap, implement memory management
	if (SceneObject * newSceneObject = new SceneObject())
	{
		newSceneObject->SetData(a_newObject);
		a_newObject->Startup();
		a_newObject->SetState(GameObjectState::Active);
		m_objects.Insert(newSceneObject);

		++m_numObjects;
	}
}

bool Scene::RemoveObject(unsigned int a_objectId)
{
	SceneObject * curObject = m_objects.GetHead();
	while (curObject != NULL)
	{
		// Delete a target with a specific id
		GameObject * gameObject = curObject->GetData();
		if (gameObject->GetId() == a_objectId)
		{
			m_objects.Remove(curObject);
			delete gameObject;
			delete curObject;
			return true;
		}
	
		curObject = curObject->GetNext();
	}

	return false;
}

bool Scene::AddLight(const char * a_name, const Vector & a_pos, const Vector & a_dir, float a_ambient, float a_diffuse, float a_specular)
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
	return false;
}

GameObject * Scene::GetSceneObject(unsigned int a_objectId)
{
	// Iterate through all objects in the scene
	SceneObject * curObject = m_objects.GetHead();
	while (curObject != NULL)
	{
		// To find a target with a specific id
		GameObject * gameObject = curObject->GetData();
		if (gameObject->GetId() == a_objectId)
		{
			return gameObject;
		}
	
		curObject = curObject->GetNext();
	}

	return NULL;
}

GameObject * Scene::GetSceneObject(const char * a_objName)
{
	// Iterate through all objects in the scene
	SceneObject * curObject = m_objects.GetHead();
	while (curObject != NULL)
	{
		// To find a target with a specific id
		GameObject * gameObject = curObject->GetData();
		if (strstr(gameObject->GetName(),a_objName) != 0)
		{
			return gameObject;
		}
	
		curObject = curObject->GetNext();
	}

	return NULL;
}

GameObject * Scene::GetSceneObject(Vector a_worldPos)
{
	// Iterate through all objects in the scene
	SceneObject * curObject = m_objects.GetHead();
	while (curObject != NULL)
	{
		// To the first object that intersects with a point
		GameObject * gameObject = curObject->GetData();
		if (gameObject->CollidesWith(a_worldPos))
		{
			return gameObject;
		}
	
		curObject = curObject->GetNext();
	}

	return NULL;
}

GameObject * Scene::GetSceneObject(Vector a_lineStart, Vector a_lineEnd)
{
	// Iterate through all objects in the scene
	SceneObject * curObject = m_objects.GetHead();
	while (curObject != NULL)
	{
		// To the first object that intersects with a point
		GameObject * gameObject = curObject->GetData();
		if (gameObject->CollidesWith(a_lineStart, a_lineEnd))
		{
			return gameObject;
		}
	
		curObject = curObject->GetNext();
	}

	return NULL;
}

bool Scene::Update(float a_dt)
{
	// Iterate through all objects in the scene and update state
	bool updateSuccess = true;
	SceneObject * curObject = m_objects.GetHead();
	while (curObject != NULL)
	{
		GameObject * gameObject = curObject->GetData();
		updateSuccess &= gameObject->Update(a_dt);

		curObject = curObject->GetNext();
	}

	// Now state and position have been updated, submit resources to be rendered
	bool drawSuccess = Draw();

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
	SceneObject * curObject = m_objects.GetHead();
	while (curObject != NULL)
	{
		// Alias the game object in the scene
		GameObject * childGameObject = curObject->GetData();

		// Do not save out objects created by script
		if (!childGameObject->IsScriptOwned())
		{
			// Add the object to the game file
			childGameObject->Serialise(sceneFile, sceneObject);
		}

		curObject = curObject->GetNext();
	}
	
	// Write all the game file data to a file
	sceneFile->Write(scenePath);
	delete sceneFile;
}

bool Scene::Draw()
{
	// Iterate through all objects in the scene and update state
	bool drawSuccess = true;
	SceneObject * curObject = m_objects.GetHead();
	while (curObject != NULL)
	{
		GameObject * gameObject = curObject->GetData();
		drawSuccess &= gameObject->Draw();

		curObject = curObject->GetNext();
	}

	// Draw all the lights in the scene when debug menu is up
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		const float lightSize = 0.1f;
		for (int i = 0; i < m_numLights; ++i)
		{
			const Colour drawColour = m_lights[i].m_enabled ? sc_colourYellow : sc_colourGrey;
			RenderManager::Get().AddDebugSphere(m_lights[i].m_pos, lightSize, drawColour);
			RenderManager::Get().AddDebugLine(m_lights[i].m_pos, m_lights[i].m_pos + m_lights[i].m_dir, drawColour);
			FontManager::Get().DrawDebugString3D(m_lights[i].m_name, m_lights[i].m_pos, drawColour);
		}
	}

	return drawSuccess;
}

bool WorldManager::LoadScene(const char * a_scenePath, Scene * a_sceneToLoad_OUT)
{
	// Quick safety check
	if (a_sceneToLoad_OUT != NULL)
	{
		GameFile * sceneFile = new GameFile();
		sceneFile->Load(a_scenePath);

		// Create a new widget and copy properties from file
		if (GameFile::Object * sceneObject = sceneFile->FindObject("scene"))
		{
			// Set various properties of a scene
			bool propsOk = true;
			if (GameFile::Object * sceneObj = sceneFile->FindObject("scene"))
			{
				GameFile::Property * nameProp = sceneObj->FindProperty("name");
				GameFile::Property * beginLoadedProp = sceneObj->FindProperty("beginLoaded");
				if (nameProp && beginLoadedProp)
				{
					a_sceneToLoad_OUT->SetName(nameProp->GetString());
					a_sceneToLoad_OUT->SetBeginLoaded(beginLoadedProp->GetBool());

					// Set whole scene shader if specified
					if (GameFile::Property * shaderProp = sceneObj->FindProperty("shader"))
					{
						RenderManager::Get().ManageShader(a_sceneToLoad_OUT, shaderProp->GetString());
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
							Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Scene %s declares more than the maximum of %d lights.", a_scenePath, Shader::s_maxLights);
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
							a_sceneToLoad_OUT->AddLight(lName->GetString(), 
														lPos->GetVector(),
														lDir->GetVector(),
														lAmbient->GetFloat(),
														lDiffuse->GetFloat(),
														lSpecular->GetFloat());
							numLights++;
						}
						else
						{
							Log::Get().Write(LogLevel::Error, LogCategory::Game, "Scene %s declares a light without the required required name, pos, dir, ambient, diffuse and specular properties.");
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
				// Chidlren of the scene file can be lighting or game objects, make sure we only create game objects
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
				GameObject * newObject = CreateObject<GameObject>(templateName, a_sceneToLoad_OUT);

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
					else
					{
						Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Invalid clip type of %s specified for object %s in scene %s, defaulting to box.", clipType->GetString(), newObject->GetName(), a_scenePath);
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
				else if (a_sceneToLoad_OUT->HasLights())
				{
					// Set default shader
					newObject->SetShader(RenderManager::Get().GetLightingShader());		
				}
				
				childGameObject = childGameObject->GetNext();
			}

			// No properties present
			if (!propsOk) 
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Error loading scene file %s, scene does not have required properties.", a_scenePath);
				return false;
			}
		}
		else // Unexpected file format, no root element
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Error loading scene file %s, no valid scene parent element.", a_scenePath);
			return false;
		}
	}
	else // No valid scene to write into
	{
		Log::Get().WriteEngineErrorNoParams("Scene load failed because there is no place in memory to write to.");
		return false;
	}

	return true;
}

bool WorldManager::Startup(const char * a_templatePath, const char * a_scenePath)
{
	// Cache off the template path for non qualified loading of game object
	memset(&m_templatePath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_templatePath, a_templatePath, sizeof(char) * strlen(a_templatePath) + 1);

	// Generate list to iterate through all scenes in the scenepath and load them
	memset(&m_scenePath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_scenePath, a_scenePath, sizeof(char) * strlen(a_scenePath) + 1);
	FileManager::FileList sceneFiles;
	FileManager::Get().FillFileList(m_scenePath, sceneFiles, ".scn");

	// Load each scene in the directory
	unsigned int numSceneFiles = 0;
	FileManager::FileListNode * curNode = sceneFiles.GetHead();
	while(curNode != NULL)
	{
		char fullPath[StringUtils::s_maxCharsPerLine];
		sprintf(fullPath, "%s%s", a_scenePath, curNode->GetData()->m_name);
		
		// Add to the loaded scenes if begin loaded
		// TODO Implement memory management
		Scene * newScene = new Scene();
		if (LoadScene(fullPath, newScene))
		{
			if (newScene->IsBeginLoaded())
			{
				// Insert into list
				SceneNode * newSceneNode = new SceneNode();
				newSceneNode->SetData(newScene);
				m_scenes.Insert(newSceneNode);

				// Set current scene to the first loaded scene
				if (m_currentScene == NULL)
				{
					m_currentScene = newScene;
				}
			}

			++numSceneFiles;
		}
		else
		{
			// Clean up the scene, the load will report the error
			delete newScene;
		}
		curNode = curNode->GetNext();
	}

	// Clean up the list of fonts
	FileManager::Get().CleanupFileList(sceneFiles);

	// If no scenes, setup the default scene
	if (numSceneFiles == 0)
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
	// Iterate through all loaded scenes and update them
	bool updateOk = true;
	SceneNode * next = m_scenes.GetHead();
	while(next != NULL)
	{
		updateOk &= next->GetData()->Update(a_dt);
		next = next->GetNext();
	}

	return updateOk;
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

bool WorldManager::DestoryAllObjects(bool a_destroyScriptOwned)
{
	// Iterate through all objects in the scene
	bool objectDestroyed = false;
	LinkedListNode<GameObject> * curObject = m_currentScene->GetHeadObject();
	while (curObject != NULL)
	{
		// Cache off next pointer as destruction will unlink the destroyed object from the list
		LinkedListNode<GameObject> * nextObj = curObject->GetNext();

		// Test if script owned before destruction
		GameObject * gameObject = curObject->GetData();
		bool destroyObject = !a_destroyScriptOwned && gameObject->IsScriptOwned() ? false : true;
		if (destroyObject)
		{
			objectDestroyed &= DestroyObject(gameObject->GetId());
		}
		curObject = nextObj;
	}
	
	return objectDestroyed;
}

bool WorldManager::DestoryAllScriptOwnedObjects(bool a_destroyScriptBindings)
{
	// Iterate through all objects in the scene
	bool objectDestroyed = false;
	LinkedListNode<GameObject> * curObject = m_currentScene->GetHeadObject();
	while (curObject != NULL)
	{
		// Cache off next pointer as destruction will unlink the destroyed object from the list
		LinkedListNode<GameObject> * nextObj = curObject->GetNext();

		// Only destroy script owned objects
		GameObject * gameObject = curObject->GetData();
		if (gameObject->IsScriptOwned())
		{
			objectDestroyed &= DestroyObject(gameObject->GetId(), a_destroyScriptBindings);
		}
		curObject = nextObj;
	}
	
	return objectDestroyed;
}

GameObject * WorldManager::GetGameObject(unsigned int a_objectId)
{
	// First try to find the object in the current scene
	if (GameObject * foundObject = m_currentScene->GetSceneObject(a_objectId))
	{
		return foundObject;
	}
	else // Look through each scene for the target object
	{
		SceneNode * next = m_scenes.GetHead();
		while(next != NULL)
		{
			if (GameObject * foundObject = next->GetData()->GetSceneObject(a_objectId))
			{
				return foundObject;
			}
			next = next->GetNext();
		}
	}

	// Failure case
	return NULL;
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
