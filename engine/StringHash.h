#ifndef _CORE_STRING_HASH_
#define _CORE_STRING_HASH_
#pragma once

#include <string>
#include <string_view>
#include <cassert>

#include "StringUtils.h"

//\brief Class that stores a string along with its CRC32 based hash for comparison
//		 All hash generation is lower case only so comparisons will be case insensitive
class StringHash
{
public:

	//\brief Create a Cyclic Redundancy Check value for a string
	static unsigned int GenerateCRC(std::string_view a_string, bool a_convertToLower = true);

	//\brief Create a Cyclic Redundancy Check value for some binary data
	static unsigned int GenerateCRCBinary(const unsigned int * a_binaryData, unsigned int a_length);

	//\brief No arg constructor so a StringHash can be used in array initialisers
	StringHash();

	//\brief Construct the class and copy the string characters to a member
	StringHash(std::string_view a_sourceString);

	//\brief Reset the string and hash
	void SetCString(std::string_view a_newString);

	//\brief Accessors
	inline const char * GetCString() const { return m_string.c_str(); }
	inline std::string_view GetStringView() const { return m_string; }
	inline unsigned int GetHash() const { return m_hash; }
	inline bool IsEmpty() const { return m_hash == 0; }

	//\brief The most useful part of the string hash is the comparison
	bool operator == (const StringHash & a_compare) const { return m_hash == a_compare.m_hash; }
	bool operator == (unsigned int a_compare) const { return m_hash == a_compare; }

private:

	static const unsigned int s_stdCRCTable[256];		// Hash table generated using CRC-32

	unsigned int m_hash;	// Storage for the crc equivalent
	std::string m_string;	// Storage for the original string
};

#endif //_CORE_STRING_HASH
