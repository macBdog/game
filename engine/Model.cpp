#include <iostream>
#include <fstream>

#include "Log.h"
#include "Texture.h"
#include "StringUtils.h"

#include "Model.h"

using namespace std;	// For iostream resources

bool Model::Load(const char *a_modelFilePath)
{
	// Early out for no file case
	if (a_modelFilePath == NULL)
	{
		return false;
	}

	char line[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	ifstream file(a_modelFilePath);
	unsigned int lineCount = 0;
	
	// Open the file and parse each line 
	if (file.is_open())
	{
		while (file.good())
		{
			file.getline(line, StringUtils::s_maxCharsPerLine);
			lineCount++;

			// Comment
			if (strstr(line, "#"))
			{
				continue;
			}
			// Material library declaration
			else if (strstr(line, "mtllib"))
			{
				continue;
			}
			// Material name
			else if (strstr(line, "usemtl"))
			{
				// Set diffuse texture
				continue;
			}
			// Vertex
			else if (line[0] == 'v' && line[1] == ' ')
			{

			}
			// Texture coord
			else if (line[0] == 'v' && line[1] == 't')
			{

			}
			// Vertex normal
			else if (line[0] == 'v' && line[1] == 'n')
			{

			}
			// Group declaration
			else if (line[0] == 'g' && line[1] == ' ')
			{
				// bool?

			}
			// Smoothing group
			else if (line[0] == 's' && line[1] == ' ')
			{
				// bool?
			}
			// Merging group
			else if (line[0] == 'm' && line[1] == 'g')
			{
				// bool?
			}
			// Object name
			else if (line[0] == 'o' && line[1] == ' ')
			{
				// bool?
			}
			// Face
			else if (line[0] == 'f' && line[1] == ' ')
			{

			}
		}

		// Loaded succesfully
		file.close();
		return lineCount > 0;
	}
	else
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Could not open model file resource at path %s", a_modelFilePath);
		return false;
	}

	// Load texture data into memory and check if successful
}

bool Model::Unload()
{
	// Deallocate memory here

	return true;
}