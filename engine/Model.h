#ifndef _ENGINE_MODEL_H_
#define _ENGINE_MODEL_H_

#include "../core/LinearAllocator.h"
#include "../core/Vector.h"

class TexCoord;
class Texture;

//\brief Representation of a 3D model for drawing in engine.
class Model
{
public:

	// Assigned texture IDs start from 0
	Model() 
		: m_loaded(false)
		, m_displayListGenerated(false)
		, m_diffuseTex(NULL)
		, m_normalTex(NULL)
		, m_specularTex(NULL)
		, m_numFaces(0) 
		, m_displayListId(0) {}

	~Model() { if (m_loaded) { Unload(); } }

	//\brief Load a TGA file into memory and store out the texture ID
	//\param a_modelFilePath pointer to a c string containing the fully qualified path to the model to load
	//\param a memory pool ref to be used to allocate vertices while reading from the model file
	//\param a memory pool ref to be used to allocate normals while reading from the model file
	//\param a memory pool ref to be used to allocate texture coords while reading from the model file
	//\return bool true if the file was loaded successfully, false for failure
	bool Load(const char * a_modelFilePath, LinearAllocator<Vector> & a_vertPool, LinearAllocator<Vector> & a_normalPool, LinearAllocator<TexCoord> & a_uvPool);
	bool Unload();
	inline bool IsLoaded() { return m_loaded; }

	//\brief Accessors for the model's data
	inline unsigned int GetNumFaces() const { return m_numFaces; }
	inline unsigned int GetNumVertices() const { return m_numFaces * s_vertsPerTri; }
	inline Vector * GetVertices() const { return m_verts; }
	inline Vector * GetNormals() const { return m_normals; }
	inline TexCoord * GetUvs() const { return m_uvs; }

	//\brief Accessors for rendering buffer Ids
	inline bool IsDisplayListGenerated() const { return m_displayListGenerated; }
	inline unsigned int GetDisplayListId() const { return m_displayListId; }
	inline void SetDisplayListId(unsigned int a_displayListId) { m_displayListId = a_displayListId; m_displayListGenerated = true; }

	//\brief Accessors for texture data
	inline Texture * GetDiffuseTexture() const { return m_diffuseTex; }

	static const unsigned int s_vertsPerTri = 3;	///< Seems silly to have a variable for the number of sides to a triangle but it's instructional when reading code that references it

private:

	//\brief Read the material file specified in the model file and load any textures required
	//\param a_materialFileName pointer to a c string containing the file to load, adjacent to the model file itself
	//\param a_materialName is the name in the material file to use for the model
	//\return true if the material was loaded successfully and a texture for the model was assigned
	bool LoadMaterial(const char * a_materialFileName, const char * a_materialName);

	bool m_loaded;							///< If the model has been loaded correctly
	bool m_displayListGenerated;			///< If the render manager has set the buffer Ids

	Texture * m_diffuseTex;					///< The texture used to draw the model
	Texture * m_normalTex;					///< For drawing normal depth mapping
	Texture * m_specularTex;				///< The shininess map

	Vector * m_verts;						///< Storage for the verts of the model
	Vector * m_normals;						///< Storage for the normals
	TexCoord * m_uvs;						///< Storage for the tex coords
	unsigned int m_numFaces;				///< All indexed by face

	unsigned int m_displayListId;			///< Assigned by the render manager when added for rendering
};

#endif /* _ENGINE_MODEL_H_ */