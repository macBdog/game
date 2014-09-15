#include <iostream>
#include <fstream>

#include "Log.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Model.h"

using namespace std;	// For iostream resources

bool Model::Load(const char * a_modelFilePath, ModelDataPool & a_modelData)
{
	// Early out for no file case
	if (a_modelFilePath == NULL)
	{
		return false;
	}

	// Set name
	sprintf(m_name, "%s", StringUtils::ExtractFileNameFromPath(a_modelFilePath));

	// Storage for model file reading progress
	char line[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	ifstream file(a_modelFilePath, ios::binary);

	// Read one object at a time
	Object * currentObject = NULL;
	ios::streampos lastReadOffset = 0;
	ios::streampos currentObjectOffset = 0;
	ObjectReadPass::Enum objectReadPass = ObjectReadPass::CountFaces;
	unsigned int numNormals = 0;
	unsigned int numVerts = 0;
	unsigned int numUvs = 0;
	unsigned int numFaces = 0;
	unsigned int objectVertOffset = 0;
	unsigned int objectNormOffset = 0;
	unsigned int objectUvOffset = 0;
	Vector * objectVerts = NULL;
	Vector * objectNormals = NULL;
	TexCoord * objectUvs = NULL;

	// Storage for material file reading progress
	char materialFileName[StringUtils::s_maxCharsPerLine];
	char materialFilePath[StringUtils::s_maxCharsPerLine];
	strcpy(materialFilePath, a_modelFilePath);
	StringUtils::TrimFileNameFromPath(materialFilePath);
	memset(&materialFileName, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);

	// Open the file and parse each line 
	if (file.is_open())
	{
		while (file.good())
		{
			lastReadOffset = file.tellg();
			file.getline(line, StringUtils::s_maxCharsPerLine);

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
			// Object name
			else if (line[0] == 'o' && line[1] == ' ')
			{
				char objectName[StringUtils::s_maxCharsPerName];
				objectName[0] = '\0';
				sscanf(line, "o %s", &objectName[0]);
				currentObject = new Object();
				currentObject->SetName(objectName);
				currentObjectOffset = lastReadOffset;
				continue;
			}

			// A nameless object has begun
			if (currentObject == NULL && (line[0] == 'v' || line[0] == 'f' || line[0] == 'g' || line[0] == 's' || line[0] == 'm' || line[0] == 'u'))
			{
				currentObject = new Object();
				currentObject->SetName("UNNAMED_OBJECT");
				currentObjectOffset = lastReadOffset;
			}

			if (currentObject != NULL)
			{
				while (line[0] == 'v' || line[0] == 'f' || line[0] == 'g' || line[0] == 's' || line[0] == 'm' || line[0] == 'u')
				{
					// Material name
					if (strstr(line, "usemtl"))
					{
						// Read the material from the mtl file and assign it
						char materialName[StringUtils::s_maxCharsPerLine];
						materialName[0] = '\0';
						sscanf(line, "usemtl %s", materialName);
				
						// Load the material resources
						if (strlen(materialName) > 0)
						{
							Material * newMaterial = a_modelData.m_materialPool.Allocate(sizeof(Material), true);
							if (newMaterial->Load(materialFilePath, materialName))
							{
								currentObject->SetMaterial(newMaterial);
							}
							else
							{
								Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Material load failed for material called %s in file %s!", materialName, materialFilePath);
								a_modelData.m_materialPool.DeAllocate(sizeof(Material));
							}
						}
					}

					if (objectReadPass == ObjectReadPass::CountFaces)
					{
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
							// Check for no texture map
							if (strstr(line, "\\\\") != NULL)
							{
								Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot load model %s with no uv or normal exported per face", a_modelFilePath);
								file.close();
								return false;
							}
							else
							{
								++numFaces;
							}
						}
					}
					lastReadOffset = file.tellg();
					file.getline(line, StringUtils::s_maxCharsPerLine);
				}
				
				// Now we know the size of the mesh, allocate memory for verts
				objectVerts = (Vector *)malloc(sizeof(Vector) * numFaces * s_vertsPerTri);
				objectNormals = (Vector *)malloc(sizeof(Vector) * numFaces * s_vertsPerTri);
				objectUvs = (TexCoord *)malloc(sizeof(TexCoord) * numFaces * s_vertsPerTri);
				
				// Allocate space on the stack for temporary storage of the indices
				LinearAllocator<unsigned int> vertIndexPool(sizeof(unsigned int) * 3 * numFaces);
				LinearAllocator<unsigned int> normIndexPool(sizeof(unsigned int) * 3 * numFaces);
				LinearAllocator<unsigned int> uvIndexPool(sizeof(unsigned int) * 3 * numFaces);
				unsigned int * vertIndices = vertIndexPool.GetHead();
				unsigned int * normIndices = normIndexPool.GetHead();
				unsigned int * uvIndices = uvIndexPool.GetHead();
				
				// Now read the object again this time parsing the mesh data
				int numFacesPatched = 0;
				objectReadPass = ObjectReadPass::ReadFaces;
				if (!file.good())
				{
					file.seekg(0, ios::beg);
					file.clear();
				}
				file.seekg(currentObjectOffset);
				file.getline(line, StringUtils::s_maxCharsPerLine);
				file.getline(line, StringUtils::s_maxCharsPerLine);

				while (line[0] == 'v' || line[0] == 'f' || line[0] == 'g' || line[0] == 's' || line[0] == 'm' || line[0] == 'u')
				{
					// Vertex
					if (line[0] == 'v' && line[1] == ' ')
					{
						if (Vector * v = a_modelData.m_vertPool.Allocate(sizeof(Vector)))
						{
							float vec[3];
							sscanf(line, "v %f %f %f", &vec[0], &vec[1], &vec[2]);
							v->SetX(vec[0]);
							v->SetY(vec[1]);
							v->SetZ(vec[2]);
						}
						else
						{
							Log::Get().WriteEngineErrorNoParams("Ran out of model loading resources!");
						}
					}
					// Texture coord
					else if (line[0] == 'v' && line[1] == 't')
					{
						if (TexCoord * t = a_modelData.m_uvPool.Allocate(sizeof(TexCoord)))
						{
							float uv[2];
							sscanf(line, "vt %f %f", &uv[0], &uv[1]);
							t->SetX(uv[0]);
							t->SetY(uv[1]);
						}
						else
						{
							Log::Get().WriteEngineErrorNoParams("Ran out of model loading resources!");
						}
					}
					// Vertex normal
					else if (line[0] == 'v' && line[1] == 'n')
					{
						if (Vector * vn = a_modelData.m_normalPool.Allocate(sizeof(Vector)))
						{
							float norm[3];
							sscanf(line, "vn %f %f %f", &norm[0], &norm[1], &norm[2]);
							vn->SetX(norm[0]);
							vn->SetY(norm[1]);
							vn->SetZ(norm[2]);
						}
						else
						{
							Log::Get().WriteEngineErrorNoParams("Ran out of model loading resources!");
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
					// Face
					else if (line[0] == 'f' && line[1] == ' ')
					{
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
							++numFacesPatched;
						}
						else
						{
							Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Model file %s contains faces that have not been unwrapped!", a_modelFilePath);
						}
					}
					lastReadOffset = file.tellg();
					file.getline(line, StringUtils::s_maxCharsPerLine);
					objectReadPass = numFacesPatched == numFaces ? ObjectReadPass::PatchFaces : ObjectReadPass::ReadFaces;
				}

				// Last step is to patch in the mesh data to the faces
				if (objectReadPass == ObjectReadPass::PatchFaces)
				{
					unsigned int numObjectVertices = 0;
					if (objectVerts != NULL && objectNormals != NULL && objectUvs != NULL)
					{
						// Create an alias to the loading memory pools so the data can be accessed randomly
						Vector * verts = a_modelData.m_vertPool.GetHead();
						Vector * normals = a_modelData.m_normalPool.GetHead();
						TexCoord * uvs = a_modelData.m_uvPool.GetHead();
						vertIndices = vertIndexPool.GetHead();
						normIndices = normIndexPool.GetHead();
						uvIndices = uvIndexPool.GetHead();

						// Now map faces to triangles by linking vertex, uv and normals by index
						for (unsigned int i = 0; i < numFaces; ++i)
						{                
							// Set the face data up
							objectVerts[numObjectVertices]		= verts[vertIndices[0] - objectVertOffset];
							objectNormals[numObjectVertices]	= normals[normIndices[0] - objectNormOffset];
							objectUvs[numObjectVertices]		= uvs[uvIndices[0] - objectUvOffset];
							numObjectVertices++;

							objectVerts[numObjectVertices]		= verts[vertIndices[1] - objectVertOffset];
							objectNormals[numObjectVertices]	= normals[normIndices[1] - objectNormOffset];
							objectUvs[numObjectVertices]		= uvs[uvIndices[1] - objectUvOffset];
							numObjectVertices++;

							objectVerts[numObjectVertices]		= verts[vertIndices[2] - objectVertOffset];
							objectNormals[numObjectVertices]	= normals[normIndices[2] - objectNormOffset];
							objectUvs[numObjectVertices]		= uvs[uvIndices[2] - objectUvOffset];
							numObjectVertices++;

							// Advance to the next face
							vertIndices += 3;
							uvIndices += 3;
							normIndices += 3;
						}
					}
					else
					{
						Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot allocate memory for the faces of model %s", a_modelFilePath);
					}
				
					// Deallocate the loading pools
					a_modelData.m_vertPool.DeAllocate(sizeof(Vector) * numVerts);
					a_modelData.m_normalPool.DeAllocate(sizeof(Vector) * numNormals);
					a_modelData.m_uvPool.DeAllocate(sizeof(TexCoord) * numUvs);
				
					// Add the object to the list of objects
					currentObject->SetNumVertices(numObjectVertices);
					currentObject->SetVertices(objectVerts);
					currentObject->SetNormals(objectNormals);
					currentObject->SetUvs(objectUvs);
					ObjectNode * newObject = new ObjectNode();
					newObject->SetData(currentObject);
					m_objects.Insert(newObject);

					// Reset object reading state
					currentObject = NULL;
					currentObjectOffset = 0;
					objectReadPass = ObjectReadPass::CountFaces;
					objectVertOffset += numVerts;
					objectNormOffset += numNormals;
					objectUvOffset += numUvs;
					numNormals = 0;
					numVerts = 0;
					numUvs = 0;
					numFaces = 0;
					objectVerts = NULL;
					objectNormals = NULL;
					objectUvs = NULL;
				
					// Setup the next object to be read
					if (line[0] == 'o')
					{
						char objectName[StringUtils::s_maxCharsPerName];
						objectName[0] = '\0';
						sscanf(line, "o %s", &objectName[0]);
						currentObject = new Object();
						currentObject->SetName(objectName);
						currentObjectOffset = lastReadOffset;
						continue;
					}
					else
					{
						break;
					}
				}
			}
		}

		// Model data loaded succesfully
		file.close();
		return m_objects.GetLength() > 0;
	}
	else
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not open model file resource at path %s", a_modelFilePath);
		return false;
	}
}


bool Model::Unload()
{
	ObjectNode * curObjectNode = m_objects.GetHead();
	while (curObjectNode != NULL)
	{
		// Deallocate memory allocated during mode load
		Object * curObject = curObjectNode->GetData();

		free(curObject->GetVertices());
		free(curObject->GetNormals());
		free(curObject->GetUvs());

		curObject->SetVertices(NULL);
		curObject->SetNormals(NULL);
		curObject->SetUvs(NULL);

		// Deallocate memory for the object storage in the model list
		ObjectNode * next = curObjectNode->GetNext();
		delete curObject;
		delete curObjectNode;

		curObjectNode = next;
	}

	
	return true;
}

bool Material::Load(const char * a_materialFileName, const char * a_materialName)
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
				// If we have already found the target material then this is a another declaration and we can quit out
				if (foundTargetMaterial)
				{
					file.close();
					return true;
				}

				// Cache off the material file name for later usage
				sscanf(line, "newmtl %s", &m_name);

				// Set flag so we load the next map specific in the material as the diffuse map
				foundTargetMaterial = strstr(m_name, a_materialName) != NULL;
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
					if (strstr(line, "\\") == NULL)
					{
						sscanf(line, "map_Kd %s", &tempMatName);
					}
					else
					{
						sscanf(StringUtils::ExtractFileNameFromPath(line), "%s", &tempMatName);
					}
					m_diffuseTex = TextureManager::Get().GetTexture(tempMatName, TextureCategory::Model);
				}
				// Ambient colour
				if (strstr(line, "Ka"))
				{
					sscanf(line, "Ka %f %f %f", &matX, &matY, &matZ);
					m_ambient.SetR(matX);
					m_ambient.SetG(matY);
					m_ambient.SetB(matZ);
				}
				// Diffuse colour
				else if (strstr(line, "Kd"))
				{
					sscanf(line, "Kd %f %f %f", &matX, &matY, &matZ);
					m_diffuse.SetR(matX);
					m_diffuse.SetG(matY);
					m_diffuse.SetB(matZ);
				}
				// Specular colour
				else if (strstr(line, "Ks"))
				{
					sscanf(line, "Ks %f %f %f", &matX, &matY, &matZ);
					m_specular.SetR(matX);
					m_specular.SetG(matY);
					m_specular.SetB(matZ);
				}
				// Shininess value
				else if (strstr(line, "Ns"))
				{
					float shininess = 0.0f;
					sscanf(line, "Ns %f", &shininess);
					m_shininess = (int)shininess;
				}
			}
		}
		file.close();

		if (foundTargetMaterial)
		{
			return true;
		}
		else // Report error and fail out
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot find material def %s in material file %s", a_materialName, a_materialFileName);
			return false;
		}
	}

	// No material file to load
	Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot open material file %s", a_materialFileName);
	return false;
}

