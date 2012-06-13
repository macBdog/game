#ifndef _CORE_SYSTEM_LOG_
#define _CORE_SYSTEM_LOG_
#pragma once

#include <iostream>
#include <stdarg.h>

#include "../core/Colour.h"
#include "../core/LinkedList.h"

#include "FontManager.h"
#include "Singleton.h"
#include "StringUtils.h"
#include "Time.h"

class Log : public Singleton<Log>
{
public:

	//\brief The importance of the entry being logged
    enum LogLevel
    {
        LL_INFO = 0,
        LL_WARNING,
        LL_ERROR,

        LL_COUNT
    };

	//\brief What part of the system logged the entry
    enum LogCategory
    {
        LC_CORE = 0,
        LC_GAME,

        LC_COUNT
    };

	// Log does nothing on startup but needs to cleanup
	Log() {};
	~Log() { Shutdown(); }

	//\brief Clean up any allocated memory for log lines still being displayed
	//\return true if all cleanup tasks were successful
	bool Shutdown();

	//\brief Create a log entry that is written to standard out and displayed on screen
    void Write(LogLevel a_level, LogCategory a_category, const char * a_message, ...);

	//\brief Update will draw all log entries that need to be displayed
	//\param a_dt float of the time that has passed since the last update call
	void Update(float a_dt);
 
private:
	
	//\brief A log display entry needs it's own properties as it's drawn per line
	struct LogDisplayEntry
	{
		//\brief Set up the basic properties of a log display message
		LogDisplayEntry(const char * a_message, LogLevel a_level)
		{
			strncpy(m_message, a_message, strlen(a_message));
			m_lifeTime = s_logDisplayTime[a_level];
			m_colour = s_logDisplayColour[a_level];
		}

		char m_message[StringUtils::s_maxCharsPerLine];		// What text to display on screen
		Colour m_colour;
		float m_lifeTime;									// How long to display this line
	};

	//\brief Shortcuts for creating and mainting a list of display entries
	typedef LinkedListNode<LogDisplayEntry> LogDisplayNode;
	typedef LinkedList<LogDisplayEntry> LogDisplayList;

	const static float	s_logDisplayTime[LL_COUNT];		// How long to display each log category on screen
	const static Colour s_logDisplayColour[LL_COUNT];	// What colours to display each log category in

	LogDisplayList m_displayList;	// All log entries that are being displayed at a time
};

#endif // _CORE_SYSTEM_LOG_
