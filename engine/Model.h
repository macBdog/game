#ifndef _ENGINE_MODEL_H_
#define _ENGINE_MODEL_H_

#include "../core/Colour.h"
#include "../core/LinearAllocator.h"
#include "../core/Vector.h"

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

	void SetName(const char * a_name) { strncpy(m_name, a_name, strlen(a_name) + 1); }
	void SetNumVertices(unsigned int a_numFaces) { m_numVertices = a_numFaces; }
	void SetVertices(Vector * a_verts) { m_verts = a_verts; }
	void SetNormals(Vector * a_normals) { m_normals = a_normals; }
	void SetUvs(TexCoord * a_uvs) { m_uvs = a_uvs; }
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
	Model() { m_name[0] = '\0'; }
	~Model();

	//\brief Load a TGA file into memory and store out the texture ID
	//\param a_modelFilePath pointer to a c string containing the fully qualified path to the model to load
	//\return bool true if the file was loaded successfully, false for failure
	bool Load(const char * a_modelFilePath, ModelDataPool & a_modelDataPool);
	bool Unload();
	inline bool IsLoaded() { return m_objects.GetLength() > 0; }

	//\brief Accessors for the model's data
	inline const char * GetName() const { return m_name; }
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

	char m_name[StringUtils::s_maxCharsPerName];	///< Name of the model as referenced by the game
	ObjectList m_objects;							///< List of pointers to the objects that are loaded
};

#endif /* _ENGINE_MODEL_H_ */