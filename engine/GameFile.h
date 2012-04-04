#ifndef _ENGINE_GAME_FILE_
#define _ENGINE_GAME_FILE_
#pragma once

//\brief A GameFile is a general purpose configuration and data file that
//		 is designed to be human readable and lightweight in syntax unlike
//		 XML. There are no types, the data is stored as chars and converted
//		 On the fly. It looks like this:
//		
//		 Object
//		 {
//		   Property: value;
//		   Property2: value;
//		   Property3: multiline
//						value;
//       }
//		 AnotherObject
//		 {
//			Something: value;
//		 }

class GameFile
{

public:
	
	//\ No work done in the constructor, only Init
	GameFile() : m_numObjects(0) {};

	//\brief Load the game file and parse it into data
    bool Load(const char * a_filePath);

	//\brief Accessors to the gamefile property data
	const char * GetString(const char * a_object, const char * a_property);
	int GetInt(const char * a_object, const char * a_property);
	float GetFloat(const char * a_object, const char * a_property);
	bool GetBool(const char * a_object, const char * a_property);

private:
	unsigned int m_numObjects;

};

#endif // _ENGINE_GAME_FILE
