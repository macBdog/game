#include <iostream>
#include <fstream>

#include "Log.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Model.h"

using namespace std;	// For iostream resources

bool Model::Load(const char *a_modelFilePath, LinearAllocator<Vector> & a_vertPool, LinearAllocator<Vector> & a_normalPool, LinearAllocator<TexCoord> & a_uvPool)
{
	// Early out for no file case
	if (a_modelFilePath == NULL)
	{
		return false;
	}

	// Storage for model file reading progress
	char line[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	ifstream file(a_modelFilePath);
	unsigned int lineCount = 0;

	// Storage for material file reading progress
	char materialFileName[StringUtils::s_maxCharsPerLine];
	char materialFilePath[StringUtils::s_maxCharsPerLine];
	char materialName[StringUtils::s_maxCharsPerLine];
	strcpy(materialFilePath, a_modelFilePath);
	StringUtils::TrimFileNameFromPath(materialFilePath);
	memset(&materialFileName, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	memset(&materialName, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	
	// Open the file and parse each line 
	if (file.is_open())
	{
		// Quickly scan through and count the elements
		unsigned int numNormals = 0;	// How many normals in the model
		unsigned int numVerts = 0;		// How many vertices are in the model
		unsigned int numUvs = 0;		// Number of texture coords
		while (file.good())
		{
			file.getline(line, StringUtils::s_maxCharsPerLine);
			if (line[0] == 'v' && line[1] == ' ')
			{
				++numVerts;
			}
			else if (line[0] == 'v' && line[1] == 't')
			{
				++numUvs;
			}
			else if (line[0] == 'v' && line[1] == 'n')
			{
				++numNormals;
			}
			else if (line[0] == 'f' && line[1] == ' ')
			{
				++m_numFaces;
			}
		}

		// Allocate space for the indices
		LinearAllocator<unsigned int> vertIndexPool(sizeof(unsigned int) * 3 * m_numFaces);
		LinearAllocator<unsigned int> normIndexPool(sizeof(unsigned int) * 3 * m_numFaces);
		LinearAllocator<unsigned int> uvIndexPool(sizeof(unsigned int) * 3 * m_numFaces);

		unsigned int * vertIndices = vertIndexPool.GetHead();
		unsigned int * normIndices = normIndexPool.GetHead();
		unsigned int * uvIndices = uvIndexPool.GetHead();

		// Rewind the file and start reading again
		file.clear();
		file.seekg(0, ios::beg);
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
				// Cache off the material file name for later usage
				sscanf(line, "mtllib %s", &materialFileName);

				// Append the filename onto the file path
				StringUtils::AppendString(materialFilePath, materialFileName); 
				continue;
			}
			// Material name
			else if (strstr(line, "usemtl"))
			{
				// Set diffuse texture
				sscanf(line, "usemtl %s", materialName);
			}
			// Vertex
			else if (line[0] == 'v' && line[1] == ' ')
			{
				if (Vector * v = a_vertPool.Allocate(sizeof(Vector)))
				{
					float vec[3];
					sscanf(line, "v %f %f %f", &vec[0], &vec[1], &vec[2]);
					v->SetX(vec[0]);
					v->SetY(vec[1]);
					v->SetZ(vec[2]);
				}
			}
			// Texture coord
			else if (line[0] == 'v' && line[1] == 't')
			{
				if (TexCoord * t = a_uvPool.Allocate(sizeof(TexCoord)))
				{
					float uv[2];
					sscanf(line, "vt %f %f", &uv[0], &uv[1]);
					t->SetX(uv[0]);
					t->SetY(uv[1]);
				}
			}
			// Vertex normal
			else if (line[0] == 'v' && line[1] == 'n')
			{
				if (Vector * vn = a_normalPool.Allocate(sizeof(Vector)))
				{
					float norm[3];
					sscanf(line, "vn %f %f %f", &norm[0], &norm[1], &norm[2]);
					vn->SetX(norm[0]);
					vn->SetY(norm[1]);
					vn->SetZ(norm[2]);
				}
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
				// Check for no texture map
				if (strstr(line, "\\\\") != NULL)
				{
					Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot load model %s with no uv or normal exported per face", a_modelFilePath);
					file.close();
					return false;
				}

				// Ignore face definitions without the correct format
				if (StringUtils::CountCharacters(line, '/') == 6)
				{
					// Extract and store out which vertices are used for the faces of the model
					sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", 
					&vertIndices[0], &uvIndices[0], &normIndices[0], 
					&vertIndices[1], &uvIndices[1], &normIndices[1], 
					&vertIndices[2], &uvIndices[2], &normIndices[2]);

					// File format specifies vertices are indexed from 1 instead of 0 so fix that
					--vertIndices[0];	--vertIndices[1];	--vertIndices[2];
					--uvIndices[0];		--uvIndices[1];		--uvIndices[2];
					--normIndices[0];	--normIndices[1];	--normIndices[2];

					// Advance to the next face
					vertIndices+=3;
					uvIndices+=3;
					normIndices+=3;
				}
				else // Remove invalid face
				{
					--m_numFaces;
				}
			}
		}

		// Now we know the size of the mesh, allocate memory for verts
		// TODO Use a resizable memory structure for mesh data
		m_verts = (Vector *)malloc(sizeof(Vector) * m_numFaces * s_vertsPerTri);
		m_normals = (Vector *)malloc(sizeof(Vector) * m_numFaces * s_vertsPerTri);
		m_uvs = (TexCoord *)malloc(sizeof(TexCoord) * m_numFaces * s_vertsPerTri);
		if (m_verts != NULL && m_normals != NULL && m_uvs != NULL)
		{
			// Create an alias to the loading memory pools so the data can be accessed randomly
			Vector * verts = a_vertPool.GetHead();
			Vector * normals = a_normalPool.GetHead();
			TexCoord * uvs = a_uvPool.GetHead();
			vertIndices = vertIndexPool.GetHead();
			normIndices = normIndexPool.GetHead();
			uvIndices = uvIndexPool.GetHead();

			// Now map faces to triangles by linking vertex, uv and normals by index
			unsigned int vertCount = 0;
			for (unsigned int i = 0; i < m_numFaces; ++i)
			{                
				// Set the face data up
				m_verts[vertCount]		= verts[vertIndices[0]];
				m_normals[vertCount]	= normals[normIndices[0]];
				m_uvs[vertCount]		= uvs[uvIndices[0]];
				vertCount++;

				m_verts[vertCount]		= verts[vertIndices[1]];
				m_normals[vertCount]	= normals[normIndices[1]];
				m_uvs[vertCount]		= uvs[uvIndices[1]];
				vertCount++;

				m_verts[vertCount]		= verts[vertIndices[2]];
				m_normals[vertCount]	= normals[normIndices[2]];
				m_uvs[vertCount]		= uvs[uvIndices[2]];

				// Advance to the next face
				vertIndices += 3;
				uvIndices += 3;
				normIndices += 3;
				vertCount++;
			}
		}
		else
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot allocate memory for the faces of model %s", a_modelFilePath);
		}		

		// Model data loaded succesfully
		file.close();
		m_loaded = true;

		// Load the material resources
		bool materialLoadSuccess = LoadMaterial(materialFilePath, materialName);

		return m_numFaces > 0 && materialLoadSuccess;
	}
	else
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not open model file resource at path %s", a_modelFilePath);
		return false;
	}
}

bool Model::LoadMaterial(const char * a_materialFileName, const char * a_materialName)
{
	// Early out for no file case
	if (a_materialFileName == NULL)
	{
		return false;
	}

	// Storage for material file reading progress
	char line[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	ifstream file(a_materialFileName);
	unsigned int lineCount = 0;
	bool foundTargetMaterial = false;
	
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
			else if (strstr(line, "newmtl"))
			{
				// Cache off the material file name for later usage
				char tempMatName[StringUtils::s_maxCharsPerLine];
				memset(&tempMatName, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
				sscanf(line, "newmtl %s", &tempMatName);

				// Set flag so we load the next map specific in the material as the diffuse map
				foundTargetMaterial = strstr(tempMatName, a_materialName) != NULL;
				continue;
			}
			else if (foundTargetMaterial)
			{
				float matX, matY, matZ = 0.0f;

				// Diffuse map name
				if (strstr(line, "map_Kd") )
				{
					char tempMatName[StringUtils::s_maxCharsPerLine];
					memset(&tempMatName, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
					sscanf(StringUtils::ExtractFileNameFromPath(line), "%s", &tempMatName);

					m_diffuseTex = TextureManager::Get().GetTexture(tempMatName, TextureCategory::Model);
					file.close();
					return true;
				}
				// Ambient colour
				if (strstr(line, "Ka"))
				{
					sscanf(line, "Ka %f %f %f", &matX, &matY, &matZ);
					m_material.m_ambient.SetX(matX);
					m_material.m_ambient.SetY(matY);
					m_material.m_ambient.SetZ(matZ);
				}
				// Diffuse colour
				else if (strstr(line, "Kd"))
				{
					sscanf(line, "Kd %f %f %f", &matX, &matY, &matZ);
					m_material.m_diffuse.SetX(matX);
					m_material.m_diffuse.SetY(matY);
					m_material.m_diffuse.SetZ(matZ);
				}
				// Specular colour
				else if (strstr(line, "Ks"))
				{
					sscanf(line, "Ks %f %f %f", &matX, &matY, &matZ);
					m_material.m_specular.SetX(matX);
					m_material.m_specular.SetY(matY);
					m_material.m_specular.SetZ(matZ);
				}
				// Shininess value
				else if (strstr(line, "Ns"))
				{
					float shininess = 0.0f;
					sscanf(line, "Ns %f", &shininess);
					m_material.m_shininess = (int)shininess;
				}
			}
		}

		// Report error and fail out
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot find material def %s in material file %s", a_materialName, a_materialFileName);
		file.close();
		return false;
	}

	// No material file to load
	Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot open material file %s", a_materialFileName);
	return false;
}

bool Model::Unload()
{
	// Deallocate memory here
	free(m_verts);
	free(m_normals);
	free(m_uvs);

	m_loaded = false;
	return true;
}
