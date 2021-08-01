#include "StringUtils.h"

#include <string.h>
#include <stdlib.h>

const char * StringUtils::s_charLineEnd = "\n";				// Char sequence for the end of a line
const char * StringUtils::s_charTab = "\t";					// Char sequence for a tab
const char * StringUtils::s_charPathSep = "\\";				// Path seperator in Win32

const char * StringUtils::ExtractField(const char *a_buffer, const char *a_delim, unsigned int a_fieldIndex)
{
	// Early out for bad string or no delim
	if (a_buffer == nullptr || 
		strlen(a_buffer) == 0 || 
		!strstr(a_buffer, a_delim))
	{
		return nullptr;
	}
   
    // Ignore all delims before the fields
    while (strstr(a_buffer, a_delim) == a_buffer) 
	{
		a_buffer += strlen(a_delim);
    }
    
    // Maintain pointers to the start and end of the buffer
	const char * ptrStart = nullptr;
    const char * ptrEnd = nullptr;
    unsigned int i = 0;
    ptrStart = ptrEnd = a_buffer;
    
    if (a_fieldIndex==0) 
	{
		ptrEnd = strstr(ptrStart, a_delim);
	} 
	else 
	{
		// Move the start and end pointers to the delimeter
		for (i=0; i<a_fieldIndex; i++) 
		{
			ptrEnd = strstr(ptrStart, a_delim);
            if (ptrEnd==nullptr) 
			{
				break;
			}

            ptrStart = ptrEnd + (sizeof(char)*strlen(a_delim));
		}
 
		if (*ptrStart=='\0' || a_fieldIndex > i) 
		{
			return nullptr;
		} 
		else 
		{
			ptrEnd = strstr(ptrStart, a_delim);
            if (ptrEnd==nullptr) 
			{
				ptrEnd = strchr(ptrStart, '\0');
			}
		}
	}
 
	// No instance of delim, early out
    if (ptrStart==ptrEnd) {
		return nullptr;
    }
            
    // Allocate memory for the new buffer and return
    char * retBuf = (char*)malloc(sizeof(char)*(ptrEnd-ptrStart)+1);
	if (retBuf != nullptr)
	{
		// Copy chars from the buffer and terminate the string
		memcpy(retBuf, ptrStart, (sizeof(char)*(ptrEnd-ptrStart)));
		retBuf[ptrEnd-ptrStart] = '\0';
 
		// Trim off any unwanted characters such an newlines and quotes
		return TrimString(retBuf, true);
	}

	// Failure case
	return nullptr;
}

const char * StringUtils::ExtractPropertyName(const char *a_buffer, const char *a_delim)
{
	// Early out for bad string or no delim
	const char * delimLoc = strstr(a_buffer, a_delim);
	if (a_buffer == nullptr || 
		strlen(a_buffer) == 0 || 
		delimLoc == nullptr)
	{
		return nullptr;
	}
   
    // Alloc a new string and return it
	unsigned int propLength = delimLoc - a_buffer;
	char * retBuf = (char*)malloc(sizeof(char)*propLength+1);
	if (retBuf != nullptr)
	{
		// Copy all chars up to delim and null terminate
		memset(retBuf, 0, sizeof(char) * propLength);
		memcpy(retBuf, a_buffer, propLength);
		retBuf[propLength] = '\0';
		return TrimString(retBuf);
    }
	
	// Failure case
	return nullptr;
}

const char * StringUtils::ExtractValue(const char *a_buffer, const char *a_delim)
{
	// Early out for bad string or no delim
	const char * delimLoc = strstr(a_buffer, a_delim);
	if (a_buffer == nullptr || 
		strlen(a_buffer) == 0 || 
		delimLoc == nullptr)
	{
		return nullptr;
	}
   
	// Return everything after the delimeter
	return TrimString(delimLoc+1);
}

const char * StringUtils::ExtractFileNameFromPath(const char * a_buffer)
{
	unsigned int  bufLength = strlen(a_buffer);
	if (bufLength > 0)
	{
		for (unsigned int i = bufLength; i > 0; --i)
		{
			if (a_buffer[i] == '\\')
			{
				return &a_buffer[i+1];
			}
		}
	}

	return nullptr;
}

void StringUtils::TrimFileNameFromPath(char * a_buffer_OUT)
{
	unsigned int  bufLength = strlen(a_buffer_OUT);
	if (a_buffer_OUT > 0)
	{
		// Work backwards through the string until a slash is encountered
		for (unsigned int i = bufLength; i > 0; --i)
		{
			if (a_buffer_OUT[i] == '\\')
			{
				// Terminate the string at the slash
				a_buffer_OUT[i+1] = '\0';
				return;
			}
		}
	}
}


const char * StringUtils::TrimString(const char * a_buffer, bool a_trimQuotes) 
{
	// Iterate through the string and remove bad characters
	if (a_buffer != nullptr)
	{
		const unsigned int stringLength = strlen(a_buffer);

		 // Allocate memory for the new buffer
		char * retBuf = (char*)malloc(sizeof(char)*stringLength+2);
		if (retBuf != nullptr)
		{
			// Copy chars from the buffer and terminate the string
			unsigned int nonNullChars = 0;
			for (unsigned int i = 0; i < stringLength; i++) 
			{
				// Remove all whitespace characters
				if (a_buffer[i] != 10 &&	// 
					a_buffer[i] != 32 &&	// 
					a_buffer[i] != 9 &&		// Tab?
					a_buffer[i] != ';')
				{
					// And quotes if requested
					if (!(a_trimQuotes && a_buffer[i] == '"'))
					{
						retBuf[nonNullChars++] = a_buffer[i];
					}
				}
			}

			retBuf[nonNullChars] = '\0';

			return retBuf;
		}
	}

	return nullptr;
}

const char * StringUtils::ReadLine(FILE *a_filePointer) 
{
	// Buffer of the max characters per line
	char * retBuf = (char*)malloc(sizeof(char)*s_maxCharsPerLine+1);
	memset(&retBuf, 0, sizeof(char) * s_maxCharsPerLine);

    char *findc;
	do {
		fgets(retBuf, s_maxCharsPerLine, a_filePointer);

		// Early out for bad file
        if (feof(a_filePointer)) 
		{
            retBuf[0] = 0;
            return retBuf;
        }
	} while ((retBuf[0] == '/') || (retBuf[0] == '\n') || (retBuf[0] == '\r'));

	// Terminate the string
    findc = strchr(retBuf, '\r');
    if (findc)
	{
        *findc = 0;
	}
   
	return retBuf;										
}

unsigned char StringUtils::ConvertToLower(unsigned char a_char)
{
	// TODO!
	return a_char;
}

unsigned int StringUtils::CountCharacters(const char * a_string, const char & a_searchChar)
{
	// Early out for bad input
	if (a_string == nullptr || a_string[0] == '\0')
	{
		return 0;
	}

	// O(n) search through each character
	unsigned int numOccurances = 0;
	const unsigned int strLen = strlen(a_string);
	for (unsigned int i = 0; i < strLen; ++i)
	{
		if (a_string[i] == a_searchChar)
		{
			++numOccurances;
		}
	}
	return numOccurances;
}

bool StringUtils::PrependString(char * a_buffer_OUT, const char * a_prefix)
{
	char tempString[s_maxCharsPerLine];
	if (strlen(a_buffer_OUT) < s_maxCharsPerLine)
	{
		strncpy(tempString, a_buffer_OUT, s_maxCharsPerLine);
		sprintf(a_buffer_OUT, "%s%s", a_prefix, tempString);
		return true;
	}

	return false;
}

bool StringUtils::AppendString(char * a_buffer_OUT, const char * a_suffix)
{
	char tempString[s_maxCharsPerLine];
	if (strlen(a_buffer_OUT) < s_maxCharsPerLine)
	{
		strncpy(tempString, a_buffer_OUT, s_maxCharsPerLine);
		sprintf(a_buffer_OUT, "%s%s", tempString, a_suffix);
		return true;
	}

	return false;
}

const char * StringUtils::BoolToString(bool a_input)
{
	return a_input ? "true" : "false";
}
