#include "StringUtils.h"

#include <stdlib.h>

static const char * StringUtils::ExtractField(const char *a_buffer, const char *a_delim, unsigned int a_fieldIndex)
{
	// Early out for bad string or no delim
	if (a_buffer == NULL || 
		strlen(a_buffer) == 0 || 
		!strstr(a_buffer, a_delim))
	{
		return NULL;
	}
   
    // Ignore all delims before the fields
    while (strstr(a_buffer, a_delim) == a_buffer) 
	{
		a_buffer += strlen(a_delim);
    }
    
    // Maintain pointers to the start and end of the buffer
	const char * ptrStart = NULL;
    const char * ptrEnd = NULL;
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
            if (ptrEnd==NULL) 
			{
				break;
			}

            ptrStart = ptrEnd + (sizeof(char)*strlen(a_delim));
		}
 
		if (*ptrStart=='\0' || a_fieldIndex > i) 
		{
			return NULL;
		} 
		else 
		{
			ptrEnd = strstr(ptrStart, a_delim);
            if (ptrEnd==NULL) 
			{
				ptrEnd = strchr(ptrStart, '\0');
			}
		}
	}
 
	// No instance of delim, early out
    if (ptrStart==ptrEnd) {
		return NULL;
    }
            
    // Allocate memory for the new buffer and return
    char * retBuf = (char*)malloc(sizeof(char)*(ptrEnd-ptrStart)+1);
	if (retBuf != NULL)
	{
		// Copy chars from the buffer and terminate the string
		memcpy(retBuf, ptrStart, (sizeof(char)*(ptrEnd-ptrStart)));
		retBuf[ptrEnd-ptrStart] = '\0';
 
		// Trim off any unwanted characters such an newlines
		return TrimString(retBuf);
	}

	return NULL;
}

const char * StringUtils::TrimString(const char * a_buffer) 
{
	// Iterate through the string and remove bad characters
	if (a_buffer != NULL)
	{
		const unsigned int charNewLine = 10;
		const unsigned int stringLength = strlen(a_buffer);

		 // Allocate memory for the new buffer
		char * retBuf = (char*)malloc(sizeof(char)*stringLength+1);
		if (retBuf != NULL)
		{
			// Copy chars from the buffer and terminate the string
			for (unsigned int i = 0; i < stringLength; i++) 
			{
				if (a_buffer[i] != 10) 
				{
					retBuf[i] = a_buffer[i];
				}
			}

			retBuf[stringLength] = '\0';

			return retBuf;
		}
	}

	return NULL;
}

const char * StringUtils::ReadLine(FILE *a_filePointer) 
{
	// Buffer of the max characters per line
	char * retBuf = (char*)malloc(sizeof(char)*sc_maxCharsPerLine+1);
	memset(&retBuf, 0, sizeof(char) * sc_maxCharsPerLine);

    char *findc;
	do {
		fgets(retBuf, sc_maxCharsPerLine, a_filePointer);

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
