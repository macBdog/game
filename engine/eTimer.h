#ifndef _ENGINE_TIMER_
#define _ENGINE_TIMER_
#pragma once

#include <SDL.h>

namespace eSystemTime
{
    static const unsigned int GetSystemTime() { return SDL_GetTicks(); }
};

class eTimer
{
public:

    eTimer()
    : m_active(false)
    , m_expired(false)
    , m_currentTime(0)
    , m_timerLength(0)
    , m_callback(NULL)
    {
    }

    void Update();

    inline void Set(unsigned int a_totalTime) { m_timerLength = a_totalTime; }
    inline void RegisterCallback(void * a_func) { m_callback = a_func; }
    inline void Reset() { m_currentTime = 0; m_expired = false; }
    inline void Expire() { m_active = false; m_expired = true; m_currentTime = m_timerLength; }
    inline void Activate() { m_active = true; }
    inline void Deactivate() { m_active = false; }

private:
        bool m_active;                  // Is the timer currently running down
        bool m_expired;                 // Has the timer been running for timerLength
        unsigned int m_currentTime;     // How far the timer has run down
        unsigned int m_timerLength;     // How long the timer will take to run down
        void * m_callback;              // Function to call when the timer expires
};

#endif // _ENGINE_TIMER_
