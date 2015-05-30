#ifndef _ENGINE_GAME_FILE_
#define _ENGINE_GAME_FILE_
#pragma once

#include <iostream>
#include <fstream>

#include "../core/Colour.h"
#include "../core/LinkedList.h"
#include "../core/Vector.h"
#include "../core/Quaternion.h"

#include "DataPack.h"
#include "Log.h"
#include "StringHash.h"
#include "StringUtils.h"

using namespace std;	//< For fstream operations

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

struct DataPackEntry;

class GameFile
{
public:
	
	//\brief Each property belonging to an object
	struct Property
	{
		Property()
			: m_data(NULL) { }

		//\brief Property type casting unsafe versions
		inline const char * GetString() const
		{
			return (const char *)m_data;
		}

		inline int GetInt() const
		{
			return atoi((const char *)m_data);
		}

		inline float GetFloat() const
		{
			return (float)atof((const char *)m_data);
		}

		inline bool GetBool() const
		{
			const char * vecString = (const char *)m_data;
			return strstr(vecString, "true") != NULL;
		}

		inline Colour GetColour() const
		{
			const char * colString = (const char *)m_data;
			if (strstr(colString, ","))
			{
				float r, g, b, a = 0.0f;
				sscanf(StringUtils::TrimString(colString), "%f,%f,%f,%f", &r, &g, &b, &a);
				return Colour(r, g, b, a);
			}
			return sc_colourWhite;
		}

		inline Vector GetVector() const
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

		inline Quaternion GetQuaternion() const
		{
			const char * quatString = (const char *)m_data;
			if (strstr(quatString, ","))
			{
				float f1, f2, f3, f4 = 0.0f;
				sscanf(StringUtils::TrimString(quatString), "%f,%f,%f,%f", &f1, &f2, &f3, &f4);
				return Quaternion(f1, f2, f3, f4);
			}
			return Quaternion();
		}

		inline Vector2 GetVector2() const
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
				a_stream << m_name.GetCString() << ": " << (const char *)m_data << StringUtils::s_charLineEnd;
			}
		}

		StringHash m_name;
		void * m_data;
	};

	//\brief Each entity in the game file
	struct Object
	{
		Object() { }

		//\brief Direct accessor of property data for convenience
		Property * FindProperty(const char * a_propertyName) const;
		Object * FindObject(const char * a_objectName) const;
		inline LinkedListNode<Object> * GetChildren() const { return m_children.GetHead(); }

		//\brief Write out in memory gamefile to stream
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

			// Now the children of this child
			LinkedListNode<Object> * curChild = m_children.GetHead();
			while (curChild != NULL)
			{
				curChild->GetData()->Serialise(a_stream, a_indentLevel);
				curChild = curChild->GetNext();
			}

			// Now end the object
			--a_indentLevel;
			GameFile::WriteTabs(a_stream, a_indentLevel);
			a_stream << "}" << StringUtils::s_charLineEnd;
		}

		StringHash m_name;					///< Literal declared before the open brace
		LinkedList<Object> m_children;		///< Any child objects belonging to this object
		LinkedList<Property> m_properties;	///< Properties of this object
	};

	//\ No work done in the constructor, only Init
	GameFile() {};
	GameFile(const char * a_filePath) { Load(a_filePath); }
	GameFile(DataPackEntry * a_packData) { Load(a_packData); }
	~GameFile() { Unload(); }

	//\brief Load the game file and parse it into data
	bool Load(const char * a_filePath)
	{
		ifstream file(a_filePath, ifstream::in);
		if (LoadData<ifstream>(file))
		{
			return true;
		}
		else
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not open game file resource at path %s", a_filePath);
			return false;
		}
	}

	inline bool Load(DataPackEntry * a_packData)
	{
		if (LoadData<DataPackEntry>(*a_packData))
		{
			return true;
		}
		else
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not open game datapack file resource.");
			return false;
		}
	}

	void Unload();
	inline bool IsLoaded() const { return m_objects.GetLength() > 0; }

	//\brief Write data from memory to file preserving inheritance
	bool Write(const char * a_filePath);

	//\brief Accessors to the gamefile property data
	const char * GetString(const char * a_object, const char * a_property) const;
	int GetInt(const char * a_object, const char * a_property) const;
	float GetFloat(const char * a_object, const char * a_property) const;
	bool GetBool(const char * a_object, const char * a_property) const;
	bool GetVector(const char * a_object, const char * a_property, Vector & a_vec_OUT) const;
	bool GetVector2(const char * a_object, const char * a_property, Vector2 & a_vec_OUT) const;

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
	Object * FindObject(const char * a_name) const;

private:

	//\brief Load function will work with either datapack entry or input stream
	template <typename TInputData>
	bool LoadData(TInputData & a_input)
	{
		char line[StringUtils::s_maxCharsPerLine];
		memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);

		// Open the file
		if (a_input.is_open())
		{
			// Read till the file has more contents or a rule is broken
			unsigned int lineCount = 0;
			while (a_input.good())
			{
				a_input.getline(line, StringUtils::s_maxCharsPerLine);
				lineCount++;

				// Parse any comment lines
				if (strstr(line, "//"))
				{
					continue;
				}

				// Parse any empty lines
				if (strlen(line) <= 0)
				{
					continue;
				}

				// A line without any symbols means a new object
				if (IsLineNewObject(line))
				{
					int linesRead = ReadObjectAndProperties<TInputData>(line, a_input);
					if (linesRead == 0)
					{
						Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not open game file resource");
						return false;
					}
					lineCount += linesRead;
				}
				else // Bad formatting
				{
					Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Bad game file format, expecting an object declaration at line %d.", lineCount);
				}
			}
		}
		a_input.close();
		return true;
	}

	//\brief Recursive function to read an object definition with any child objects
	//\return the number of lines read
	template <typename TInputData>
	unsigned int ReadObjectAndProperties(const char * a_objectName, TInputData & a_stream, Object * a_parent = NULL)
	{
		char line[StringUtils::s_maxCharsPerLine];
		memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
		unsigned int lineCount = 0;

		// Create a new child and optionally link it to the parent
		Object * currentObject = AddObject(StringUtils::TrimString(a_objectName), a_parent);
		a_stream.getline(line, StringUtils::s_maxCharsPerLine);
		lineCount++;

		// Parse any comment line
		if (strstr(line, "//"))
		{
			a_stream.getline(line, StringUtils::s_maxCharsPerLine);
			lineCount++;;
		}

		// Next line should be a brace
		unsigned int braceCount = 0;
		if (strstr(line, "{"))
		{
			braceCount++;
			a_stream.getline(line, StringUtils::s_maxCharsPerLine);
			lineCount++;

			// Parse any comment line
			if (strstr(line, "//"))
			{
				a_stream.getline(line, StringUtils::s_maxCharsPerLine);
				lineCount++;
			}

			// Now properties of that object
			while (!strstr(line, "}"))
			{
				// Look for child object
				if (IsLineNewObject(line))
				{
					// Read the child object
					lineCount += ReadObjectAndProperties(line, a_stream, currentObject);
					a_stream.getline(line, StringUtils::s_maxCharsPerLine);
					lineCount++;
				}
				else if (strlen(StringUtils::TrimString(line)) > 0) // Checking for normal property
				{
					// Break apart the property and parse for type
					const char * propName = StringUtils::ExtractPropertyName(line, ":");
					const char * propValue = StringUtils::ExtractValue(line, ":");
					if (propName && propValue)
					{
						AddProperty(currentObject, propName, propValue);
						a_stream.getline(line, StringUtils::s_maxCharsPerLine);
						lineCount++;
					}
					else
					{
						Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Bad game file format, there is a missing property name and/or value for object %s at line %u.", a_objectName, lineCount);
						return 0;
					}
				}
				else // Whitespace, move on
				{
					break;
				}
			}
			braceCount--;
		}
		else if (braceCount > 0) // Mismatched number of braces
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Bad game file format, expecting an open brace after declaration for object %s on line %u.", a_objectName, lineCount);
			return 0;
		}

		return lineCount;
	}

	//\brief Helper function to determine if a line of text defines a new object
	//\param const char * to a line read in from a file
	static inline bool IsLineNewObject(const char * a_line)
	{
		return !strstr(a_line, "{") && !strstr(a_line, "}") && !strstr(a_line, ":") && strlen(StringUtils::TrimString(a_line)) > 0;
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
};

#endif // _ENGINE_GAME_FILE
