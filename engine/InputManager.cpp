#include "InputManager.h"

#include "Log.h"
#include "RenderManager.h"

template<> InputManager * Singleton<InputManager>::s_instance = NULL;

const float InputManager::s_gamePadCheckTime = 2.0f;	///< Timer to check for new plugged in gamepads

bool InputManager::Startup(bool a_fullScreen)
{
	m_fullScreen = a_fullScreen;

	// Init gamepad pointers
	SDL_JoystickEventState(SDL_ENABLE);
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	#define	SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS 1
	m_numGamepads = SDL_NumJoysticks();
	for (int i = 0; i < m_numGamepads && i < s_maxGamepads; ++i)
	{
		// Assign controller
		if (m_gamepads[i] = SDL_JoystickOpen(i))
		{
			// Clear all depressed button flags
			for (int j = 0; j < s_maxGamepadButtons; ++j)
			{
				m_depressedGamepadButtons[i][j] = false;
			}
		}
	}

    return true;
}

bool InputManager::Shutdown()
{
	// Clean up any registered events
	InputEventNode * next = m_events.GetHead();
	while(next != NULL)
	{
		// Cache off next pointer
		InputEventNode * cur = next;
		next = cur->GetNext();

		m_events.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	// Close gamepads down
	int connectedPads = SDL_NumJoysticks();
	for (int i = 0; i < s_maxGamepads; ++i)
	{
		if (m_gamepads[i])
		{
			SDL_JoystickClose(m_gamepads[i]);
		}
		m_gamepads[i] = NULL;
	}

	return true;
}

bool InputManager::Update(float a_dt)
{
	// Check for new gamepads and enable them
	m_gamePadCheckTimer += a_dt;
	if (m_gamePadCheckTimer >= s_gamePadCheckTime)
	{
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		SDL_InitSubSystem(SDL_INIT_JOYSTICK);
		const int numGamePads = SDL_NumJoysticks();
		if (numGamePads > m_numGamepads)
		{
			m_numGamepads = numGamePads;
			for (int i = 0; i < m_numGamepads; ++i)
			{
				if (m_gamepads[i] == NULL)
				{
					if (m_gamepads[i] = SDL_JoystickOpen(i))
					{
						for (int j = 0; j < s_maxGamepadButtons; ++j)
						{
							m_depressedGamepadButtons[i][j] = false;
						}
					}
				}
			}
		}
		m_gamePadCheckTimer = 0.0f;
	}

	return true;
}

bool InputManager::EventPump(const SDL_Event & a_event)
{
	// Cache off rendermanager as it gets used plenty here
	RenderManager & renderMan = RenderManager::Get();

	// Deal with each different event type
    switch (a_event.type)
    {
        // Window controls
        case SDL_QUIT:
        {
            return false;
        }
		// Lost focus
		case SDL_APP_DIDENTERBACKGROUND:
		{
			SetFocus(false);
			break;
		}
		// Gained focus
		case SDL_APP_DIDENTERFOREGROUND:
		{
			SetFocus(true);
			break;
		}
		case SDL_WINDOWEVENT:
		{
			switch (a_event.window.event)
			{
				// Window closed
				case SDL_WINDOWEVENT_CLOSE:
				{
					return false;
				}
				// Resize
				case SDL_WINDOWEVENT_RESIZED:
				{
					renderMan.Resize(a_event.window.data1, a_event.window.data2, renderMan.GetViewDepth());
					break;
				}
				case SDL_WINDOWEVENT_MINIMIZED:
				{
					SetFocus(false);
					break;
				}
				case SDL_WINDOWEVENT_MAXIMIZED:
				{
					SetFocus(true);
					break;
				}
				case SDL_WINDOWEVENT_RESTORED:
				{
					SetFocus(true);
					break;
				}
				default: break;
			}
		}
        // Keypresses
        case SDL_KEYDOWN:
        {
			// Escape will release windowed focus
            if (m_focus && !m_fullScreen && a_event.key.keysym.sym == SDLK_ESCAPE)
            {
				SetFocus(false);
            }
			ProcessKeyDown(a_event.key.keysym.sym);
            break;
        }
		case SDL_KEYUP:
        {
			ProcessKeyUp(a_event.key.keysym.sym);
            break;
        }
		// Mouse events
		case SDL_MOUSEMOTION:
		{
			m_mousePos.SetX((float) a_event.motion.x);
			m_mousePos.SetY((float) a_event.motion.y);
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			// Give focus back to the app
			if (!m_focus && !m_fullScreen)
			{
				SetFocus(true);
			}

			// Convert SDL button to internal enum
			MouseButton::Enum but = MouseButton::Left;
			switch (a_event.button.button)
			{
				case SDL_BUTTON_LEFT:	but = MouseButton::Left; break;
				case SDL_BUTTON_MIDDLE: but = MouseButton::Middle; break;
				case SDL_BUTTON_RIGHT:  but = MouseButton::Right; break;
				default: break;
			}
			ProcessMouseDown(but);
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			// Convert SDL button to internal enum
			MouseButton::Enum but = MouseButton::Left;
			switch (a_event.button.button)
			{
				case SDL_BUTTON_LEFT:	but = MouseButton::Left; break;
				case SDL_BUTTON_MIDDLE: but = MouseButton::Middle; break;
				case SDL_BUTTON_RIGHT:  but = MouseButton::Right; break;
				default: break;
			}
			ProcessMouseUp(but);
			break;
		}
		case SDL_JOYBUTTONDOWN:
		{
			ProcessGamepadButtonDown(a_event.jbutton.which, a_event.jbutton.button);
			break;
		}
		case SDL_JOYBUTTONUP:
		{
			ProcessGamepadButtonUp(a_event.jbutton.which, a_event.jbutton.button);
			break;
		}
        default: break;
    }

    return true;
}

void InputManager::SetFocus(bool a_focus)
{
	// Toggle cursor, input grab and rendering
	if (a_focus)
	{
		m_focus = true;
		SDL_SetRelativeMouseMode(SDL_TRUE);
		RenderManager::Get().SetRenderMode(RenderMode::Full);
	}
	else
	{
		m_focus = false;
		SDL_SetRelativeMouseMode(SDL_FALSE);
		RenderManager::Get().SetRenderMode(RenderMode::None);
	}
}

Vector2 InputManager::GetMousePosRelative()
{
	// Convert from screen space to container space
	RenderManager & renderMan = RenderManager::Get();
	float relX = m_mousePos.GetX() / renderMan.GetViewWidth();
	float relY = m_mousePos.GetY() / renderMan.GetViewHeight();
	return Vector2(relX*2.0f-1.0f, 1.0f-relY*2.0f);
}

void InputManager::SetMousePosRelative(const Vector2 & a_newPos)
{
	// Convert from container to screen space
	RenderManager & renderMan = RenderManager::Get();
	float absX = (a_newPos.GetX()+1.0f) * (renderMan.GetViewWidth()*0.5f);
	float absY = (1.0f-a_newPos.GetY()) * (renderMan.GetViewHeight()*0.5f);
	m_mousePos = Vector2(absX, absY);
}

bool InputManager::IsKeyDepressed(SDL_Keycode a_key)
{
	// Check all keys in the list
	for (unsigned int i = 0; i < s_maxDepressedKeys; ++i)
	{
		if (m_depressedKeys[i] == a_key)
		{
			return true;
		}
	}

	return false;
}

bool InputManager::ProcessMouseDown(MouseButton::Enum a_button)
{
	// Nothing is done here except set the depressed button list that the script manager accesses
	m_depressedMouseButtons[a_button] = true;
	return true;
}

bool InputManager::ProcessMouseUp(MouseButton::Enum a_button)
{
	m_depressedMouseButtons[a_button] = false;

	if (!DebugMenu::Get().IsDebugMenuEnabled() && !m_mouseEnabled)
	{
		return false;
	}

	bool foundEvent = false;
	InputEventNode * curEvent = m_events.GetHead();
	while(curEvent != NULL)
	{
		// Check the details of the event
		InputEvent * ev = curEvent->GetData();
		if (ev->m_type == InputType::MouseUp && 
			ev->m_src.m_mouseButton == a_button)
		{
			ev->m_delegate.Execute(true);

			// Remove event if set to one shot
			if (ev->m_oneShot)
			{
				// TODO!
				// UnregisterEvent(ev);
			}
			foundEvent = true;
		}

		curEvent = curEvent->GetNext();
	}

	return foundEvent;
}

bool InputManager::ProcessKeyUp(SDL_Keycode a_key)
{
	// Set convenience keys
	m_lastKeyRelease = a_key;
	
	// Release depressed keys
	bool clearedDepressedKey = false;
	for (unsigned int i = 0; i < s_maxDepressedKeys; ++i)
	{
		if (m_depressedKeys[i] == a_key)
		{
			m_depressedKeys[i] = SDLK_UNKNOWN;
			clearedDepressedKey = true;
			break;
		}
	}
	if (!clearedDepressedKey)
	{
		Log::Get().Write(LogLevel::Warning, LogCategory::Engine, "InputManager cannot correctly release the list of depressed keys, are there more than %d keys down?", s_maxDepressedKeys);
	}

	// Process the global callbacks
	if (m_alphaKeysUp.m_type == InputType::KeyUp)
	{
		m_alphaKeysUp.m_delegate.Execute(false);
	}

	bool foundEvent = false;
	InputEventNode * curEvent = m_events.GetHead();
	while(curEvent != NULL)
	{
		// Check the details of the event
		InputEvent * ev = curEvent->GetData();
		if (ev->m_type == InputType::KeyUp && 
			ev->m_src.m_key == a_key)
		{
			ev->m_delegate.Execute(false);

			// Remove event if set to one shot
			if (ev->m_oneShot)
			{
				// TODO!
				// UnregisterEvent(ev);
			}
			foundEvent = true;
		}

		curEvent = curEvent->GetNext();
	}

	return foundEvent;
}

bool InputManager::ProcessKeyDown(SDL_Keycode a_key)
{
	// Set convenience keys
	m_lastKeyPress = a_key;

	// Set depressed keys
	bool keyAlreadyDown = false;
	for (unsigned int i = 0; i < s_maxDepressedKeys; ++i)
	{
		if (m_depressedKeys[i] == a_key)
		{
			keyAlreadyDown = true;
		}
	}
	if (!keyAlreadyDown)
	{
		bool setDepressedKey = false;
		for (unsigned int i = 0; i < s_maxDepressedKeys; ++i)
		{
			if (m_depressedKeys[i] == SDLK_UNKNOWN)
			{
				m_depressedKeys[i] = a_key;
				setDepressedKey = true;
				break;
			}
		}
		if (!setDepressedKey)
		{
			Log::Get().Write(LogLevel::Warning, LogCategory::Engine, "InputManager cannot correctly maintain the list of depressed keys, are there more than %d keys down?", s_maxDepressedKeys);
		}
	}

	// Process the global callbacks
	if (m_alphaKeysDown.m_type == InputType::KeyDown)
	{
		m_alphaKeysDown.m_delegate.Execute(true);
	}

	InputEventNode * curEvent = m_events.GetHead();
	while(curEvent != NULL)
	{
		// Check the details of the event
		InputEvent * ev = curEvent->GetData();
		if (ev->m_type == InputType::KeyDown && 
			ev->m_src.m_key == a_key)
		{
			ev->m_delegate.Execute(true);

			// Remove event if set to one shot
			if (ev->m_oneShot)
			{
				// TODO!
				// UnregisterEvent(ev);
			}
			return true;
		}

		curEvent = curEvent->GetNext();
	}

	return false;
}

bool InputManager::ProcessGamepadButtonDown(int a_gamepadId, int a_buttonId)
{
	if (a_gamepadId < s_maxGamepads && a_buttonId < s_maxGamepadButtons)
	{
		m_depressedGamepadButtons[a_gamepadId][a_buttonId] = true;
		return true;
	}
	return false;
}

bool InputManager::ProcessGamepadButtonUp(int a_gamepadId, int a_buttonId)
{
	if (a_gamepadId < s_maxGamepads && a_buttonId < s_maxGamepadButtons)
	{
		m_depressedGamepadButtons[a_gamepadId][a_buttonId] = false;
		return true;
	}
	return false;
}
