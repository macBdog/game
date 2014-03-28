#include "InputManager.h"

#include "Log.h"
#include "RenderManager.h"

template<> InputManager * Singleton<InputManager>::s_instance = NULL;

bool InputManager::Startup(bool a_fullScreen)
{
	m_fullScreen = a_fullScreen;

	// Init gamepad pointers
	SDL_JoystickEventState(SDL_ENABLE);
	m_numGamePads = SDL_NumJoysticks();
	for (int i = 0; i < m_numGamePads; ++i)
	{
		m_gamePads[i] = SDL_JoystickOpen(i);
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
	for (int i = 0; i < m_numGamePads; ++i)
	{
		SDL_JoystickClose(m_gamePads[i]);
		m_gamePads[i] = NULL;
	}

	return true;
}

bool InputManager::Update(const SDL_Event & a_event)
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
		case SDL_ACTIVEEVENT:
		{
			if (a_event.active.gain == 0)
			{
				SetFocus(false);
			}
			else
			{
				SetFocus(true);
			}
			break;
		}
		// Resize
		case SDL_VIDEORESIZE:
		{
			renderMan.Resize(a_event.resize.w, a_event.resize.h, renderMan.GetViewDepth());
			break;
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
			m_mousePos.SetX(a_event.motion.x);
			m_mousePos.SetY(a_event.motion.y);
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			// Give focus back to the app
			if (!m_focus && !m_fullScreen)
			{
				SetFocus(true);
			}
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
		SDL_ShowCursor(SDL_DISABLE);
		SDL_WM_GrabInput(SDL_GRAB_ON);
		RenderManager::Get().SetRenderMode(RenderMode::Full);
	}
	else
	{
		m_focus = false;
		SDL_ShowCursor(SDL_ENABLE);
        SDL_WM_GrabInput(SDL_GRAB_OFF);
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

bool InputManager::IsKeyDepressed(SDLKey a_key)
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

bool InputManager::ProcessMouseUp(MouseButton::Enum a_button)
{
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

bool InputManager::ProcessKeyUp(SDLKey a_key)
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

bool InputManager::ProcessKeyDown(SDLKey a_key)
{
	// Set convenience keys
	m_lastKeyPress = a_key;

	// Set depressed keys
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