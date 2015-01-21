#include <assert.h>

#include "Log.h"

#include "GameFile.h"

using namespace std;	//< For fstream operations

bool GameFile::Load(const char * a_filePath)
{
	char line[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	ifstream file(a_filePath, std::ifstream::in);
	
	// Open the file and parse each line 
	if (file.is_open())
	{
		// Read till the file has more contents or a rule is broken
		unsigned int lineCount = 0;
		while (file.good())
		{
			file.getline(line, StringUtils::s_maxCharsPerLine);
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
				int linesRead = ReadObjectAndProperties(line, file);
				if (linesRead == 0)
				{
					Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not open game file resource at path %s", a_filePath);
					return false;
				}
				lineCount += linesRead;
			}
			else // Bad formatting
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Bad game file format, expecting an object declaration at line %d of file %s.", lineCount, a_filePath);
			}
		}

		file.close();
		return true;
	}
	else
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not open game file resource at path %s", a_filePath);
		return false;
	}
}

unsigned int GameFile::ReadObjectAndProperties(const char * a_objectName, ifstream & a_stream, Object * a_parent)
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
		while(!strstr(line, "}"))
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

void GameFile::Unload()
{
	// Iterate through all objects and delete inclusive of properties
	LinkedListNode<Object> * nextObject = m_objects.GetHead();
	while(nextObject != NULL)
	{
		LinkedListNode<Property> * nextProperty = nextObject->GetData()->m_properties.GetHead();	
		while(nextProperty != NULL)
		{
			// Cache off the working node so we can delete it and its data
			LinkedListNode<Property> * curProperty = nextProperty;
			nextProperty = nextProperty->GetNext();

			nextObject->GetData()->m_properties.Remove(curProperty);
			delete curProperty->GetData();
			delete curProperty;
		}

		// Cache off working node as above
		LinkedListNode<Object> * curObject = nextObject;
		nextObject = nextObject->GetNext();

		m_objects.Remove(curObject);
		delete curObject->GetData();
		delete curObject;	
	}	
}

bool GameFile::Write(const char * a_filePath)
{
	// Create an output stream
	ofstream fileOutput;
	fileOutput.open(a_filePath);

	// Write menu header
	if (fileOutput.is_open())
	{
		// Output each object and it's properties
		LinkedListNode<Object> * cur = m_objects.GetHead();
		while(cur != NULL)
		{
			cur->GetData()->Serialise(fileOutput);	
			cur = cur->GetNext();
		}
	}

	// Cleanup
	fileOutput.close();
	return true;
}

const char * GameFile::GetString(const char * a_object, const char * a_property) const
{
	// First find the object
	if (Object * parentObject = FindObject(a_object))
	{
		// Then the property
		if (Property * prop = parentObject->FindProperty(a_property))
		{
			return prop->GetString();
		}
	}
	return "";
}

int GameFile::GetInt(const char * a_object, const char * a_property) const
{
	// First find the object
	if (Object * parentObject = FindObject(a_object))
	{
		// Then the property
		if (Property * prop = parentObject->FindProperty(a_property))
		{
			return prop->GetInt();
		}
	}
	return -1;
}

float GameFile::GetFloat(const char * a_object, const char * a_property) const
{
		// First find the object
	if (Object * parentObject = FindObject(a_object))
	{
		// Then the property
		if (Property * prop = parentObject->FindProperty(a_property))
		{
			return prop->GetFloat();
		}
	}
	return 0.0f;
}

bool GameFile::GetBool(const char * a_object, const char * a_property) const
{
	// First find the object
	if (Object * parentObject = FindObject(a_object))
	{
		// Then the property
		if (Property * prop = parentObject->FindProperty(a_property))
		{
			return prop->GetBool();
		}
	}
	return false;
}

bool GameFile::GetVector(const char * a_object, const char * a_property, Vector & a_vec_OUT) const
{
	// First find the object
	if (Object * parentObject = FindObject(a_object))
	{
		// Then the property
		if (Property * prop = parentObject->FindProperty(a_property))
		{
			a_vec_OUT = prop->GetVector();
			return true;
		}
	}
	return false;
}
	
bool GameFile::GetVector2(const char * a_object, const char * a_property, Vector2 & a_vec_OUT) const
{
	// First find the object
	if (Object * parentObject = FindObject(a_object))
	{
		// Then the property
		if (Property * prop = parentObject->FindProperty(a_property))
		{
			a_vec_OUT = prop->GetVector2();
			return true;
		}
	}
	return false;
}

GameFile::Object * GameFile::AddObject(const char * a_objectName, Object * a_parent)
{
	LinkedListNode<Object> * newObject = new LinkedListNode<Object>();
	newObject->SetData(new Object());

	// Set name
	newObject->GetData()->m_name = StringHash(a_objectName);

	// Top level object
	if (a_parent == NULL)
	{
		m_objects.Insert(newObject);
	}
	else // Set parent child relationship
	{
		a_parent->m_children.Insert(newObject);
	}

	return newObject->GetData();
}

GameFile::Property * GameFile::AddProperty(GameFile::Object * a_parentObject, const char * a_propertyName, const char * a_value)
{
	LinkedListNode<Property> * newProperty = new LinkedListNode<Property>();
	newProperty->SetData(new Property());
	newProperty->GetData()->m_name = StringHash(a_propertyName);
	ALLOC_CSTRING_COPY(newProperty->GetData()->m_data, a_value);

	a_parentObject->m_properties.Insert(newProperty);

	return newProperty->GetData();
}

GameFile::Object * GameFile::FindObject(const char * a_name) const
{
	// Iterate through all objects in this file looking for a name match
	LinkedListNode<Object> * cur = m_objects.GetHead();
	while(cur != NULL)
	{
		// Quit out of the loop if found
		if (cur->GetData()->m_name == StringHash::GenerateCRC(a_name))
		{
			return cur->GetData();
		}
		cur = cur->GetNext();
	}

	return NULL;
}

GameFile::Property * GameFile::Object::FindProperty(const char * a_propertyName) const
{
	// Iterate through all objects in the parent's property list till a name match is found
	LinkedListNode<Property> * cur = m_properties.GetHead();
	while(cur != NULL)
	{
		// Quit out of the loop if found
		if (cur->GetData()->m_name == StringHash::GenerateCRC(a_propertyName))
		{
			return cur->GetData();
		}
		cur = cur->GetNext();
	}

	// Not found
	return NULL;
}

GameFile::Object * GameFile::Object::FindObject(const char * a_objectName) const
{
	// Walk the hierachy to find the targeted object
	LinkedListNode<Object> * curChild = m_children.GetHead();
	while (curChild != NULL)
	{
		Object * curObject = curChild->GetData();
		if (curObject->m_name == StringHash::GenerateCRC(a_objectName))
		{
			return curObject;
		}
		curChild = curChild->GetNext();
	}

	// Not found
	return NULL;
}
