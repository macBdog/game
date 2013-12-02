#include "AnimationManager.h"
#include "GameObject.h"

#include "AnimationBlender.h"

int AnimationBlender::GetChannel()
{
	// Find and return an inactive channel
	{
		for (int i = 0; i < s_maxAnimationChannels; ++i)
		{
			if (!m_channels[i].m_active)
			{
				return i;
			}
		}
	}
	return -1;
}

bool AnimationBlender::Update(float a_dt)
{
	if (m_gameObject == NULL)
	{
		return false;
	}

	// Iterate through each channel and increment it's progress
	bool playedAnim = false;
	for (int i = 0; i < s_maxAnimationChannels; ++i)
	{
		if (m_channels[i].m_active)
		{
			Matrix & worldMat = m_gameObject->GetWorldMat();
			Vector tAxis = worldMat.GetPos();
			tAxis = tAxis + m_channels[i].m_data->m_prs.GetPos();
			worldMat.SetPos(tAxis);
			m_gameObject->SetWorldMat(worldMat);
			
			m_channels[i].m_curFrame++;
			m_channels[i].m_data++;
			playedAnim = true;
		}

		// Stop the animation if it's run out of frames
		if (m_channels[i].m_curFrame >= m_channels[i].m_numFrames)
		{
			m_channels[i].m_active = false;
		}
	}

	return playedAnim;
}