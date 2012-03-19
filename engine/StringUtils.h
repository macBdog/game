#ifndef _ENGINE_STRINGUTILS_H_
#define _ENGING_STRINGUTILS_H_
#pragma once

#include <stdio.h>
#include <string.h>

namespace StringUtils
{
	//\brief Reads from a_buffer looking for the a_fieldIndex instance of a_delim. "Name=Charles" would return "Charles"
	//\return a buffer containing a c string with everything after the delimeter and before the next delimeter
	static const char * ExtractField(const char *a_buffer, const char *a_delim, unsigned int a_fieldIndex);

	//\brief Removes whitespace characters \t \n from a_buffer and returns a modified buffer containing a c string
	static const char * TrimString(const char *a_buffer);

	//\brief Reads from a file until a newline or carriage return is found then returns a c string of the last read
	static const char * ReadLine(FILE *a_filePointer);

	static const unsigned int sc_maxCharsPerLine = 256;		// Maximum number of chars that can be read before a cr
}

#endif /* _ENGINE_STRINGUTILS_H_ */
