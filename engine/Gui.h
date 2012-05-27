#ifndef _ENGINE_GAME_FILE_
#define _ENGINE_GAME_FILE_
#pragma once

#include "../core/LinkedList.h"

#include "StringUtils.h"

//\brief A GameFile is a general purpose configuration and data file that
//		 is designed to be human readable and lightweight in syntax unlike
//		 XML. There are no types, the data is stored as chars and converted
//		 On the fly. It looks like this:
//		
//		 Object
//		 {
//		   Property: value;
//		   Property2: value;
//		   Property3: "multiline
//						value";
//       }
//		 AnotherObject
//		 {
//			Something: value;
//		 }

class GameFile
{
public:
	
	//\ No work done in the constructor, only Init
	GameFile() {};
	~GameFile() { Reset(); }

	//\brief Load the game file and parse it into data
    bool Load(const char * a_filePath);
	void Reset();
	inline bool Reload() { Reset(); return Load(m_filePath); }
	inline bool IsLoaded() { return m_objects.GetLength() > 0; }

	//\brief Accessors to the gamefile property data
	const char * GetString(const char * a_object, const char * a_property);
	int GetInt(const char * a_object, const char * a_property);
	float GetFloat(const char * a_object, const char * a_property);
	bool GetBool(const char * a_object, const char * a_property);

private:

	//\brief Each property belonging to an object
	struct Property
	{
		char * m_name;
		void * m_data;
	};

	//\brief Each entity in the game file
	struct Object
	{
		char * m_name;
		LinkedList<Property> m_properties;
	};

	//\brief Add an object that has properties
	Object * AddObject(const char * a_objectName);

	//\brief Add a property with a parent object
	Property * AddProperty(GameFile::Object * a_parentObject, const char * a_propertyName, const char * a_value);

	//\brief Helper function to find an object by name
	Object * GetObject(const char * a_name);

	//brief Helper function to find a property of an object
	Property * GetProperty(Object * a_parent, const char * a_propertyName);

	LinkedList<Object> m_objects;							// All the objects in this file
	char m_filePath[StringUtils::s_maxCharsPerLine];		// Cache off the filepath so the file can be reloaded
};

#endif // _ENGINE_GAME_FILE
