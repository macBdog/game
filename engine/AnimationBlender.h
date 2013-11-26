#ifndef _ENGINE_ANIMATION_BLENDER_
#define _ENGINE_ANIMATION_BLENDER_
#pragma once

#include "../core/Matrix.h"

#include "StringHash.h"

struct KeyFrame;

//\brief Animation Blender reads animation data and applies it to a model
class AnimationBlender
{
public:

	//\ No work done in the constructor, only Init
	AnimationBlender() { }
	
	//\brief Update will apply keyframes to the model from animation channels
	bool Update(float a_dt);

private:

	static const int s_maxAnimationChannels = 8;			///< Maximum animations that can be playing on one model at a time

	//\brief An animation channel is grouping of data to keep track of keyframes to apply
	struct AnimationChannel
	{
		AnimationChannel()
			: m_active(false)
			, m_influence(0.0f)
			, m_curFrame(0)
			, m_numFrames(0)
			, m_name()
			, m_data(NULL) { }
		bool m_active;		///< If an animation is currently playing
		float m_influence;	///< How much weight the animation has
		int m_curFrame;		///< How far through the animation
		int m_numFrames;	///< How many frames are being played
		StringHash m_name;	///< The name of the animation that is being played
		KeyFrame * m_data;	///< The current keyframe data for the channel
	};

	AnimationChannel m_channels[s_maxAnimationChannels];	///< A number of animations can be playing at once
};

#endif // _ENGINE_ANIMATION_BLENDER_
