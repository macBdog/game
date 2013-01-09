#ifndef _ENGINE_COMPONENT_ROOT_MOTION_
#define _ENGINE_COMPONENT_ROOT_MOTION_
#pragma once

#include "../../core/MathUtils.h"
#include "../../core/Vector.h"

#include "../GameObject.h"

#include "Component.h"

/// \brief Root motion component can move game objects about in the world over time.
class ComponentRootMotion : public Component
{
public:

	//\brief eMoveType is the type of movement supported
	enum eMoveType
	{
		eMoveTypeLinear = 0,
		eMoveTypeAccelerate,
		eMoveTypeDecelerate,
		eMoveTypeAccelAndDecel,
		
		eMoveTypeCount,
	};

	ComponentRootMotion()
		: m_numMoves(0)
		, m_currentMoveProgress(0.0f)
		, m_currentMoveTimer(0.0f)
	{ }
	
	virtual Component::eComponentType GetComponentType() { return Component::eComponentTypeRootMotion; }

	virtual bool Update(float a_dt)
	{
		if (m_numMoves > 0)
		{
			const Move & curMove = m_moves[0];

			// Burn down the move timer
			if (m_currentMoveTimer < curMove.m_startDelay)
			{
				m_currentMoveTimer += a_dt;
			}
			else // Movement has begun
			{
				// Check we haven't reached the destination
				const Vector curPos = m_parentGameObject->GetPos();
				Vector toDest = curMove.m_destination - curPos;
				if (toDest.LengthSquared() > EPSILON)
				{
					m_currentMoveProgress = MathUtils::LerpFloat(m_currentMoveProgress, 1.0f, a_dt);
					m_parentGameObject->SetPos(curPos + (toDest * m_currentMoveProgress));
				}
				else // The move is over
				{
					CancelAllMoves();
				}
			}

			return true;
		}

		return false;
	}
	
	virtual bool Shutdown()
	{
		CancelAllMoves();

		return true;
	}

	inline void QueueMove(const Vector & a_worldPos, float a_moveSpeed, float a_startDelay = 0.0f, eMoveType a_moveType = ComponentRootMotion::eMoveTypeLinear)
	{
		if (m_numMoves == 0)
		{
			Move & curMove = m_moves[0];
			curMove.m_destination = a_worldPos;
			curMove.m_startDelay = a_startDelay;
			++m_numMoves;
		}
	}

	void QueueMoveOverTime(const Vector & a_worldPos, float a_moveTime, float a_startDelay = 0.0f, eMoveType a_moveType = ComponentRootMotion::eMoveTypeLinear)
	{
		if (m_numMoves == 0)
		{
			Move & curMove = m_moves[0];
			curMove.m_destination = a_worldPos;
			curMove.m_startDelay = a_startDelay;
			++m_numMoves;
		}
	}

	void CancelAllMoves(bool a_warpToCurrentDest = false)
	{
		if (a_warpToCurrentDest && m_numMoves > 0 )
		{
			if (m_parentGameObject != NULL)
			{
				m_parentGameObject->SetPos(m_moves[0].m_destination);
			}
		}

		m_moves[0] = Move();
		m_currentMoveTimer = 0.0f;
		m_numMoves = 0;
	}

	bool GetTimeTillDeparture();
	bool GetTimeTillArrival();
	bool IsMoving() { return m_numMoves > 0 && m_currentMoveTimer < m_moves[0].m_startDelay; }
	bool IsMoveQueued();

private:

	//\brief A move records all the important details about where to go and when
	struct Move
	{
		Move()
			: m_startDelay(0)
			, m_moveSpeed(0.0f)
			, m_destination(0.0f) 
			, a_moveOverTime(false) {}
		float m_startDelay;
		union
		{
			float m_moveSpeed;
			float m_moveTime;
		};
		bool a_moveOverTime;
		Vector m_destination;
	};

	static const unsigned int sc_maxMoves = 16;
	unsigned int m_numMoves;
	float m_currentMoveProgress;
	float m_currentMoveTimer;
	Move m_moves[sc_maxMoves];  ///< TODO Implement free list to queue up to 16 moves. At the moment its only the zeroeth element of this array

};

#endif //_ENGINE_COMPONENT_ROOT_MOTION_
