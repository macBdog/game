#include <iostream>
#include <fstream>

#include "Texture.h"

#include "Model.h"

using namespace std;	// For iostream resources

bool Model::Load(const char *a_modelFilePath)
{
	// Early out for no file case
	if (a_modelFilePath == NULL)
	{
		return false;
	}

	ifstream file(a_modelFilePath);
	unsigned int lineCount = 0;
	
	// Open the file and parse each line 
	if (file.is_open())
	{
		// Loaded succesfully
		return true;
	}
	else
	{
		return false;
	}
	// Load texture data into memory and check if successful
}

bool Model::Unload()
{
	// Deallocate memory here

	return true;
}