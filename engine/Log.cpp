#include "Log.h"

template<> Log * Singleton<Log>::s_instance = NULL;

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

void Log::Write(LogLevel::Enum a_level, LogCategory::Enum a_category, const char * a_message, ...)
{
    char levelBuf[128];
    char categoryBuf[128];
	levelBuf[0] = '\0';
	categoryBuf[0] = '\0';
	char * formatString = NULL;
	char * finalString = NULL;

	// Create a preformatted error string
	PrependLogDetails(a_level, a_category, &levelBuf[0]);
	char errorString[StringUtils::s_maxCharsPerLine];
	memset(&errorString, 0, sizeof(char)*StringUtils::s_maxCharsPerLine);
	sprintf(errorString, "%u -> %s::%s:", Time::GetSystemTime(), categoryBuf, levelBuf);

	// Grab all the log arguments passed in the elipsis
	va_list formatArgs;
	va_start(formatArgs, a_message);
	int finalStringSize = _vscprintf(a_message, formatArgs);
	formatString = (char *)malloc(sizeof(char) * finalStringSize + 1);
	finalString = (char *)malloc(sizeof(char) * finalStringSize + strlen(errorString) + 8);
	if (finalString != NULL && formatString != NULL)
	{
		vsprintf(formatString, a_message, formatArgs);
		sprintf(finalString, "%s %s\n", errorString, formatString);
		printf(finalString);
	}
	else // Something is horribly wrong
	{
		printf("FATAL ERROR! Memory allocation failed in Log::Write.");
		return;
	}
	va_end(formatArgs);

	// Also add to the list which is diaplyed on screen
	if (m_renderToScreen)
	{
		// Print a single line to the log
		const unsigned int stringLen = strlen(finalString);
		if (stringLen < StringUtils::s_maxCharsPerLine)
		{
			LogDisplayNode * newLogEntry = new LogDisplayNode();
			newLogEntry->SetData(new LogDisplayEntry(finalString, a_level));
			m_displayList.Insert(newLogEntry);
		}
		else // Split up log entries greater than line length
		{
			for (unsigned int i = 0; i < stringLen / StringUtils::s_maxCharsPerLine + 1; ++i)
			{
				char * pStart = &finalString[i * StringUtils::s_maxCharsPerLine];
				unsigned int endOfLine = (i+1) * StringUtils::s_maxCharsPerLine;
				endOfLine = endOfLine > stringLen ? stringLen : endOfLine;
				char * pEnd = &finalString[(endOfLine)-1];
				*pEnd = '\0';
				LogDisplayNode * newLogEntry = new LogDisplayNode();
				newLogEntry->SetData(new LogDisplayEntry(pStart, a_level));
				m_displayList.Insert(newLogEntry);
			}
		}
	}

	// Free memory for error strings
	if (formatString != NULL)
	{
		free(formatString);
	}
	if (finalString != NULL)
	{
		free(finalString);
	}
}

void Log::WriteOnce(LogLevel::Enum a_level, LogCategory::Enum a_category, const char * a_message, ...)
{
	// Add message to write once list
	unsigned int msgHash = StringHash::GenerateCRC(a_message, false);
	unsigned int unused;
	if (!m_writeOnceList.Get(msgHash, unused))
	{
		m_writeOnceList.Insert(msgHash, msgHash);

		char levelBuf[128];
		char categoryBuf[128];
		memset(levelBuf, 0, sizeof(char)*128);
		memset(categoryBuf, 0, sizeof(char)*128);

		// Create a preformatted error string
		PrependLogDetails(a_level, a_category, &levelBuf[0]);
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
			FontManager::Get().DrawDebugString2D(logEntry->m_message, Vector2(-1.0f, logDisplayPosY), logEntry->m_colour);
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
