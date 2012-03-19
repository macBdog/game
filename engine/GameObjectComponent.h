#ifndef _ENGINE_GAME_OBJECT_COMPONENT_
#define _ENGINE_GAME_OBJECT_COMPONENT_
#pragma once

/// \brief eGameObjectComponent is designed to be pure virtual
class GameObjectComponent
{
public:
	virtual GameObjectComponent() = 0;
	virtual ~GameObjectComponent() = 0;

private:
	static const unsigned int m_componentId;

};

#endif //_ENGINE_GAME_OBJECT_COMPONENT_
