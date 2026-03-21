#include "FontManager.h"

#include "Log.h"

template<> Log * Singleton<Log>::s_instance = nullptr;

const float	Log::s_logDisplayTime[LogLevel::Count] =
{
	1.0f,	// INFO
	2.0f,	// WARNING
	9.0f	// ERROR
};

const Colour Log::s_logDisplayColour[LogLevel::Count]=
{
	sc_colourGreen,
	sc_colourPurple,
	sc_colourRed
};

bool Log::Shutdown()
{
	m_displayList.ForeachAndDelete([&](auto cur)
	{
		m_displayList.Remove(cur);
	});
	return true;
}

void Log::Write(LogLevel a_level, LogCategory a_category, const char * a_message, ...)
{
#ifdef _RELEASE
	return;
#endif

	// Create a preformatted error string
	char errorString[StringUtils::s_maxCharsPerLine];
	snprintf(errorString, sizeof(errorString), "%u -> %.*s::%.*s:",
		Time::GetSystemTime(),
		(int)GetLogCategoryString(a_category).size(), GetLogCategoryString(a_category).data(),
		(int)GetLogLevelString(a_level).size(), GetLogLevelString(a_level).data());

	// Grab all the log arguments passed in the ellipsis
	va_list formatArgs;
	va_start(formatArgs, a_message);
	int needed = _vscprintf(a_message, formatArgs);
	std::string formatString(needed, '\0');
	vsprintf(formatString.data(), a_message, formatArgs);
	va_end(formatArgs);

	std::string finalString = std::string(errorString) + " " + formatString + "\n";
	printf("%s", finalString.c_str());

	// Also add to the list which is displayed on screen
	if (m_renderToScreen)
	{
		if (finalString.size() < StringUtils::s_maxCharsPerLine)
		{
			LogDisplayNode * newLogEntry = new LogDisplayNode();
			newLogEntry->SetData(new LogDisplayEntry(finalString, a_level));
			m_displayList.Insert(newLogEntry);
		}
		else // Split up log entries greater than line length
		{
			for (unsigned int i = 0; i < finalString.size() / StringUtils::s_maxCharsPerLine + 1; ++i)
			{
				unsigned int start = i * StringUtils::s_maxCharsPerLine;
				unsigned int end = (i + 1) * StringUtils::s_maxCharsPerLine;
				if (end > finalString.size()) end = (unsigned int)finalString.size();
				std::string_view chunk(finalString.data() + start, end - start);
				LogDisplayNode * newLogEntry = new LogDisplayNode();
				newLogEntry->SetData(new LogDisplayEntry(chunk, a_level));
				m_displayList.Insert(newLogEntry);
			}
		}
	}
}

void Log::WriteOnce(LogLevel a_level, LogCategory a_category, const char * a_message, ...)
{
#ifdef _RELEASE
	return;
#endif

	// Add message to write once list
	void * tempVal = nullptr;
	unsigned int msgHash = StringHash::GenerateCRC(a_message, false);
	if (!m_writeOnceList.Get(msgHash, tempVal))
	{
		m_writeOnceList.Insert(msgHash, tempVal);

		// Create a preformatted error string
		char errorString[StringUtils::s_maxCharsPerLine];
		snprintf(errorString, sizeof(errorString), "%u -> %.*s::%.*s:",
			Time::GetSystemTime(),
			(int)GetLogCategoryString(a_category).size(), GetLogCategoryString(a_category).data(),
			(int)GetLogLevelString(a_level).size(), GetLogLevelString(a_level).data());

		// Grab all the log arguments passed in the ellipsis
		va_list formatArgs;
		va_start(formatArgs, a_message);
		char formatString[StringUtils::s_maxCharsPerLine];
		vsprintf(formatString, a_message, formatArgs);
		va_end(formatArgs);

		std::string finalString = std::string(errorString) + " " + formatString + "\n";
		printf("%s", finalString.c_str());

		// Also add to the list which is displayed on screen
		if (m_renderToScreen)
		{
			LogDisplayNode * newLogEntry = new LogDisplayNode();
			newLogEntry->SetData(new LogDisplayEntry(finalString, a_level));
			m_displayList.Insert(newLogEntry);
		}
	}
}

void Log::Update(float a_dt)
{
#ifdef _RELEASE
	return;
#endif

	// Walk through the list printing out debug lists
	LogDisplayNode * curEntry = m_displayList.GetHead();
	float logDisplayPosY = 1.0f;
	int logEntryCount = 0;
	while(curEntry != nullptr)
	{
		LogDisplayEntry * logEntry = curEntry->GetData();
		LogDisplayNode * toDelete = curEntry;
		curEntry = curEntry->GetNext();

		// Display log entries that are still alive
		if (logEntry->m_lifeTime > 0.0f)
		{
			FontManager::Get().DrawDebugString2D(logEntry->m_message.c_str(), Vector2(-1.0f, logDisplayPosY), logEntry->m_colour);
			if (logEntryCount == 0)
			{
				logEntry->m_lifeTime -= a_dt;
			}
			logDisplayPosY -= 0.04f;
			++logEntryCount;
		}
		else // This log entry is dead, remove it
		{
			delete logEntry;
			m_displayList.Remove(toDelete);
			delete toDelete;
		}
	}
}

void Log::ClearRendering()
{
	// Delete all display entries
	m_displayList.ForeachAndDelete([&](auto curEntry)
	{
		LogDisplayEntry * logEntry = curEntry->GetData();
		LogDisplayNode * toDelete = curEntry;
		m_displayList.Remove(toDelete);
	});
}
