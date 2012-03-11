#ifndef _ENGINE_INPUT_MANAGER_
#define _ENGINE_INPUT_MANAGER_
#pragma once

#include <SDL.h>

class eInputManager
{

public:

	eInputManager();
	~eInputManager();

    bool Init();
	bool Update(const SDL_Event & a_event);

private:

    bool ProcessKeyDown();
    bool ProcessKeyUp();
    bool ProcessMouseMove();

};

#endif // _ENGINE_INPUT_MANAGER_
