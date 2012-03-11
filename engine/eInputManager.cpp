#include "eInputManager.h"

eInputManager::eInputManager()
{

}

eInputManager::~eInputManager()
{

}

bool eInputManager::Init()
{
    return true;
}

bool eInputManager::Update(const SDL_Event & a_event)
{
    switch (a_event.type)
    {
        // Window controls
        case SDL_QUIT:
        {
            return false;
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
        default: break;
    }

    return true;
}

