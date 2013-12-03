#ifndef _ENGINE_ANIMATION_MANAGER_
#define _ENGINE_ANIMATION_MANAGER_
#pragma once

#include "../core/LinearAllocator.h"
#include "../core/LinkedList.h"
#include "../core/Matrix.h"

#include "FileManager.h"
#include "Singleton.h"
#include "StringHash.h"

class GameObject;

//\brief A KeyFrame stores data to apply to a model at a certain time
struct KeyFrame
{
	KeyFrame()
		: m_time(0.0f)
		, m_prs()
		, m_transformName() { }
	float m_time;					///< What relative time the keyframe is applied
	Matrix m_prs;					///< Where the keyframe locates the transform
	StringHash m_transformName;		///< What the keyframe locates 
};

//\brief AnimationManager loads animations for disk resources and supplies to animation blenders
class AnimationManager : public Singleton<AnimationManager>
{
public:

	//\ No work done in the constructor, only Init
	AnimationManager(float a_updateFreq = s_updateFreq) 
		: m_updateFreq(a_updateFreq)
		, m_updateTimer(0.0f)
		, m_data(NULL)
		{ m_animPath[0] = '\0'; }
	~AnimationManager() { Shutdown(); }

	//\brief Set clear colour buffer and depth buffer setup 
    bool Startup(const char * a_animPath);
	bool Shutdown();

	//\brief Update and reload animation data
	bool Update(float a_dt);

	//\brief Play an animation on a game object
	bool PlayAnimation(GameObject * a_gameObj, const StringHash & a_animName);

private:

	static const unsigned int s_animPoolSize;					///< How much memory is assigned for all game animations
	static const float s_updateFreq;							///< How often the animation manager should check for resource updates

	//\brief A managed animation stores animation data and metadata about the file resource
	struct ManagedAnim
	{
		ManagedAnim(const char * a_animPath, const char * a_animName, const FileManager::Timestamp & a_timeStamp)
			: m_timeStamp(a_timeStamp)	
			, m_numKeys(0)
			, m_name(a_animName)
			, m_data(NULL)
		{ 
			strcpy(&m_path[0], a_animPath); 
		}
		FileManager::Timestamp m_timeStamp;						///< When the anim file was last edited
		char m_path[StringUtils::s_maxCharsPerLine];			///< Where the anim resides for reloading
		StringHash m_name;										///< What the anim is called
		int m_numKeys;											///< How many keys are in the animation
		int m_frameRate;										///< The speed at which the animation should be played
		KeyFrame * m_data;										///< Pointer to the keyframe data
	};

	typedef LinkedListNode<ManagedAnim> ManagedAnimNode;		///< Alias for a linked list node that points to a managed animation
	typedef LinkedList<ManagedAnim> ManagedAnimList;			///< Alias for a linked list of managed animations

	//\brief Load each take from an ascii FBX file into managed animations
	//\param a_fbxPath the path to the file
	//\return int the number of animations loaded
	int LoadAnimationsFromFile(const char * a_fbxPath);

	ManagedAnimList m_anims;									///< List of all the scripts found on disk at startup
	LinearAllocator<KeyFrame> m_data;							///< Keyframe data shared with blenders
	char m_animPath[StringUtils::s_maxCharsPerLine];			///< Cache off path to animation data 
	float m_updateFreq;											///< How often the script manager should check for changes to shaders
	float m_updateTimer;										///< If we are due for a scan and update of scripts
	KeyFrame * m_last;											///< Last allocated animation memory
};

#endif // _ENGINE_ANIMATION_MANAGER
