#include "InputManager.h"

#include "Gui.h"
#include "RenderManager.h"

template<> InputManager * Singleton<InputManager>::s_instance = NULL;

InputManager::InputManager()
	: m_focus(true)
	, m_fullScreen(false)
{

}

InputManager::~InputManager()
{

}

bool InputManager::Init(bool a_fullScreen)
{
	m_fullScreen = a_fullScreen;
    return true;
}

bool InputManager::Update(const SDL_Event & a_event)
{
	// Cache off rendermanager as it gets used plenty here
	RenderManager & renderMan = RenderManager::Get();
	Gui::GuiManager & guiMan = Gui::GuiManager::Get();

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
            break;
        }
		// Mouse events
		case SDL_MOUSEMOTION:
		{
			guiMan.SetMousePos(a_event.motion.x, a_event.motion.y);
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
		RenderManager::Get().SetRenderMode(RenderManager::eRenderModeFull);
	}
	else
	{
		m_focus = false;
		SDL_ShowCursor(SDL_ENABLE);
        SDL_WM_GrabInput(SDL_GRAB_OFF);
		RenderManager::Get().SetRenderMode(RenderManager::eRenderModeNone);
	}
}

