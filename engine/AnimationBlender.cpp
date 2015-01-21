#include "../core/MathUtils.h"
#include "../core/Quaternion.h"

#include "AnimationManager.h"
#include "GameObject.h"
#include "Log.h"

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

void AnimationBlender::ApplyKeyToWorld(const Vector & a_pos, const Vector & a_rot, const Vector & a_scale, Matrix & a_world) const
{
	const Vector worldPos = a_world.GetPos() + a_pos;
	const Vector worldScale = a_world.GetScale() * a_scale;
	Quaternion keyRotation(MathUtils::Deg2Rad(a_rot));
	a_world = a_world.Multiply(keyRotation.GetRotationMatrix());
	a_world.SetPos(worldPos);
	a_world.SetScale(worldScale);
}

bool AnimationBlender::Update(float a_dt)
{
	if (m_gameObject == NULL)
	{
		return false;
	}

	// Construct an aggregate matrix of all the playing animations
	Vector blendedPos(0.0f);
	Vector blendedRot(0.0f);
	Vector blendedScale(1.0f);

	// Iterate through each channel and increment it's progress
	bool playedAnim = false;
	Matrix & local = m_gameObject->GetLocalMat();
	Matrix & world = m_gameObject->GetWorldMat();
	for (int i = 0; i < s_maxAnimationChannels; ++i)
	{
		Vector keyPos(0.0f);
		Vector keyRot(0.0f);
		Vector keyScale(1.0f);
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
		if (m_channels[i].m_curFrame > 0 && m_channels[i].m_curFrame >= m_channels[i].m_numFrames)
		{
			m_channels[i].m_active = false;

			// Push the channels last state onto the world matrix of the object so it's persistent
			ApplyKeyToWorld(keyPos, keyRot, keyScale, world);
		}
		else
		{
			blendedPos += keyPos;
			blendedRot += keyRot;
			blendedScale *= keyScale;
		}
	}

	// Set the channels matrix to the key
	local = Matrix::Identity();
	Quaternion channelRot(MathUtils::Deg2Rad(blendedRot));
	local = local.Multiply(channelRot.GetRotationMatrix());
	local.SetScale(blendedScale);
	local.SetPos(blendedPos);

	return playedAnim;
}