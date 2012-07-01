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

    bool Init(bool a_fullScreen = false);
	bool Update(const SDL_Event & a_event);

	//\brief Enable or disable the app for OS focus. This will toggle rendering and input grab
	//\param The new app focus setting
	void SetFocus(bool a_focus);

private:

    bool ProcessKeyDown();
    bool ProcessKeyUp();
    bool ProcessMouseMove();

	bool m_focus;				// If the app currently has OS focus
	bool m_fullScreen;			// If the app is fullscreen, input manager needs to handle focus

};

#endif // _ENGINE_INPUT_MANAGER_
