#include "GameFile.h"
#include "Log.h"
#include "ModelManager.h"

#include "WorldManager.h"

template<> WorldManager * Singleton<WorldManager>::s_instance = NULL;

void Scene::AddObject(GameObject * a_newObject)
{
	// TODO Scene objects should also have a heap
	if (SceneObject * newSceneObject = new SceneObject())
	{
		newSceneObject->SetData(a_newObject);
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

bool WorldManager::Startup(const char * a_templatePath)
{
	// Cache off the template path for non qualified loading of game object
	memset(&m_templatePath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_templatePath, a_templatePath, strlen(a_templatePath));

	return true;
}

bool WorldManager::Shutdown()
{
	// Cleanup scene objects

	return true;
}

bool WorldManager::Update(float a_dt)
{
	// TODO Iterate through all scenes
	return m_defaultScene.Update(a_dt);
}

GameObject * WorldManager::CreateObject(const char * a_templatePath)
{
	// TODO Please allocate a heap for game objects

	// Template paths are either fully qualified or relative to the config template dir
	ModelManager & modelMan = ModelManager::Get();
	if (a_templatePath)
	{
		char fileNameBuf[StringUtils::s_maxCharsPerLine];
		if (!strstr(a_templatePath, ":\\"))
		{
			sprintf(fileNameBuf, "%s%s", m_templatePath, a_templatePath);
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
			if (GameObject * newGameObject = new GameObject(m_totalSceneNumObjects++))
			{
				newGameObject->SetState(GameObject::eGameObjectState_Loading);
				newGameObject->SetPos(Vector(0.0f, 0.0f, -20.0f));

				if (GameFile::Object * object = templateFile.FindObject("object"))
				{
					if (GameFile::Property * name = object->FindProperty("name"))
					{
						newGameObject->SetName(name->GetString());
					}
					if (GameFile::Property * model = object->FindProperty("model"))
					{
						newGameObject->SetModel(modelMan.GetModel(model->GetString()));
					}
					// TODO Pos, rot, shader, etc

					// Add to currently active scene
					m_defaultScene.AddObject(newGameObject);

					return newGameObject;
				}
			}
		}
		else
		{
			Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Unable to load template file %s", a_templatePath);
		}
	}
	else // Create default object
	{
		if (GameObject * newGameObject = new GameObject(m_totalSceneNumObjects++))
		{
			newGameObject->SetState(GameObject::eGameObjectState_Loading);
			newGameObject->SetName("NEW_GAME_OBJECT");
			newGameObject->SetPos(Vector(0.0f, 0.0f, -20.0f));
				
			// Add to currently active scene
			m_defaultScene.AddObject(newGameObject);

			return newGameObject;
		}
	}

	return NULL;
}

GameObject * WorldManager::GetGameObject(unsigned int a_objectId)
{
	// TODO Look through each scene for the target object
	return m_defaultScene.GetSceneObject(a_objectId);
}