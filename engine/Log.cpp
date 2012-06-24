#include "Log.h"

template<> Log * Singleton<Log>::s_instance = NULL;

const float	Log::s_logDisplayTime[LL_COUNT] =
{
	1.0f,
	2.0f,
	10.0f
};

const Colour Log::s_logDisplayColour[LL_COUNT]=
{
	sc_colourGreen,
	sc_colourPurple,
	sc_colourRed
};

bool Log::Shutdown()
{
	LogDisplayNode * next = m_displayList.GetHead();
	while(next != NULL)
	{
		// Cache off next pointer
		LogDisplayNode * cur = next;
		next = cur->GetNext();

		m_displayList.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	return true;
}

void Log::Write(LogLevel a_level, LogCategory a_category, const char * a_message, ...)
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
        case LC_ENGINE:       sprintf(categoryBuf, "%s", "ENGINE"); break;
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

	// Grab all the log arguments passed in the elipsis
	va_list formatArgs;
	va_start(formatArgs, a_message);
	vsprintf(formatString, a_message, formatArgs);

	// Print out both together to standard out
	char finalString[StringUtils::s_maxCharsPerLine];
	sprintf(finalString, "%s %s\n", errorString, formatString);
	printf("%s", finalString);
	va_end(formatArgs);

	// Also add to the list which is diaplyed on screen
	LogDisplayNode * newLogEntry = new LogDisplayNode();
	newLogEntry->SetData(new LogDisplayEntry(finalString, a_level));
	m_displayList.Insert(newLogEntry);
}

void Log::Update(float a_dt)
{
	// Walk through the list printing out debug lists
	LogDisplayNode * curEntry = m_displayList.GetHead();
	float logDisplayPosY = 1.0f;
	int logEntryCount = 0;
	while(curEntry != NULL)
	{
		LogDisplayEntry * logEntry = curEntry->GetData();
		LogDisplayNode * toDelete = curEntry;
		curEntry = curEntry->GetNext();

		// Display log entries that are still alive
		if (logEntry->m_lifeTime > 0.0f)
		{
			FontManager::Get().DrawDebugString(logEntry->m_message, Vector2(-1.0f, logDisplayPosY), logEntry->m_colour);
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
