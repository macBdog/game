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
		a_newObject->SetState(GameObject::eGameObjectState_Active);
		m_objects.Insert(newSceneObject);

		++m_numObjects;
	}
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
	
	// Add each object in the scene
	SceneObject * curObject = m_objects.GetHead();
	while (curObject != NULL)
	{
		// Alias the game object in the scene
		GameObject * childGameObject = curObject->GetData();
		
		// Add the object to the game file
		childGameObject->Serialise(sceneFile, sceneObject);
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
			if (sceneFile->FindProperty(sceneObject, "name") &&
				sceneFile->FindProperty(sceneObject, "beginLoaded"))
			{
				a_sceneToLoad_OUT->SetName(sceneFile->GetString("scene", "name"));
				a_sceneToLoad_OUT->SetBeginLoaded(sceneFile->GetBool("scene", "beginLoaded"));

				// Set whole scene shader if specified
				if (sceneFile->FindProperty(sceneObject, "shader"))
				{
					if (Shader * pNewShader = new Shader(sceneFile->GetString("scene", "shader")))
					{
						if (RenderManager::InitShaderFromFile(*pNewShader))
						{
							a_sceneToLoad_OUT->SetShader(pNewShader);
							RenderManager::Get().ManageShader(a_sceneToLoad_OUT);
						}	
						else // Compile error will be reported in the log
						{
							delete pNewShader;
						}
					}
				}
			}
			else
			{
				propsOk = false;
			}

			// Load child game objects of the scene
			GameFile::Object * childGameObject = sceneObject->m_firstChild;
			while (childGameObject != NULL)
			{
				if (GameFile::Property * prop = childGameObject->FindProperty("template"))
				{
					// Create object from template values and add to the scene
					if (GameObject * newObject = CreateObject<GameObject>(prop->GetString(), a_sceneToLoad_OUT))
					{
						// Override any template values
						newObject->SetTemplate(prop->GetString());
						newObject->SetName(childGameObject->FindProperty("name")->GetString());
						newObject->SetPos(childGameObject->FindProperty("pos")->GetVector());
					}
				}
				
				childGameObject = childGameObject->m_next;
			}

			// No properties present
			if (!propsOk) 
			{
				Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Error loading scene file %s, scene does not have required properties.", a_scenePath);
				return false;
			}
		}
		else // Unexpected file format, no root element
		{
			Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Error loading scene file %s, no valid scene parent element.", a_scenePath);
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
	FileManager::Get().EmptyFileList(sceneFiles);

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
