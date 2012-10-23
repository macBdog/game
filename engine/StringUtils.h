#ifndef _ENGINE_STRINGUTILS_H_
#define _ENGING_STRINGUTILS_H_
#pragma once

#include <stdio.h>

// Allocate a buffer for a cstring and zero it out
#define ALLOC_CSTRING(name, size)						\
	char * name = (char*)malloc(sizeof(char)*size+1);	\
	memset(&name, 0, sizeof(char) * size);

// Allocate a buffer for a string from an existing cstring
// and copy the data from the existing cstring to the new
#define ALLOC_CSTRING_COPY(dst, src)					\
	dst = (char*)malloc(sizeof(char)*strlen(src)+1);	\
	memcpy(dst, src, strlen(src)+1);						

namespace StringUtils
{
	//\brief Reads from a_buffer looking for the a_fieldIndex instance of a_delim. "Name=Charles" would return "Charles"
	//\param a_buffer pointer to a cstring to read from and parse
	//\param a_delim pointer to a cstring containing the delimiter to search for
	//\return a buffer containing a c string with everything after the delimeter and before the next delimeter
	extern const char * ExtractField(const char * a_buffer, const char * a_delim, unsigned int a_fieldIndex);

	//\brief Reads from a_buffer looking for the first index of a_delim and returns everything up to that delimiter
	//\param a_buffer pointer to a cstring to read from and parse
	//\param a_delim pointer to a cstring containing the delimiter to search for
	//\return a buffer containing a c string with everything before the delimeter
	extern const char * ExtractPropertyName(const char * a_buffer, const char * a_delim);

	//\brief Reads from a_buffer looking for the first index of a_delim and returns everything after that delimiter
	//\param a_buffer pointer to a cstring to read from and parse
	//\param a_delim pointer to a cstring containing the delimiter to search for
	//\return a buffer containing a c string with everything after the delimeter
	extern const char * ExtractValue(const char * a_buffer, const char * a_delim);

	//\brief Returns a string that is all characters after the last backslash in a string
	//\param a_buffer pointer to a cstring to work from
	//\return a c string pointing to the start of the just the filename from the buffer
	extern const char * ExtractFileNameFromPath(const char * a_buffer);

	//\brief Terminates a string after the last backslash 
	//\param a_buffer pointer to a cstring to modify
	extern void TrimFileNameFromPath(char * a_buffer_OUT);

	//\brief Removes whitespace characters \t \n from a_buffer and returns a modified buffer containing a c string
	//\param a_buffer pointer to a cstring to read from and parse
	//\param a_trimQuotes to optioanlly also strip all quote chars from the string
	extern const char * TrimString(const char * a_buffer, bool a_trimQuotes = false);

	//\brief Reads from a file until a newline or carriage return is found then returns a c string of the last read
	extern const char * ReadLine(FILE * a_filePointer);

	//\brief Convert a character to it's lower case equivalent
	//\param a_char the character to downcase
	//\return the lower case equivalent char
	extern unsigned char ConvertToLower(unsigned char a_char);

	//\brief Modify a c string argument by adding on another string in front of it
	//\param a_buffer is the c string to modify
	//\param a_prefix is the string add to the start
	//\return true if the string was moodified
	extern bool PrependString(char * a_buffer_OUT, const char * a_prefix);

	//\brief Modify a c string argument by adding on another string at the end of it
	//\param a_buffer is the c string to modify
	//\param a_prefix is the string add to the end
	//\return true if the string was moodified
	extern bool AppendString(char * a_buffer_OUT, const char * a_suffix);

	static const unsigned int s_maxCharsPerLine = 256u;		// Maximum number of chars that can be read before a cr
	static const unsigned int s_maxCharsPerName = 64u;		// Maximum number of chars for a simple name
}

#endif /* _ENGINE_STRINGUTILS_H_ */
