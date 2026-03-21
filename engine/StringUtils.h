#ifndef _ENGINE_STRINGUTILS_H_
#define _ENGINE_STRINGUTILS_H_
#pragma once

#include <string>
#include <string_view>
#include <cstdio>

class StringUtils
{
public:
	//\brief Reads from a_buffer looking for the a_fieldIndex instance of a_delim
	static std::string ExtractField(std::string_view a_buffer, std::string_view a_delim, unsigned int a_fieldIndex);

	//\brief Reads from a_buffer looking for the first index of a_delim and returns everything up to that delimiter
	static std::string ExtractPropertyName(std::string_view a_buffer, std::string_view a_delim);

	//\brief Reads from a_buffer looking for the first index of a_delim and returns everything after that delimiter
	static std::string ExtractValue(std::string_view a_buffer, std::string_view a_delim);

	//\brief Returns a string that is all characters after the last path separator
	static std::string_view ExtractFileNameFromPath(std::string_view a_buffer);

	//\brief Truncates a string after the last path separator
	static std::string TrimFileNameFromPath(std::string_view a_path);

	//\brief Removes whitespace characters \t \n from a_buffer and returns a modified string
	static std::string TrimString(std::string_view a_buffer, bool a_trimQuotes = false);

	//\brief Reads from a file until a newline or carriage return is found
	static std::string ReadLine(FILE* a_filePointer);

	//\brief Convert a character to its lower case equivalent
	static unsigned char ConvertToLower(unsigned char a_char);

	//\brief Count the number of character occurrences in a string
	static unsigned int CountCharacters(std::string_view a_string, char a_searchChar);

	//\brief Prepend a prefix to a string
	static std::string PrependString(std::string_view a_buffer, std::string_view a_prefix);

	//\brief Append a suffix to a string
	static std::string AppendString(std::string_view a_buffer, std::string_view a_suffix);

	//\brief Check if a path is absolute (e.g. "C:/foo" on Windows, "/foo" on Unix)
	static inline bool IsAbsolutePath(std::string_view a_path)
	{
		if (a_path.empty()) return false;
		// Unix absolute path
		if (a_path[0] == '/') return true;
		// Windows drive letter (e.g. "C:/" or "C:\")
		if (a_path.size() >= 2 && a_path[1] == ':') return true;
		return false;
	}

	//\brief String versions of primitive types for serialisation
	static std::string_view BoolToString(bool a_input);

	static constexpr unsigned int s_maxCharsPerLine = 256u;		///< Maximum number of chars that can be read before a cr
	static constexpr unsigned int s_maxCharsPerName = 64u;		///< Maximum number of chars for a simple name

	static constexpr std::string_view s_charLineEnd = "\n";		///< Char sequence for the end of a line
	static constexpr std::string_view s_charTab = "\t";			///< Char sequence for a tab
	static constexpr std::string_view s_charPathSep = "/";		///< Path separator
};

#endif /* _ENGINE_STRINGUTILS_H_ */
