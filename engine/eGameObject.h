#ifndef _ENGINE_GAME_OBJECT_
#define _ENGINE_GAME_OBJECT_
#pragma once

#include "core/cVector.h"

class eGameObject
{

public:

	eGameObject(unsigned int a_id)
		: m_id(a_id)
	{}

private:
	unsigned int m_id;
	cVector m_worldPos;

};

#endif // _ENGINE_GAME_OBJECT_
