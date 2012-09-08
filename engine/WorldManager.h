#ifndef _ENGINE_WORLD_MANAGER_H_
#define _ENGINE_WORLD_MANAGER_H_

#include "../core/HashMap.h"
#include "../core/LinearAllocator.h"
#include "../core/LinkedList.h"

#include "GameObject.h"
#include "Singleton.h"
#include "StringHash.h"
#include "StringUtils.h"

//\brief WorldManager handles object and scene management.
//		 The current thought is that the world is made of scenes.
//		 And scenes are filled with objects.
class WorldManager : public Singleton<WorldManager>
{
public:

	//\brief Ctor calls through to startup
	WorldManager() { }
	~WorldManager() { Shutdown(); }

	//\brief Initialise memory pools on startup, cleanup worlds objects on shutdown
	//\param a_templatePath is the path to templates for game object creation
	bool Startup(const char * a_templatePath);
	bool Shutdown();

	//\brief Update will propogate through all scenes and objects
	//\return true if a world was update without issue
	bool Update(float a_dt);

	//\brief Create and object from an optional game file template
	//\param a_templatePath Pointer to a cstring with an optional game file to create from
	//\return A pointer to the newly created game object of NULL for failure
	GameObject * CreateObject(const char * a_templatePath = NULL);

	//\brief Get a pointer to an existing object in the world.
	//\param a_objectId the unique game id for this object
	//\return Pointer to a game object in the world
	GameObject * GetObject(unsigned int a_objectId);

	//\brief Reload an object...

	//\brief Accessor for the relative template path
	inline const char * GetTemplatePath() { return m_templatePath; }

private:

	//\brief Alias to refer to a group of objects
	typedef LinkedListNode<GameObject> SceneObject;
	typedef LinkedList<GameObject> Scene;

	//\brief Draw will cause active objects to submit resources to the render manager
	//\return true if resources were submitted without issue
	bool Draw();

	Scene	m_defaultScene;					///< Eventually scenes can be added and removed by the user
	unsigned int m_defaultSceneNumObjects;	///< How many objects are in the default scene
	char	m_templatePath[StringUtils::s_maxCharsPerLine]; ///< Path for templates
};

#endif /* _ENGINE_WORLD_MANAGER_H_ */