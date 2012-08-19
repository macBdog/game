#ifndef _ENGINE_GAME_FILE_
#define _ENGINE_GAME_FILE_
#pragma once

#include <iostream>
#include <fstream>

#include "../core/LinkedList.h"

#include "StringUtils.h"

//\brief A GameFile is a general purpose configuration and data file that
//		 is designed to be human readable and lightweight in syntax unlike
//		 XML. There are no types, the data is stored as chars and converted
//		 On the fly by request. It looks like this:
//		
//		 // A single line comment
//		 Object
//		 {
//		   Property: value;
//		   Property2: value;
//		   Property3: "multiline
//						value";
//			ChildObject
//			{
//				Property: value;
//			}
//       }
//		 AnotherObject
//		 {
//			Something: anotherThing;
//		 }

class GameFile
{
public:
	
	//\brief Each property belonging to an object
	struct Property
	{
		Property()
			: m_name(NULL)
			, m_data(NULL) { }

		char * m_name;
		void * m_data;
	};

	//\brief Each entity in the game file
	struct Object
	{
		Object()
			: m_name(NULL)
			, m_parent(NULL) { }

		char * m_name;						// Literal declared before the open brace
		Object * m_parent;					// Any parent object for nested objects
		LinkedList<Property> m_properties;	// Properties of this object
	};

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

	//\brief Add an object that has properties
	//\param a_objectName is the literal declared on the line preceeding the open bracket
	//\param a_parent is the object's parent in the case of nested objects
	Object * AddObject(const char * a_objectName, Object * a_parent = NULL);

	//\brief Add a property with a parent object
	Property * AddProperty(GameFile::Object * a_parentObject, const char * a_propertyName, const char * a_value);

	//\brief Helper function to find an object by name
	Object * GetObject(const char * a_name);

	//brief Helper function to find a property of an object
	Property * GetProperty(Object * a_parent, const char * a_propertyName);

private:

	//\brief Recursive function to read an object definition with any child objects
	//\return the number of lines read
	unsigned int ReadObjectAndProperties(const char * a_objectName, std::ifstream & a_stream, Object * a_parentObject = NULL);

	//\brief Helper function to determine if a line of text defines a new object
	static inline bool IsLineNewObject(const char * a_line)
	{
		return !strstr(a_line, "{") && !strstr(a_line, "}") && !strstr(a_line, ":") && strlen(a_line) > 0;
	}

	LinkedList<Object> m_objects;							// All the objects in this file
	char m_filePath[StringUtils::s_maxCharsPerLine];		// Cache off the filepath so the file can be reloaded
};

#endif // _ENGINE_GAME_FILE
