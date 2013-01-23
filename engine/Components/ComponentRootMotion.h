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
		, m_currentMove(NULL)
	{ }
	
	virtual Component::eComponentType GetComponentType() { return Component::eComponentTypeRootMotion; }

	virtual bool Update(float a_dt)
	{
		if (m_currentMove != NULL)
		{
			// Burn down the move timer
			if (m_currentMoveTimer < m_currentMove->m_startDelay)
			{
				m_currentMoveTimer += a_dt;
			}
			else // Movement has begun
			{
				// Check we haven't reached the destination
				const Vector curPos = m_parentGameObject->GetPos();
				Vector toDest = m_currentMove->m_destination - curPos;
				if (toDest.LengthSquared() > EPSILON)
				{
					m_currentMoveProgress = MathUtils::LerpFloat(m_currentMoveProgress, 1.0f, a_dt/m_currentMove->m_moveSpeed);
					m_parentGameObject->SetPos(curPos + (toDest * m_currentMoveProgress));
				}
				else // The move is over
				{
					EndCurrentMove();
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
		if (Move * newMove = AllocateMove())
		{
			newMove->m_destination = a_worldPos;
			newMove->m_startDelay = a_startDelay;
			newMove->m_moveSpeed = a_moveSpeed;

			// Link new move to the last queued move
			if (m_currentMove != newMove)
			{
				Move * lastMove = m_currentMove;
				while (lastMove != NULL)
				{
					if (lastMove->m_nextMove == NULL)
					{
						lastMove->m_nextMove = newMove;
						break;
					}
					lastMove = lastMove->m_nextMove;
				}
			}
		}
	}

	void QueueMoveOverTime(const Vector & a_worldPos, float a_moveTime, float a_startDelay = 0.0f, eMoveType a_moveType = ComponentRootMotion::eMoveTypeLinear)
	{
		// TODO
	}

	void CancelAllMoves(bool a_warpToFinalDest = false)
	{
		// Remove and free up all moves
		bool foundFinalDest = false;
		Vector finalDest(0.0f);
		Move * curMove = m_currentMove;
		while (curMove != NULL)
		{
			curMove->m_inUse = false;
			if (curMove->m_nextMove == NULL)
			{
				finalDest = curMove->m_destination;
				foundFinalDest = true;
				break;
			}
			curMove = curMove->m_nextMove;
		}

		// Warp the game object to the last location
		if (a_warpToFinalDest && foundFinalDest)
		{
			if (m_parentGameObject != NULL)
			{
				m_parentGameObject->SetPos(finalDest);
			}
		}

		m_currentMoveTimer = 0.0f;
		m_currentMoveProgress = 0.0f;
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
			, m_inUse(false)
			, m_moveOverTime(false)
			, m_nextMove(NULL) {}
		float m_startDelay;
		union
		{
			float m_moveSpeed;
			float m_moveTime;
		};
		bool m_inUse;
		bool m_moveOverTime;
		Vector m_destination;
		Move * m_nextMove;
	};

	Move * AllocateMove()
	{
		if (m_numMoves < sc_maxMoves)
		{
			for (unsigned int i = 0; i < sc_maxMoves; ++i)
			{
				if (!m_moves[i].m_inUse)
				{
					if (m_numMoves == 0)
					{
						m_currentMove = &m_moves[i];
					}
					++m_numMoves;
					m_moves[i].m_inUse = true;
					return &m_moves[i];
				}
			}
		}
		else
		{
			//assert
		}
		return NULL;
	}

	void EndCurrentMove()
	{
		m_numMoves--;
		m_currentMove->m_inUse = false;
		m_currentMove = m_currentMove->m_nextMove;
		m_currentMoveProgress = 0.0f;
		m_currentMoveTimer = 0.0f;
	}

	static const unsigned int sc_maxMoves = 16;
	unsigned int m_numMoves;
	float m_currentMoveProgress;
	float m_currentMoveTimer;
	Move * m_currentMove;
	Move m_moves[sc_maxMoves];						///< Unordered list to queue up to 16 moves
};

#endif //_ENGINE_COMPONENT_ROOT_MOTION_
