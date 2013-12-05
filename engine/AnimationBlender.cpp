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

	// Construct an aggregate matrix of all the playing animations
	Matrix animMat = Matrix::Identity();

	// Iterate through each channel and increment it's progress
	bool playedAnim = false;
	for (int i = 0; i < s_maxAnimationChannels; ++i)
	{
		if (m_channels[i].m_active)
		{
			Vector animRootPos(m_channels[i].m_data->m_prs.GetPos());
			animMat.Multiply(m_channels[i].m_data->m_prs);
			animMat.SetPos(animRootPos);

			// If it's time for a new frame
			if (m_channels[i].m_curFrame == 0 || m_channels[i].m_lastFrame >= m_channels[i].m_frameRateRecip)
			{
				if (m_channels[i].m_curFrame > 0)
				{
					m_channels[i].m_lastFrame -= m_channels[i].m_frameRateRecip;
				}
				m_channels[i].m_curFrame++;
				m_channels[i].m_data++;
				playedAnim = true;
			}
			else // Accumulate time
			{
				m_channels[i].m_lastFrame += a_dt;
			}
		}

		// Stop the animation if it's run out of frames
		if (m_channels[i].m_curFrame >= m_channels[i].m_numFrames)
		{
			m_channels[i].m_active = false;
		}
	}

	// Apply the aggregate matrix to the object
	Matrix & local = m_gameObject->GetLocalMat();
	local = animMat;

	return playedAnim;
}