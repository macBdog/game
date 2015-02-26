#ifndef _ENGINE_MODEL_H_
#define _ENGINE_MODEL_H_

#include "../core/Colour.h"
#include "../core/LinearAllocator.h"
#include "../core/Vector.h"

class DataPack;
struct DataPackEntry;
class TexCoord;
class Texture;

//\brief Object data is passed over multiple times when reading objects from file
namespace ObjectReadPass
{
	enum Enum
	{
		CountFaces = 0,								///< A first pass is done to count the number of faces
		ReadFaces = 1,								///< Then the vertex and face data is read from the file into buffers
		PatchFaces = 2,								///< Then the vertex data is patched to the face data as vertices of each face are referenced by index
	};
}

//\brief Materials define parameters passed through to each shader 
class Material
{
public:

	Material() 
		: m_ambient(1.0f)
		, m_diffuse(0.5f)
		, m_specular(0.5f)
		, m_emission(0.0f)
		, m_shininess(512)
		, m_diffuseTex(NULL)
		, m_normalTex(NULL)
		, m_specularTex(NULL) { m_name[0] = '\0'; }

	//\brief Load values and resources for a material matching a name from an mtl file
	//\param a_materialFileName pointer to a c string containing the file to load, adjacent to the model file itself
	//\param a_materialName is the name in the material file to use for the model
	//\return True if the target material was found in the file and set values of the material
	bool Load(const char * a_materialFileName, const char * a_materialName);
	bool Load(DataPackEntry * a_dataPack, const char * a_materialName);

	//\brief Accessors for texture data
	inline void SetDiffuseTexture(Texture * a_texture) { m_diffuseTex = a_texture; }
	inline void SetNormalTexture(Texture * a_texture) { m_normalTex = a_texture; }
	inline void SetSpecularTexture(Texture * a_texture) { m_specularTex = a_texture; }
	inline Texture * GetDiffuseTexture() const { return m_diffuseTex; }
	inline Texture * GetNormalTexture() const { return m_normalTex; }
	inline Texture * GetSpecularTexture() const { return m_specularTex; }

	Colour m_ambient;								///< Ambient light value
	Colour m_diffuse;
	Colour m_specular;
	Colour m_emission;
	int m_shininess;

private:

	//\brief Load function will work with either datapack entry or input stream
	template <typename TInputData>
	bool LoadData(TInputData & a_input, const char * a_materialName)
	{
		// Storage for material file reading progress
		char line[StringUtils::s_maxCharsPerLine];
		memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);

		unsigned int lineCount = 0;
		bool foundTargetMaterial = false;

		// Open the file and parse each line 
		if (a_input.is_open())
		{
			while (a_input.good())
			{
				a_input.getline(line, StringUtils::s_maxCharsPerLine);
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
						a_input.close();
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
					if (strstr(line, "map_Kd"))
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
					// Spec map name
					else if (strstr(line, "map_Ks"))
					{
						char tempMatName[StringUtils::s_maxCharsPerLine];
						memset(&tempMatName, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
						if (strstr(line, "\\") == NULL)
						{
							sscanf(line, "map_Ks %s", &tempMatName);
						}
						else
						{
							sscanf(StringUtils::ExtractFileNameFromPath(line), "%s", &tempMatName);
						}
						m_specularTex = TextureManager::Get().GetTexture(tempMatName, TextureCategory::Model);
					}
					// Ambient colour
					else if (strstr(line, "Ka"))
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
			a_input.close();

			if (foundTargetMaterial)
			{
				return true;
			}
			else // Report error and fail out
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot find material def %s", a_materialName);
				return false;
			}
		}

		// No material data to load
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot open material file");
		return false;
	}

	char m_name[StringUtils::s_maxCharsPerName];	///< Name of the material as referenced by the model's mtl file
	Texture * m_diffuseTex;							///< The texture used to draw the model
	Texture * m_normalTex;							///< For drawing normal depth mapping
	Texture * m_specularTex;						///< The shininess map
};

//\brief Each model is comprised of one or more objects
class Object
{
public:

	Object()
		: m_material(NULL)
		, m_displayListGenerated(false)
		, m_verts(NULL)
		, m_normals(NULL)
		, m_uvs(NULL)
		, m_numVertices(0) 
		, m_displayListId(0) { m_name[0] = '\0'; }

	inline const char * GetName() { return m_name; }
	inline void SetName(const char * a_name) { strncpy(m_name, a_name, strlen(a_name) + 1); }
	inline void SetNumVertices(unsigned int a_numFaces) { m_numVertices = a_numFaces; }
	inline void SetVertices(Vector * a_verts) { m_verts = a_verts; }
	inline void SetNormals(Vector * a_normals) { m_normals = a_normals; }
	inline void SetUvs(TexCoord * a_uvs) { m_uvs = a_uvs; }
	inline unsigned int GetNumVertices() const { return m_numVertices; }
	inline Vector * GetVertices() const { return m_verts; }
	inline Vector * GetNormals() const { return m_normals; }
	inline TexCoord * GetUvs() const { return m_uvs; }

	//\brief Accessors for rendering buffer Ids
	inline bool IsDisplayListGenerated() const { return m_displayListGenerated; }
	inline unsigned int GetDisplayListId() const { return m_displayListId; }
	inline void SetDisplayListId(unsigned int a_displayListId) { m_displayListId = a_displayListId; m_displayListGenerated = true; }
	
	//\brief Accessors for material data
	inline void SetMaterial(Material * a_material) { m_material = a_material; }
	inline Material * GetMaterial() { return m_material; }
	
	//\brief Read the material file specified in the model file and load any textures required
	//\param a_materialFileName pointer to a c string containing the file to load, adjacent to the model file itself
	//\param a_materialName is the name in the material file to use for the model
	//\return true if the material was loaded successfully and a texture for the model was assigned
	bool LoadMaterial(const char * a_materialFileName, const char * a_materialName);

private:

	char m_name[StringUtils::s_maxCharsPerName];	///< Name of the object as referenced by the model file
	Material * m_material;							///< Material properties loaded from file

	unsigned int m_numVertices;						///< All indexed by vertex
	unsigned int m_displayListId;					///< Assigned by the render manager when added for rendering
	bool m_displayListGenerated;					///< If the render manager has set the buffer Ids

	Vector * m_verts;								///< Storage for the verts of the model
	Vector * m_normals;								///< Storage for the normals
	TexCoord * m_uvs;								///< Storage for the diffuse tex coords
};

//\brief A model data pool is a neat way to pass around the memory pools required to load a model
struct ModelDataPool
{
	ModelDataPool(LinearAllocator<Object> & a_objectPool, LinearAllocator<Vector> & a_vertPool, LinearAllocator<Vector> & a_normalPool, LinearAllocator<TexCoord> & a_uvPool, LinearAllocator<Material> & m_materialPool)
		: m_objectPool(a_objectPool)
		, m_vertPool(a_vertPool)
		, m_normalPool(a_normalPool)
		, m_uvPool(a_uvPool)
		, m_materialPool(m_materialPool) {}

	LinearAllocator<Object> & m_objectPool;			//\param a memory pool ref to be used to allocate objects while reading from the model file
	LinearAllocator<Vector> & m_vertPool;			//\param a memory pool ref to be used to allocate vertices while reading from the model file
	LinearAllocator<Vector> & m_normalPool;			//\param a memory pool ref to be used to allocate normals while reading from the model file
	LinearAllocator<TexCoord> & m_uvPool;			//\param a memory pool ref to be used to allocate texture coords while reading from the model file
	LinearAllocator<Material> & m_materialPool;		//\param a memory pool ref to be used to allocate materials while reading from the model file
};

//\brief Representation of a 3D model for drawing in engine.
class Model
{
public:

	// Assigned texture IDs start from 0
	Model() { m_name[0] = '\0'; m_materialFileName[0] = '\0'; }
	~Model();

	//\brief Load a TGA file into memory and store out the texture ID
	//\param a_modelFilePath pointer to a c string containing the fully qualified path to the model to load
	//\param a_modelDataPool is a memory structure used to store all the verts and coors while loading
	//\param a_dataPack is the source datapack so the model can load other linked resources like materials
	//\return bool true if the file was loaded successfully, false for failure
	bool Load(const char * a_modelFilePath, ModelDataPool & a_modelDataPool);
	bool Load(DataPackEntry * a_packedModel, ModelDataPool & a_modelDataPool, DataPack * a_dataPack);

	bool Unload();
	inline bool IsLoaded() { return m_objects.GetLength() > 0; }

	//\brief Accessors for the model's data
	inline const char * GetName() const { return m_name; }
	inline const char * GetMaterialFileName() const { return m_materialFileName;  }
	inline unsigned int GetNumObjects() const { return m_objects.GetLength(); }
	inline Object * GetObject(unsigned int a_objectIndex) 
	{
		ObjectNode * curObject = m_objects.GetHead();
		for (unsigned int i = 0; i < a_objectIndex; ++i)
		{
			curObject = curObject->GetNext();
		}
		return curObject->GetData();
	}
	static const unsigned int s_vertsPerTri = 3;	///< Seems silly to have a variable for the number of sides to a triangle but it's instructional when reading code that references it

private:

	typedef LinkedList<Object> ObjectList;
	typedef LinkedListNode<Object> ObjectNode;

	//\brief Load function will work with either datapack entry or input stream
	template <typename TInputData>
	bool LoadData(TInputData & a_input, ModelDataPool & a_modelDataPool, const char * a_modelFilePath, DataPack * a_dataPack = NULL)
	{
		// Storage for model file reading progress
		char line[StringUtils::s_maxCharsPerLine];
		memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);

		// Storage for material file reading progress
		char materialFilePath[StringUtils::s_maxCharsPerLine];
		strcpy(materialFilePath, a_modelFilePath);
		StringUtils::TrimFileNameFromPath(materialFilePath);

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

		// Open the file and parse each line 
		if (a_input.is_open())
		{
			while (a_input.good())
			{
				lastReadOffset = a_input.tellg();
				a_input.getline(line, StringUtils::s_maxCharsPerLine);

				// Comment
				if (strstr(line, "#"))
				{
					continue;
				}
				// Material library declaration
				else if (strstr(line, "mtllib"))
				{
					// Cache off the material file name for later usage
					sscanf(line, "mtllib %s", &m_materialFileName);

					// Append the filename onto the file path
					StringUtils::AppendString(materialFilePath, m_materialFileName);
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
					currentObject->SetName("");
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
								Material * newMaterial = a_modelDataPool.m_materialPool.Allocate(sizeof(Material), true);
								bool materialLoaded = false;
								if (a_dataPack != NULL && a_dataPack->IsLoaded())
								{
									if (DataPackEntry * packedMaterial = a_dataPack->GetEntry(materialFilePath))
									{
										materialLoaded = newMaterial->Load(packedMaterial, materialName);
									}
								}
								else
								{
									materialLoaded = newMaterial->Load(materialFilePath, materialName);
								}

								if (materialLoaded)
								{
									currentObject->SetMaterial(newMaterial);
								}
								else
								{
									Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Material load failed for material called %s in file %s!", materialName, materialFilePath);
									a_modelDataPool.m_materialPool.DeAllocate(sizeof(Material));
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
									a_input.close();
									return false;
								}
								else
								{
									++numFaces;
								}
							}
						}
						lastReadOffset = a_input.tellg();
						a_input.getline(line, StringUtils::s_maxCharsPerLine);
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
					if (!a_input.good())
					{
						a_input.seekg(0, ios::beg);
						a_input.clear();
					}
					a_input.seekg((int)currentObjectOffset, ios::beg);
					a_input.getline(line, StringUtils::s_maxCharsPerLine);

					// If using object names, read an extra line to get to the verts
					if (strlen(currentObject->GetName()) > 0)
					{
						a_input.getline(line, StringUtils::s_maxCharsPerLine);
					}

					while (line[0] == 'v' || line[0] == 'f' || line[0] == 'g' || line[0] == 's' || line[0] == 'm' || line[0] == 'u')
					{
						// Vertex
						if (line[0] == 'v' && line[1] == ' ')
						{
							if (Vector * v = a_modelDataPool.m_vertPool.Allocate(sizeof(Vector)))
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
							if (TexCoord * t = a_modelDataPool.m_uvPool.Allocate(sizeof(TexCoord)))
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
							if (Vector * vn = a_modelDataPool.m_normalPool.Allocate(sizeof(Vector)))
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
								vertIndices += 3;
								uvIndices += 3;
								normIndices += 3;
								++numFacesPatched;
							}
							else
							{
								Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Model file %s contains faces that have not been unwrapped!", a_modelFilePath);
							}
						}
						lastReadOffset = a_input.tellg();
						a_input.getline(line, StringUtils::s_maxCharsPerLine);
						objectReadPass = numFacesPatched == numFaces ? ObjectReadPass::PatchFaces : ObjectReadPass::ReadFaces;
					}

					// Last step is to patch in the mesh data to the faces
					if (objectReadPass == ObjectReadPass::PatchFaces)
					{
						unsigned int numObjectVertices = 0;
						if (objectVerts != NULL && objectNormals != NULL && objectUvs != NULL)
						{
							// Create an alias to the loading memory pools so the data can be accessed randomly
							Vector * verts = a_modelDataPool.m_vertPool.GetHead();
							Vector * normals = a_modelDataPool.m_normalPool.GetHead();
							TexCoord * uvs = a_modelDataPool.m_uvPool.GetHead();
							vertIndices = vertIndexPool.GetHead();
							normIndices = normIndexPool.GetHead();
							uvIndices = uvIndexPool.GetHead();

							// Now map faces to triangles by linking vertex, uv and normals by index
							for (unsigned int i = 0; i < numFaces; ++i)
							{
								// Set the face data up
								objectVerts[numObjectVertices] = verts[vertIndices[0] - objectVertOffset];
								objectNormals[numObjectVertices] = normals[normIndices[0] - objectNormOffset];
								objectUvs[numObjectVertices] = uvs[uvIndices[0] - objectUvOffset];
								numObjectVertices++;

								objectVerts[numObjectVertices] = verts[vertIndices[1] - objectVertOffset];
								objectNormals[numObjectVertices] = normals[normIndices[1] - objectNormOffset];
								objectUvs[numObjectVertices] = uvs[uvIndices[1] - objectUvOffset];
								numObjectVertices++;

								objectVerts[numObjectVertices] = verts[vertIndices[2] - objectVertOffset];
								objectNormals[numObjectVertices] = normals[normIndices[2] - objectNormOffset];
								objectUvs[numObjectVertices] = uvs[uvIndices[2] - objectUvOffset];
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
						a_modelDataPool.m_vertPool.DeAllocate(sizeof(Vector) * numVerts);
						a_modelDataPool.m_normalPool.DeAllocate(sizeof(Vector) * numNormals);
						a_modelDataPool.m_uvPool.DeAllocate(sizeof(TexCoord) * numUvs);

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
		}

		// Model data loaded succesfully
		a_input.close();
		return m_objects.GetLength() > 0;
	}

	char m_name[StringUtils::s_maxCharsPerName];				///< Name of the model as referenced by the game
	char m_materialFileName[StringUtils::s_maxCharsPerName];	///< Name of the material file referenced by the model game
	ObjectList m_objects;										///< List of pointers to the objects that are loaded
};

#endif /* _ENGINE_MODEL_H_ */