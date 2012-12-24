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

		inline void Serialise(std::ofstream & a_stream, unsigned int a_indentLevel = 0)
		{
			if (m_data)
			{
				GameFile::WriteTabs(a_stream, a_indentLevel);
				a_stream << m_name.GetCString() << ":" << (const char *)m_data << StringUtils::s_charLineEnd;
			}
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
		Property * FindProperty(const char * a_propertyName)
		{
			return GameFile::FindProperty(this, a_propertyName);
		}

		inline void Serialise(std::ofstream & a_stream, unsigned int a_indentLevel = 0)
		{
			GameFile::WriteTabs(a_stream, a_indentLevel);

			// Write object name and opening brackets
			a_stream << m_name.GetCString() << StringUtils::s_charLineEnd;
			GameFile::WriteTabs(a_stream, a_indentLevel);
			a_stream << "{" << StringUtils::s_charLineEnd;
			++a_indentLevel;

			// Iterate through all properties in the list
			LinkedListNode<Property> * cur = m_properties.GetHead();
			while(cur != NULL)
			{
				cur->GetData()->Serialise(a_stream, a_indentLevel);
				cur = cur->GetNext();
			}

			// Output siblings of this child
			Object * nextSibling = m_next;
			while(nextSibling != NULL)
			{
				nextSibling->Serialise(a_stream, a_indentLevel);
				nextSibling = nextSibling->m_next;
			}

			// Now the children of this child
			if (m_firstChild)
			{
				m_firstChild->Serialise(a_stream, a_indentLevel);
			}

			// Now end the object
			--a_indentLevel;
			GameFile::WriteTabs(a_stream, a_indentLevel);
			a_stream << "}" << StringUtils::s_charLineEnd;
		}

		StringHash m_name;					// Literal declared before the open brace
		Object * m_firstChild;				// First child of this object
		Object * m_next;					// Next sibling object
		LinkedList<Property> m_properties;	// Properties of this object
	};

	//\ No work done in the constructor, only Init
	GameFile() {};
	GameFile(const char * a_filePath) { Load(a_filePath); }
	~GameFile() { Unload(); }

	//\brief Load the game file and parse it into data
    bool Load(const char * a_filePath);
	void Unload();
	inline bool Reload() { Unload(); return Load(m_filePath); }
	inline bool IsLoaded() { return m_objects.GetLength() > 0; }

	//\brief Write data from memory to file preserving inheritance
	bool Write(const char * a_filePath);

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
	//\param a_parentObject is a pointer to another Object of the same file that will be linked to the new property
	//\param a_propertyName is a pointer to a cstring containing the name of the property to add
	//\param a_value is the string version of the value to add, other data types will be cast from string
	//\return A pointer to the property that was added or NULL if the operation was unsuccesfull
	Property * AddProperty(GameFile::Object * a_parentObject, const char * a_propertyName, const char * a_value);

	//\brief Helper function to find an object by name
	//\param a_name is the pointer to a c string of the name
	//\return A pointer to the object data or NULL if there was no object found by name
	Object * FindObject(const char * a_name);

	//brief Static helper function to find a property of an object, can be used at the file or object level
	static Property * FindProperty(Object * a_parent, const char * a_propertyName);

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

	//\brief Helper function to write a number of tabs to an output stream
	static inline void WriteTabs(std::ofstream & a_stream, unsigned int a_indentLevel)
	{
		for (unsigned int i = 0; i < a_indentLevel; ++i)
		{
			a_stream << StringUtils::s_charTab;
		}
	}

	LinkedList<Object> m_objects;							///< All the objects in this file
	char m_filePath[StringUtils::s_maxCharsPerLine];		///< Cache off the filepath so the file can be reloaded
};

#endif // _ENGINE_GAME_FILE
