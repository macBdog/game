#include "CollisionUtils.h"
#include "DebugMenu.h"
#include "FontManager.h"
#include "ModelManager.h"
#include "PhysicsManager.h"
#include "RenderManager.h"
#include "ScriptManager.h"
#include "WorldManager.h"

#include "Scene.h"

template<> WorldManager * Singleton<WorldManager>::s_instance = nullptr;

const float Scene::s_updateFreq = 1.0f;								///< How often the scene should check it's config on disk for updates

Scene::Scene()
: m_state(SceneState::Unloaded)
, m_beginLoaded(false)
, m_shader(nullptr)
, m_numLights(0)
, m_updateTimer(0.0f)
, m_updateFreq(s_updateFreq)
, m_name("defaultScene")
{
	// Allocate memory for the scene's object pool
	m_objects.Init(s_numObjects, sizeof(GameObject));
}

Scene::~Scene()
{
	Reset();
}

void Scene::Reset()
{
	// Shutdown all game objects in the scene
	for (unsigned int i = 0; i < m_objects.GetCount(); ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			gameObj->Shutdown();
		}
	}

	// Clean up shader if set
	if (m_shader != nullptr)
	{
		RenderManager::Get().UnManageShader(this);
		m_shader = nullptr;
	}

	m_numLights = 0;
	m_objects.Reset();
}

bool Scene::InitFromConfig()
{
	// Create a new widget and copy properties from file
	if (GameFile::Object * sceneObject = m_sourceFile.FindObject("scene"))
	{
		// Set various properties of a scene
		bool propsOk = true;
		GameFile::Property * nameProp = sceneObject->FindProperty("name");
		GameFile::Property * beginLoadedProp = sceneObject->FindProperty("beginLoaded");
		if (nameProp && beginLoadedProp)
		{
			SetName(nameProp->GetString());
			SetBeginLoaded(beginLoadedProp->GetBool());

			// Set whole scene shader if specified
			if (GameFile::Property * shaderProp = sceneObject->FindProperty("shader"))
			{
				RenderManager::Get().ManageShader(this, shaderProp->GetString());
			}
		}
		else
		{
			propsOk = false;
		}

		// Support for up to four lights per scene
		if (GameFile::Object * lightingObj = sceneObject->FindObject("lighting"))
		{
			int numLights = 0;
			auto & lights = lightingObj->GetChildObjects();
			for (GameFile::Object * light : lights)
			{
				// Check there aren't more lights than we support
				if (numLights >= Shader::s_maxLights)
				{
					Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Scene %s declares more than the maximum of %d lights.", GetName(), Shader::s_maxLights);
					propsOk = false;
					break;
				}
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
			}
		}

		// Load child game objects of the scene
		if (GameFile::Object * gameObjectsArray = sceneObject->FindObject("gameObjects"))
		{
			auto & gameObjects = gameObjectsArray->GetChildObjects();
			for (GameFile::Object * childObj : gameObjects)
			{
				const std::string * templateName = nullptr;
				if (GameFile::Property * prop = childObj->FindProperty("template"))
				{
					templateName = &prop->GetString();
				}

				// Create object with optional template values and add to the scene
				GameObject * newObject = WorldManager::Get().CreateObject(templateName ? templateName->c_str() : nullptr, this);

				// Override any templated values
				if (templateName != nullptr)
				{
					newObject->SetTemplate(*templateName);
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
					const auto & clipStr = clipType->GetString();
					if (clipStr.find(GameObject::s_clipTypeStrings[static_cast<int>(ClipType::Sphere)]) != std::string::npos)
					{
						newObject->SetClipType(ClipType::Sphere);
					}
					else if (clipStr.find(GameObject::s_clipTypeStrings[static_cast<int>(ClipType::AxisBox)]) != std::string::npos)
					{
						newObject->SetClipType(ClipType::AxisBox);
					}
					else if (clipStr.find(GameObject::s_clipTypeStrings[static_cast<int>(ClipType::Box)]) != std::string::npos)
					{
						newObject->SetClipType(ClipType::Box);
					}
					else if (clipStr.find(GameObject::s_clipTypeStrings[static_cast<int>(ClipType::Mesh)]) != std::string::npos)
					{
						Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Mesh clip type no longer supported for object %s in scene %s, defaulting to axisbox.", newObject->GetName(), GetName());
						newObject->SetClipType(ClipType::AxisBox);
					}
					else
					{
						Log::Get().Write(LogLevel::Warning, LogCategory::Game, "Invalid clip type of %s specified for object %s in scene %s, defaulting to box.", clipStr.c_str(), newObject->GetName(), GetName());
						newObject->SetClipType(ClipType::AxisBox);
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
				if (GameFile::Property* clipGroup = childObj->FindProperty("clipGroup"))
				{
					newObject->SetClipGroup(clipGroup->GetString(), PhysicsManager::Get().GetCollisionGroupId(clipGroup->GetString()));
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
			}
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

GameObject * Scene::AddObject()
{
	// Scene objects are stored contiguously in object ID order
	GameObject * newGameObject = m_objects.Add();
	return newGameObject;
}

bool Scene::AddLight(std::string_view a_name, const Vector & a_pos, const Quaternion & a_dir, const Colour & a_ambient, const Colour & a_diffuse, const Colour & a_specular)
{
	if (m_numLights < Shader::s_maxLights)
	{
		m_lights[m_numLights].m_name = a_name;
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

bool Scene::RemoveLight(std::string_view a_name)
{
	for (unsigned int i = 0; i < Shader::s_maxLights; ++i)
	{
		if (m_lights[i].m_name == a_name)
		{
			// If the light we are looking for is on the end of the light array
			if (m_numLights == static_cast<int>(i))
			{
				m_lights[i] = Light();
			}
			else // Swap the end light with this one
			{
				m_lights[i] = m_lights[m_numLights-1];
			}
			--m_numLights;
			return true;
		}
	}
	return false;
}

GameObject * Scene::GetSceneObject(unsigned int a_storageId)
{
	if (a_storageId < m_objects.GetCount())
	{
		return m_objects.Get(a_storageId);
	}
	return nullptr;
}

GameObject * Scene::GetSceneObject(std::string_view a_objName)
{
	// Iterate through all objects in the scene
	for (unsigned int i = 0; i < m_objects.GetCount(); ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			if (std::string_view(gameObj->GetName()).find(a_objName) != std::string_view::npos)
			{
				return gameObj;
			}
		}
	}

	return nullptr;
}

GameObject * Scene::GetSceneObject(Vector a_worldPos)
{
	// Iterate through all objects in the scene
	for (unsigned int i = 0; i < m_objects.GetCount(); ++i)
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
	return nullptr;
}

GameObject * Scene::GetSceneObject(Vector a_lineStart, Vector a_lineEnd)
{
	// Iterate through all objects in the scene
	for (unsigned int i = 0; i < m_objects.GetCount(); ++i)
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
	return nullptr;
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
	return nullptr;
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
	return nullptr;
}

void Scene::RemoveAllObjects(bool a_destroyScriptOwned)
{
	for (unsigned int i = 0; i < m_objects.GetCount(); ++i)
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
	m_objects.Reset();
}

void Scene::RemoveAllScriptOwnedObjects(bool a_destroyScriptBindings)
{
	// Remove all objects including scene and script owned objects
	for (unsigned int i = 0; i < m_objects.GetCount(); ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			gameObj->Shutdown();
		}
	}

	m_numLights = 0;
	m_objects.Reset();

	// Load scene front scratch so we are back with just scene objects and no script objects
	if (!m_filePath.empty() && m_sourceFile.Load(m_filePath))
	{
		InitFromConfig();
	}
}

bool Scene::Update(float a_dt)
{
	// Iterate through all objects in the scene and update state
	bool updateSuccess = true;
	for (unsigned int i = 0; i < m_objects.GetCount(); ++i)
	{
		if (GameObject * gameObj = m_objects.Get(i))
		{
			updateSuccess &= gameObj->Update(a_dt);
		}
	}

	// Now state and position have been updated, submit resources to be rendered
	bool drawSuccess = Draw();

	DataPack & dataPack = DataPack::Get();
	if (dataPack.IsLoaded())
	{
		return true;
	}

#ifndef _RELEASE
	// Check if the scene needs to be reloaded
	if (m_updateTimer < m_updateFreq)
	{
		m_updateTimer += a_dt;
	}
	else
	{
		if (!m_filePath.empty())
		{
			m_updateTimer = 0.0f;
			FileManager::Timestamp curTimeStamp;
			FileManager::Get().GetFileTimeStamp(m_filePath, curTimeStamp);
			if (curTimeStamp > m_timeStamp)
			{
				Reset();

				if (m_sourceFile.Load(m_filePath))
				{
					InitFromConfig();
				}
				ScriptManager::Get().ReloadScripts();
				ResetFileDateStamp();
			}
		}
	}
#endif

	return updateSuccess && drawSuccess;
}

void Scene::Serialise()
{
	// Construct the path from the scene directory and name of the scene
	std::string scenePath = std::string(WorldManager::Get().GetScenePath()) + m_name + ".json";

	// Create an output stream
	GameFile * sceneFile = new GameFile();

	GameFile::Object * sceneObject = sceneFile->AddObject("scene");
	sceneFile->AddProperty(sceneObject, "name", m_name);
	sceneFile->AddProperty(sceneObject, "beginLoaded", m_beginLoaded);

	// Optional properties of the scene
	if (m_shader != nullptr)
	{
		sceneFile->AddProperty(sceneObject, "shader", m_shader->GetName());
	}

	// Add lighting section as an array
	if (HasLights())
	{
		for (int i = 0; i < m_numLights; ++i)
		{
			GameFile::Object * curLightObj = sceneFile->AddObject("lighting", sceneObject);
			sceneFile->AddProperty(curLightObj, "name", m_lights[i].m_name);
			sceneFile->AddProperty(curLightObj, "pos", m_lights[i].m_pos);
			sceneFile->AddProperty(curLightObj, "dir", m_lights[i].m_dir);
			sceneFile->AddProperty(curLightObj, "ambient", m_lights[i].m_ambient);
			sceneFile->AddProperty(curLightObj, "diffuse", m_lights[i].m_diffuse);
			sceneFile->AddProperty(curLightObj, "specular", m_lights[i].m_specular);
		}
	}

	// Add each object in the scene as an array
	for (unsigned int i = 0; i < m_objects.GetCount(); ++i)
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
	for (unsigned int i = 0; i < m_objects.GetCount(); ++i)
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
			FontManager::Get().DrawDebugString3D(m_lights[i].m_name.c_str(), m_lights[i].m_pos, drawColour);
		}
	}

	return drawSuccess;
}
