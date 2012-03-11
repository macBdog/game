#ifndef _CORE_SYSTEM_LOG_
#define _CORE_SYSTEM_LOG_
#pragma once

#include <iostream>

#include "engine/eTimer.h"

class cSystemLog
{
public:

    cSystemLog() {};

    enum LogLevel
    {
        LL_INFO = 0,
        LL_WARNING,
        LL_ERROR,

        LL_COUNT
    };

    enum LogCategory
    {
        LC_CORE = 0,
        LC_GAME,

        LC_COUNT
    };

    inline void Write(LogLevel a_level, LogCategory a_category, const char * a_message) const
    {
        char levelBuf[128];
        char categoryBuf[128];
        memset(levelBuf, 0, sizeof(char)*128);
        memset(categoryBuf, 0, sizeof(char)*128);

        switch (a_level)
        {
            case LL_INFO:       sprintf(levelBuf, "%s", "INFO"); break;
            case LL_WARNING:    sprintf(levelBuf, "%s", "WARNING"); break;
            case LL_ERROR:      sprintf(levelBuf, "%s", "ERROR"); break;
            default: break;
        }

        switch (a_category)
        {
            case LC_CORE:       sprintf(categoryBuf, "%s", "CORE"); break;
            case LC_GAME:       sprintf(categoryBuf, "%s", "GAME"); break;
            default: break;
        }
        printf("%u -> %s::%s: %s", eSystemTime::GetSystemTime(), categoryBuf, levelBuf, a_message);
    }

    private:

};

#endif // _CORE_SYSTEM_LOG_
