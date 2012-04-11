#ifndef _CORE_SYSTEM_LOG_
#define _CORE_SYSTEM_LOG_
#pragma once

#include <iostream>
#include <stdarg.h>

#include "Singleton.h"
#include "StringUtils.h"
#include "Time.h"

class Log : public Singleton<Log>
{
public:

    Log() {};

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

    inline void Write(LogLevel a_level, LogCategory a_category, const char * a_message, ...) const
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

		// Create a preformatted error string
		char errorString[StringUtils::s_maxCharsPerLine];
		memset(&errorString, 0, sizeof(char)*StringUtils::s_maxCharsPerLine);
		sprintf(errorString, "%u -> %s::%s:", Time::GetSystemTime(), categoryBuf, levelBuf);

		// Parse the variable number of arguments
		char formatString[StringUtils::s_maxCharsPerLine];
		memset(&formatString, 0, sizeof(char)*StringUtils::s_maxCharsPerLine);

		va_list formatArgs;
		va_start(formatArgs, a_message);
		vsprintf(formatString, a_message, formatArgs);

		// Print out both together
		printf("%s %s", errorString, formatString);

		va_end(formatArgs);
    }
};

#endif // _CORE_SYSTEM_LOG_
