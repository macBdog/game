#ifndef _ENGINE_INPUT_MANAGER_
#define _ENGINE_INPUT_MANAGER_
#pragma once

#include <SDL.h>

#include "Singleton.h"

class InputManager : public Singleton<InputManager>
{

public:

	InputManager();
	~InputManager();

    bool Init();
	bool Update(const SDL_Event & a_event);

private:

    bool ProcessKeyDown();
    bool ProcessKeyUp();
    bool ProcessMouseMove();

};

#endif // _ENGINE_INPUT_MANAGER_
