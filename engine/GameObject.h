#ifndef _ENGINE_GAME_OBJECT_
#define _ENGINE_GAME_OBJECT_
#pragma once

#include "core/Vector.h"

class GameObject
{

public:

	GameObject(unsigned int a_id)
		: m_id(a_id)
	{}

private:
	unsigned int		  m_id;				// Unique identifier, objects can be resolved from ids
	GameObjectComponent * m_components;		// Purpose built object features live in a list of components
	Vector			      m_worldPos;		// Position in the world, will be replaced by a matrix

};

#endif // _ENGINE_GAME_OBJECT_
