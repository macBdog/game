#include "Log.h"

#include "WorldManager.h"

template<> WorldManager * Singleton<WorldManager>::s_instance = NULL;

bool WorldManager::Startup(const char * a_templatePath)
{
	// Cache off the template path for non qualified loading of game object
	memset(&m_templatePath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_templatePath, a_templatePath, strlen(a_templatePath));

	// Init scene object counts
	m_defaultSceneNumObjects = 0;

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

	// Iterate through all objects in the scene and update state
	bool updateSuccess = true;
	SceneObject * curObject = m_defaultScene.GetHead();
	while (curObject != NULL)
	{
		GameObject * gameObject = curObject->GetData();
		
		updateSuccess &= gameObject->Update(a_dt);
	}

	// Now state and position have been updated, submit resources to be rendered
	bool drawSuccess = Draw();

	return updateSuccess && drawSuccess;
}

bool WorldManager::Draw()
{
	// TODO Iterate through all scenes

	// Iterate through all objects in the scene and update state
	bool drawSuccess = true;
	SceneObject * curObject = m_defaultScene.GetHead();
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
	if (GameObject * newGameObject = new GameObject(m_defaultSceneNumObjects++))
	{
		// Load resources
	
		return newGameObject;
	}
	else
	{
		return NULL;
	}
}

GameObject * WorldManager::GetObject(unsigned int a_objectId)
{
	// Look through each category for the target texture
	SceneObject * curObject = m_defaultScene.GetHead();
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