#include "InputManager.h"

#include "Gui.h"
#include "RenderManager.h"

template<> InputManager * Singleton<InputManager>::s_instance = NULL;

InputManager::InputManager()
{

}

InputManager::~InputManager()
{

}

bool InputManager::Init()
{
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
				renderMan.SetRenderMode(RenderManager::eRenderModeNone);
			}
			else
			{
				renderMan.SetRenderMode(RenderManager::eRenderModeFull);
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
            if (a_event.key.keysym.sym == SDLK_ESCAPE)
            {
                return false;
            }
            break;
        }
		// Mouse events
		case SDL_MOUSEMOTION:
		{
			guiMan.SetMousePos(a_event.motion.x, a_event.motion.y);
			break;
		}
        default: break;
    }

    return true;
}

