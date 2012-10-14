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
		, m_diffuseTex(NULL)
		, m_normalTex(NULL)
		, m_specularTex(NULL)
		, m_numFaces(0) {}

	~Model() { Unload(); }

	//\brief Load a TGA file into memory and store out the texture ID
	//\param a_modelFilePath pointer to a c string containing the fully qualified path to the model to load
	//\param a memory pool ref to be used to allocate vertices while reading from the model file
	//\param a memory pool ref to be used to allocate normals while reading from the model file
	//\param a memory pool ref to be used to allocate texture coords while reading from the model file
	//\return bool true if the file was loaded successfully, false for failure
	bool Load(const char * a_modelFilePath, LinearAllocator<Vector> & a_vertPool, LinearAllocator<Vector> & a_normalPool, LinearAllocator<TexCoord> & a_uvPool);
	bool Unload();
	inline bool IsLoaded() { return m_loaded; }

private:

	///\brief Vertex data stored on the model is organised by face
	struct Face
	{
		Vector m_verts[3];
		Vector m_normals[3];
		TexCoord m_uvs[3];
	};

	bool m_loaded;				// If the model has been loaded correctly

	Texture * m_diffuseTex;		// The texture used to draw the model
	Texture * m_normalTex;		// For drawing normal depth mapping
	Texture * m_specularTex;	// The shininess map

	Face * m_faces;
	unsigned int m_numFaces;
};

#endif /* _ENGINE_MODEL_H_ */