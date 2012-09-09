#ifndef _ENGINE_MODEL_H_
#define _ENGINE_MODEL_H_

class TexCoord;
class Texture;
class Vector;

//\brief Representation of a 3D model for drawing in engine.
class Model
{
public:

	// Assigned texture IDs start from 0
	Model() 
		: m_loaded(false)
		, m_diffuseTex(NULL)
		, m_normalTex(NULL)
		, m_specularTex(NULL)
		, m_verts(NULL)
		, m_normals(NULL)
		, m_uvs(NULL)
		, m_numVerts(0)
		, m_numNormals(0)
		, m_numUvs(0) {}

	~Model() { Unload(); }

	//\brief Load a TGA file into memory and store out the texture ID
	bool Load(const char * a_modelFilePath);
	bool Unload();
	inline bool IsLoaded() { return m_loaded; }

private:

	bool m_loaded;				// If the model has been loaded correctly
	Texture * m_diffuseTex;		// The texture used to draw the model
	Texture * m_normalTex;		// For drawing normal depth mapping
	Texture * m_specularTex;	// The shininess map

	Vector * m_verts;			// Pointer to vertex memory
	Vector * m_normals;			// Pointer to normal memory
	TexCoord * m_uvs;			// Pointer to tex coords

	unsigned int m_numNormals;	// How many normals in the model
	unsigned int m_numVerts;	// How many vertices are in the model
	unsigned int m_numUvs;		// Number of texture coords
};

#endif /* _ENGINE_MODEL_H_ */