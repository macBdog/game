#include "Log.h"

#include "WorldManager.h"

template<> WorldManager * Singleton<WorldManager>::s_instance = NULL;

void Scene::AddObject(GameObject * a_newObject)
{
	if (SceneObject * newSceneObject = new SceneObject())
	{
		newSceneObject->SetData(a_newObject);
		m_objects.Insert(newSceneObject);

		++m_numObjects;
	}
}

GameObject * Scene::GetObject(unsigned int a_objectId)
{
	SceneObject * curObject = m_objects.GetHead();
	while (curObject != NULL)
	{
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

		if (gameObject->IsActive())
		{
			drawSuccess &= gameObject->Draw();
		}

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
	// Template paths are either fully qualified or relative to the config template dir
	char fileNameBuf[StringUtils::s_maxCharsPerLine];
	if (!strstr(a_templatePath, ":\\"))
	{
		sprintf(fileNameBuf, "%s%s", m_templatePath, a_templatePath);
	} 
	else // Already fully qualified
	{
		sprintf(fileNameBuf, "%s", a_templatePath);
	}

	// TODO Please allocate a heap for game objects
	if (GameObject * newGameObject = new GameObject(m_totalSceneNumObjects++))
	{
		// Load resources

		// Add to currently active scene
		m_defaultScene.AddObject(newGameObject);
	
		return newGameObject;
	}
	else
	{
		return NULL;
	}
}

GameObject * WorldManager::GetObject(unsigned int a_objectId)
{
	// TODO Look through each scene for the target object
	return m_defaultScene.GetObject(a_objectId);
}