#pragma once

#include <iostream>
#include <stdarg.h>

#include "../core/Colour.h"
#include "../core/HashMap.h"
#include "../core/LinkedList.h"

#include "Singleton.h"
#include "StringUtils.h"
#include "Time.h"

//\brief The importance of the entry being logged
enum class LogLevel : unsigned char
{
	Info = 0,
	Warning,
	Error,
	Count,
};

//\brief What part of the system logged the entry
enum class LogCategory : unsigned char
{
	Engine = 0,
	Game,
	Count,
};

class Log : public Singleton<Log>
{
public:

	// Log does nothing on startup but needs to cleanup
	Log() : m_renderToScreen(true) {};
	~Log() { Shutdown(); }

	//\brief Clean up any allocated memory for log lines still being displayed
	//\return true if all cleanup tasks were successful
	bool Shutdown();

	//\brief Create a log entry that is written to standard out and displayed on screen
	void Write(LogLevel a_level, LogCategory a_category, const char * a_message, ...);
	void WriteOnce(LogLevel a_level, LogCategory a_category, const char * a_message, ...);

	//\brief Some handy overrides for common usage
	inline void WriteEngineErrorNoParams(const char * a_message) { Write(LogLevel::Error, LogCategory::Engine, a_message); }
	inline void WriteGameErrorNoParams(const char * a_message) { Write(LogLevel::Error, LogCategory::Game, a_message); }

	//\brief Update will draw all log entries that need to be displayed
	//\param a_dt float of the time that has passed since the last update call
	void Update(float a_dt);

	//\brief Controls for turning log rendering on and off, useful for errors related to rendering
	inline void DisableRendering() { m_renderToScreen = false; }
	inline void EnableRendering() { m_renderToScreen = true; }
	void ClearRendering();
 
private:
	
	//\brief A log display entry needs it's own properties as it's drawn per line
	struct LogDisplayEntry
	{
		//\brief Set up the basic properties of a log display message
		LogDisplayEntry(const char * a_message, LogLevel a_level)
		{
			strncpy(m_message, a_message, sizeof(char) * strlen(a_message) + 1);
			m_lifeTime = s_logDisplayTime[static_cast<int>(a_level)];
			m_colour = s_logDisplayColour[static_cast<int>(a_level)];
		}

		char m_message[StringUtils::s_maxCharsPerLine];		// What text to display on screen
		Colour m_colour;
		float m_lifeTime;									// How long to display this line
	};

	//\brief Utility function to stringify log constants
	static void PrependLogDetails(LogLevel a_level, LogCategory a_category, char * a_string_OUT)
	{
		switch (a_level)
		{
			case LogLevel::Info:       sprintf(a_string_OUT, "%s", "INFO"); break;
			case LogLevel::Warning:    sprintf(a_string_OUT, "%s", "WARNING"); break;
			case LogLevel::Error:      sprintf(a_string_OUT, "%s", "ERROR"); break;
			default: break;
		}

		switch (a_category)
		{
			case LogCategory::Engine:     sprintf(a_string_OUT, "%s", "ENGINE"); break;
			case LogCategory::Game:       sprintf(a_string_OUT, "%s", "GAME"); break;
			default: break;
		}
	}

	//\brief Shortcuts for creating and mainting a list of display entries
	typedef LinkedListNode<LogDisplayEntry> LogDisplayNode;
	typedef LinkedList<LogDisplayEntry> LogDisplayList;

	static constexpr int s_numLogs = static_cast<int>(LogLevel::Count);
	const static float	s_logDisplayTime[s_numLogs];				///< How long to display each log category on screen
	const static Colour s_logDisplayColour[s_numLogs];				///< What colours to display each log category in

	LogDisplayList m_displayList;									///< All log entries that are being displayed at a time
	HashMap<unsigned int, void *> m_writeOnceList;					///< When a message is logged only once, it's hash is added to this map
	bool m_renderToScreen{ false };									///< If log entries should be rendered to the screen
};
