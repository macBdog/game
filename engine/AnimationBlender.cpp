#include "../core/MathUtils.h"
#include "../core/Quaternion.h"

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
	Vector keyPos(0.0f);
	Vector keyRot(0.0f);
	Vector keyScale(0.0f);

	// Iterate through each channel and increment it's progress
	bool playedAnim = false;
	for (int i = 0; i < s_maxAnimationChannels; ++i)
	{
		if (m_channels[i].m_active)
		{
			const float fracToNextFrame = MathUtils::GetMin(m_channels[i].m_lastFrame / m_channels[i].m_frameRateRecip, 1.0f);
			
			// Lerp between postions
			Vector curKey = m_channels[i].m_data->m_pos;
			Vector nextKey = m_channels[i].m_curFrame < m_channels[i].m_numFrames-1 ? (m_channels[i].m_data+1)->m_pos : curKey;
			keyPos = MathUtils::LerpVector(curKey, nextKey, fracToNextFrame);
			
			// Rotations
			curKey = m_channels[i].m_data->m_rot;
			nextKey = m_channels[i].m_curFrame < m_channels[i].m_numFrames-1 ? (m_channels[i].m_data+1)->m_rot : curKey;
			keyRot = MathUtils::LerpVector(curKey, nextKey, fracToNextFrame);

			// Scale
			curKey = m_channels[i].m_data->m_scale;
			nextKey = m_channels[i].m_curFrame < m_channels[i].m_numFrames-1 ? (m_channels[i].m_data+1)->m_scale : curKey;
			keyScale = MathUtils::LerpVector(curKey, nextKey, fracToNextFrame);

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
	local = Matrix::Identity();
	Quaternion rot(MathUtils::Deg2Rad(keyRot));
	local = local.Multiply(rot.GetRotationMatrix());
	local.Scale(keyScale);
	local.SetPos(keyPos);

	return playedAnim;
}