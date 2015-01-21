#ifndef _ENGINE_ANIMATION_BLENDER_
#define _ENGINE_ANIMATION_BLENDER_
#pragma once

#include "../core/Matrix.h"

#include "StringHash.h"

class GameObject;
struct KeyFrame;

//\brief Animation Blender reads animation data and applies it to a model
class AnimationBlender
{
public:

	//\ No work done in the constructor, only Init
	AnimationBlender(GameObject * a_gameObj) 
		: m_gameObject(a_gameObj) { }
	
	//\brief Update will apply keyframes to the model from animation channels
	bool Update(float a_dt);

	//\brief Play will start the blender mixing data from the provided keyframe stream into the transforms
	inline bool PlayAnimation(KeyFrame * a_data, int a_numFrames, int a_frameRate, StringHash a_animName)
	{
		int freeChannel = GetChannel();
		if (freeChannel >= 0 && freeChannel < s_maxAnimationChannels)
		{
			m_channels[freeChannel].m_data = a_data;
			m_channels[freeChannel].m_curFrame = 0;
			m_channels[freeChannel].m_numFrames = a_numFrames;
			m_channels[freeChannel].m_frameRateRecip = 1.0f / a_frameRate;
			m_channels[freeChannel].m_lastFrame = 0.0f;
			m_channels[freeChannel].m_active = true;
			m_channels[freeChannel].m_name = a_animName;
			return true;
		}
		return false;
	}

private:

	static const int s_maxAnimationChannels = 8;			///< Maximum animations that can be playing on one model at a time

	void ApplyKeyToWorld(const Vector & a_pos, const Vector & a_rot, const Vector & a_scale, Matrix & a_world) const;

	//\brief An animation channel is grouping of data to keep track of keyframes to apply
	struct AnimationChannel
	{
		AnimationChannel()
			: m_active(false)
			, m_influence(0.0f)
			, m_curFrame(0)
			, m_numFrames(0)
			, m_frameRateRecip(0.0f)
			, m_lastFrame(0.0f)
			, m_name()
			, m_data(NULL) { }
		bool m_active;				///< If an animation is currently playing
		float m_influence;			///< How much weight the animation has
		int m_curFrame;				///< How far through the animation
		int m_numFrames;			///< How many frames are being played
		float m_frameRateRecip;		///< Reciprocal of how many frames should be played per second
		float m_lastFrame;			///< The time elapsed since the last frame was played
		StringHash m_name;			///< The name of the animation that is being played
		KeyFrame * m_data;			///< The current keyframe data for the channel
	};

	//\brief Get the next free channel to play an animation on
	//\return the channel id to play on or -1 for no available channels
	int GetChannel();

	GameObject * m_gameObject;								///< The game object that owns this blender
	AnimationChannel m_channels[s_maxAnimationChannels];	///< A number of animations can be playing at once
};

#endif // _ENGINE_ANIMATION_BLENDER_
