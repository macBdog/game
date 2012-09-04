#ifndef _ENGINE_GAME_FILE_
#define _ENGINE_GAME_FILE_
#pragma once

#include <iostream>
#include <fstream>

#include "../core/LinkedList.h"
#include "../core/Vector.h"

#include "StringHash.h"
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
			: m_data(NULL) { }

		//\brief Property type casting unsafe versions
		inline const char * GetString()
		{
			return (const char *)m_data;
		}

		inline int GetInt()
		{
			return atoi((const char *)m_data);
		}

		inline float GetFloat()
		{
			return (float)atof((const char *)m_data);
		}

		inline bool GetBool()
		{
			const char * vecString = (const char *)m_data;
			return strstr(vecString, "true") != NULL;
		}

		inline Vector GetVector()
		{
			const char * vecString = (const char *)m_data;
			if (strstr(vecString, ","))
			{
				float f1, f2, f3 = 0.0f;
				sscanf(StringUtils::TrimString(vecString), "%f,%f,%f", &f1, &f2, &f3);
				return Vector(f1, f2, f3);
			}
			return Vector::Zero();
		}

		inline Vector2 GetVector2() 
		{
			const char * vecString = (const char *)m_data;
			if (strstr(vecString, ","))
			{
				float f1, f2 = 0.0f;
				sscanf(StringUtils::TrimString(vecString), "%f,%f", &f1, &f2);
				return Vector2(f1, f2);
			}
			return Vector2::Vector2Zero();
		}

		StringHash m_name;
		void * m_data;
	};

	//\brief Each entity in the game file
	struct Object
	{
		Object()
			: m_firstChild(NULL)
			, m_next(NULL) { }

		//\brief Direct accessor for property data for convenience
		Property * GetProperty(const char * a_propertyName)
		{
			return GameFile::GetProperty(this, a_propertyName);
		}

		StringHash m_name;					// Literal declared before the open brace
		Object * m_firstChild;				// First child of this object
		Object * m_next;					// Next sibling object
		LinkedList<Property> m_properties;	// Properties of this object
	};

	//\ No work done in the constructor, only Init
	GameFile() {};
	~GameFile() { Unload(); }

	//\brief Load the game file and parse it into data
    bool Load(const char * a_filePath);
	void Unload();
	inline bool Reload() { Unload(); return Load(m_filePath); }
	inline bool IsLoaded() { return m_objects.GetLength() > 0; }

	//\brief Accessors to the gamefile property data
	const char * GetString(const char * a_object, const char * a_property);
	int GetInt(const char * a_object, const char * a_property);
	float GetFloat(const char * a_object, const char * a_property);
	bool GetBool(const char * a_object, const char * a_property);
	bool GetVector(const char * a_object, const char * a_property, Vector & a_vec_OUT);
	bool GetVector2(const char * a_object, const char * a_property, Vector2 & a_vec_OUT);

	//\brief Add an object that has properties
	//\param a_objectName is the literal declared on the line preceeding the open bracket
	//\param a_parent is the object's parent in the case of nested objects
	Object * AddObject(const char * a_objectName, Object * a_parent = NULL);

	//\brief Add a property with a parent object
	Property * AddProperty(GameFile::Object * a_parentObject, const char * a_propertyName, const char * a_value);

	//\brief Helper function to find an object by name
	Object * GetObject(const char * a_name);

	//brief Static helper function to find a property of an object, can be used at the file or object level
	static Property * GetProperty(Object * a_parent, const char * a_propertyName);

private:

	//\brief Recursive function to read an object definition with any child objects
	//\return the number of lines read
	unsigned int ReadObjectAndProperties(const char * a_objectName, std::ifstream & a_stream, Object * a_parentObject = NULL);

	//\brief Helper function to determine if a line of text defines a new object
	//\param const char * to a line read in from a file
	static inline bool IsLineNewObject(const char * a_line)
	{
		return !strstr(a_line, "{") && !strstr(a_line, "}") && !strstr(a_line, ":") && strlen(a_line) > 0;
	}

	LinkedList<Object> m_objects;							// All the objects in this file
	char m_filePath[StringUtils::s_maxCharsPerLine];		// Cache off the filepath so the file can be reloaded
};

#endif // _ENGINE_GAME_FILE
