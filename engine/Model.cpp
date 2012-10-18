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

	// Storage for file reading progress
	char line[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	ifstream file(a_modelFilePath);
	unsigned int lineCount = 0;
	
	// Open the file and parse each line 
	if (file.is_open())
	{
		unsigned int numNormals = 0;	// How many normals in the model
		unsigned int numVerts = 0;		// How many vertices are in the model
		unsigned int numUvs = 0;		// Number of texture coords

		// TODO Yay lets only support models with 64 faces
		unsigned int vertIndices[3][64];
		unsigned int normalIndices[3][64];
		unsigned int uvIndices[3][64];

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
				m_diffuseTex = TextureManager::Get().GetTexture(StringUtils::ExtractField(line, " ", 0), TextureManager::eCategoryModel);
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
					++numVerts;
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
					++numUvs;
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
					++numNormals;
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
				// Extract and store out which vertices are used for the faces of the model
				sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", 
					&vertIndices[0][m_numFaces], &uvIndices[0][m_numFaces], &normalIndices[0][m_numFaces], 
					&vertIndices[1][m_numFaces], &uvIndices[1][m_numFaces], &normalIndices[1][m_numFaces], 
					&vertIndices[2][m_numFaces], &uvIndices[2][m_numFaces], &normalIndices[2][m_numFaces]);

				// File format specifies vertices are indexed from 1 instead of 0 so fix that
				--vertIndices[0][m_numFaces];	--vertIndices[1][m_numFaces];	--vertIndices[2][m_numFaces];
				--uvIndices[0][m_numFaces];		--uvIndices[1][m_numFaces];		--uvIndices[2][m_numFaces];
				--normalIndices[0][m_numFaces];	--normalIndices[1][m_numFaces];	--normalIndices[2][m_numFaces];

				++m_numFaces;
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

			// Now map faces to triangles by linking vertex, uv and normals by index
			unsigned int vertCount = 0;
			for (unsigned int i = 0; i < m_numFaces; ++i)
			{                
				// Set the face data up
				m_verts[vertCount]		= verts[vertIndices[0][i]];
				m_normals[vertCount]	= normals[normalIndices[0][i]];
				m_uvs[vertCount]		= uvs[uvIndices[0][i]];
				vertCount++;

				m_verts[vertCount]		= verts[vertIndices[1][i]];
				m_normals[vertCount]	= normals[normalIndices[1][i]];
				m_uvs[vertCount]		= uvs[uvIndices[1][i]];
				vertCount++;

				m_verts[vertCount]		= verts[vertIndices[2][i]];
				m_normals[vertCount]	= normals[normalIndices[2][i]];
				m_uvs[vertCount]		= uvs[uvIndices[2][i]];
				vertCount++;
			}
		}
		else
		{
			Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Cannot allocate memory for the faces of model %s", a_modelFilePath);
		}		

		// Loaded succesfully
		file.close();
		m_loaded = true;
		return m_numFaces > 0;
	}
	else
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Could not open model file resource at path %s", a_modelFilePath);
		return false;
	}
}

bool Model::Unload()
{
	// Deallocate memory here

	m_loaded = false;
	return true;
}
